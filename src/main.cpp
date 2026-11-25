#include "common.h"
#include "layout.h"
#include "settings.h"
#include "serial.h"
#include "capture.h"
#include "effects.h"
#include "theme.h"
#include "ui.h"

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

SerialPort    g_serial;
Capturer      g_capturer;
EffectsEngine g_effects;

static void LEDLoop() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    bool usedTimeBegin = false;
    HANDLE hTimer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!hTimer) {
        hTimer = CreateWaitableTimerW(NULL, TRUE, NULL);
        if (timeBeginPeriod(1) == TIMERR_NOERROR) {
            usedTimeBegin = true;
        }
    }

    auto SleepMs = [&](double ms) {
        if (ms <= 0.2) return;
        if (hTimer) {
            LARGE_INTEGER due;
            due.QuadPart = -(LONGLONG)(ms * 10000.0);
            if (SetWaitableTimer(hTimer, &due, 0, NULL, NULL, FALSE)) {
                WaitForSingleObject(hTimer, (DWORD)(ms + 10.0));
                return;
            }
        }
        Sleep((DWORD)ms);
    };

    auto tLast      = std::chrono::steady_clock::now();
    auto tLastSend  = tLast - std::chrono::seconds(1);
    auto tLastRetry = tLast - std::chrono::seconds(10);

    std::vector<RGB3>    colors;
    std::vector<RGB3>    screenColors;
    std::vector<uint8_t> pkt;
    std::vector<uint8_t> lastPkt;
    bool sentBlackOff = false;

    float fpsBuf[64] = {};
    int   fpsIdx = 0;

    while (g_running.load(std::memory_order_acquire)) {
        auto tNow = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(tNow - tLast).count();
        tLast = tNow;

        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            s = g_settings;
        }

        if (!s.leds_enabled && !g_wizardActive.load(std::memory_order_relaxed)) {
            if (!sentBlackOff && g_connected.load()) {
                g_serial.SendBlack(s.led_count);
                sentBlackOff = true;
            }
            lastPkt.clear();
            if (g_uiVisible.load(std::memory_order_relaxed)) {
                std::lock_guard<std::mutex> lk(g_colors_mtx);
                g_last_colors.assign(std::max(1, s.led_count), {0, 0, 0});
            }
            g_fps_actual.store(0.0f, std::memory_order_relaxed);
            SleepMs(100.0);
            continue;
        }
        sentBlackOff = false;

        bool connected = g_connected.load(std::memory_order_acquire);
        if (!connected) {
            bool wantRetry = g_rescan.exchange(false, std::memory_order_relaxed) ||
                             (std::chrono::duration<double>(tNow - tLastRetry).count() >= 5.0);
            if (wantRetry) {
                tLastRetry = tNow;
                std::wstring port = s.port;
                if (port == L"AUTO") {
                    auto ports = ScanPorts();
                    port = AutoDetectArduino(ports);
                }
                if (!port.empty() && port != L"AUTO") {
                    if (g_serial.Connect(port, s.baud)) {
                        g_connected.store(true, std::memory_order_release);
                        lastPkt.clear();
                        if (g_hwnd) PostMessageW(g_hwnd, WM_STATUS_UPDATE, 1, 0);
                        connected = true;
                    }
                }
            }
        }

        bool uiVis = g_uiVisible.load(std::memory_order_relaxed);

        // If disconnected and UI is hidden, do zero work until next retry
        if (!connected && !uiVis) {
            g_fps_actual.store(0.0f, std::memory_order_relaxed);
            SleepMs(200.0);
            continue;
        }

        g_effects.Tick(dt);

        const std::vector<RGB3>* scPtr = nullptr;
        if (s.effect == 0 && g_capturer.GetColors(screenColors)) {
            scPtr = &screenColors;
        }
        g_effects.Render(s, scPtr, colors);

        if (uiVis) {
            std::lock_guard<std::mutex> lk(g_colors_mtx);
            g_last_colors = colors;
        }

        if (connected) {
            BuildAdalightPacket(colors, s.color_order, pkt);
            double sinceSendMs = std::chrono::duration<double, std::milli>(tNow - tLastSend).count();
            if (pkt != lastPkt || sinceSendMs >= 250.0) {
                if (!g_serial.Send(pkt.data(), pkt.size())) {
                    g_connected.store(false, std::memory_order_release);
                    lastPkt.clear();
                    if (g_hwnd) PostMessageW(g_hwnd, WM_STATUS_UPDATE, 0, 0);
                } else {
                    lastPkt = pkt;
                    tLastSend = tNow;
                }
            }
        }

        // FPS accounting
        if (dt > 0.0001f && dt < 1.0f) {
            fpsBuf[fpsIdx % 64] = 1.0f / dt;
            ++fpsIdx;
            int cnt = std::min(fpsIdx, 64);
            float sum = 0.0f;
            for (int i = 0; i < cnt; ++i) sum += fpsBuf[i];
            g_fps_actual.store(sum / cnt, std::memory_order_relaxed);
        }

        // Disconnected preview runs at ~30 Hz; connected runs at target_fps
        int targetFps = connected ? std::clamp(s.target_fps, 1, 240) : 30;
        double frameMs = 1000.0 / targetFps;
        double elapsedMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - tNow).count();
        SleepMs(frameMs - elapsedMs - 0.3);
    }

    if (g_connected.load()) {
        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            s = g_settings;
        }
        g_serial.SendBlack(s.led_count);
        g_serial.Disconnect();
        g_connected.store(false);
    }

    if (hTimer) CloseHandle(hTimer);
    if (usedTimeBegin) timeEndPeriod(1);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow) {
    WM_APP_SHOW        = RegisterWindowMessageW(L"ETDAmbilight_Show");
    WM_TASKBAR_CREATED = RegisterWindowMessageW(L"TaskbarCreated");

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"ETDAmbilight_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(L"ETDAmbilightMain", NULL);
        if (existing && WM_APP_SHOW) {
            PostMessageW(existing, WM_APP_SHOW, 0, 0);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    // DPI awareness (Per-Monitor v2 with fallback)
    typedef BOOL(WINAPI* SetDpiCtxFn)(HANDLE);
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        auto fn = (SetDpiCtxFn)GetProcAddress(hUser, "SetProcessDpiAwarenessContext");
        if (fn) fn((HANDLE)(INT_PTR)-4);
        else {
            typedef BOOL(WINAPI* OldDpiFn)();
            auto fn2 = (OldDpiFn)GetProcAddress(hUser, "SetProcessDPIAware");
            if (fn2) fn2();
        }
    }

    INITCOMMONCONTROLSEX icc = {
        sizeof(icc),
        ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_UPDOWN_CLASS
    };
    InitCommonControlsEx(&icc);

    LoadSettings(g_settings);
    if (g_settings.autostart) {
        SetAutostart(true);
    }
    g_prevNonCalibEffect = (g_settings.effect != 11) ? g_settings.effect : 0;

    g_hAppIcon   = (HICON)LoadImageW(hInst, L"MAINICON", IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    g_hSmallIcon = (HICON)LoadImageW(hInst, L"MAINICON", IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!g_hAppIcon)   g_hAppIcon   = LoadIcon(NULL, IDI_APPLICATION);
    if (!g_hSmallIcon) g_hSmallIcon = g_hAppIcon;

    // Register Preview Canvas class
    WNDCLASSEXW wcPrev = {sizeof(wcPrev)};
    wcPrev.lpfnWndProc   = PreviewCanvasProc;
    wcPrev.hInstance     = hInst;
    wcPrev.lpszClassName = L"ETDAmbilightPreviewCanvas";
    wcPrev.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wcPrev);

    // Register Modal Dialog class
    WNDCLASSEXW wcDlg = {sizeof(wcDlg)};
    wcDlg.lpfnWndProc   = DialogWndProc;
    wcDlg.hInstance     = hInst;
    wcDlg.lpszClassName = L"ETDAmbilightDialog";
    wcDlg.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcDlg.hIcon         = g_hAppIcon;
    wcDlg.hIconSm       = g_hSmallIcon;
    RegisterClassExW(&wcDlg);

    // Register Color Calibration Wizard class
    WNDCLASSEXW wcWiz = {sizeof(wcWiz)};
    wcWiz.lpfnWndProc   = WizardWndProc;
    wcWiz.hInstance     = hInst;
    wcWiz.lpszClassName = L"ETDAmbilightWizard";
    wcWiz.hbrBackground = CreateSolidBrush(CLR_BG);
    wcWiz.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcWiz.hIcon         = g_hAppIcon;
    wcWiz.hIconSm       = g_hSmallIcon;
    RegisterClassExW(&wcWiz);

    // Register Main Window class
    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"ETDAmbilightMain";
    wc.hbrBackground = CreateSolidBrush(CLR_BG);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = g_hAppIcon;
    wc.hIconSm       = g_hSmallIcon;
    RegisterClassExW(&wc);

    UpdateDpiForWindow(NULL);
    g_hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"ETDAmbilightMain",
        LStr(L"ETDAmbilight — Adalight Ambilight Kontrolcüsü",
             L"ETDAmbilight — Adalight Ambilight Controller"),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        S(1120), S(720),
        NULL, NULL, hInst, NULL);

    if (!g_hwnd) {
        if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
        return 1;
    }

    bool isAutostart = (lpCmdLine && wcsstr(lpCmdLine, L"--autostart") != NULL);
    bool startHidden = isAutostart && g_settings.start_hidden;

    if (startHidden) {
        g_uiVisible.store(false, std::memory_order_relaxed);
    } else {
        ShowWindow(g_hwnd, nCmdShow);
        UpdateWindow(g_hwnd);
        g_uiVisible.store(true, std::memory_order_relaxed);
        SetTimer(g_hwnd, 1, 33, NULL);
    }

    g_capturer.Start();
    g_running.store(true, std::memory_order_release);
    std::thread ledThread(LEDLoop);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_running.store(false, std::memory_order_release);
    if (ledThread.joinable()) ledThread.join();
    g_capturer.Stop();

    {
        std::lock_guard<std::mutex> lk(g_settings_mtx);
        SaveSettings(g_settings, false);
    }

    if (g_hAppIcon)   DestroyIcon(g_hAppIcon);
    if (g_hSmallIcon && g_hSmallIcon != g_hAppIcon) DestroyIcon(g_hSmallIcon);
    if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
    return (int)msg.wParam;
}
