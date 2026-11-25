#pragma once

#include "common.h"
#include "layout.h"
#include "settings.h"
#include "effects.h"
#include "theme.h"

// Forward declarations from ui.h
static void SyncControlsFromSettings();

enum WizCtrlId {
    WIZ_BTN_CANCEL = 4001,
    WIZ_BTN_BACK,
    WIZ_BTN_NEXT,
    WIZ_COMBO_ORDER,
    WIZ_BTN_PRIM_R,
    WIZ_BTN_PRIM_G,
    WIZ_BTN_PRIM_B,
    WIZ_SLIDER_R,
    WIZ_SLIDER_G,
    WIZ_SLIDER_B,
    WIZ_BTN_RESET_RGB,
    WIZ_SLIDER_GAMMA,
    WIZ_SLIDER_BRIGHT,
    WIZ_SLIDER_SAT,
    WIZ_SLIDER_SMOOTH,
};

struct WizardState {
    HWND     parent = NULL;
    Settings backup;
    int      step = 0;
    bool     saved = false;

    // Backbuffer + pre-rendered sample scene DIB
    HDC      memDC = NULL;
    HBITMAP  hbm = NULL;
    HGDIOBJ  oldHbm = NULL;
    int      bufW = 0, bufH = 0;
    HBITMAP  hSceneBmp = NULL;
    int      sceneW = 240, sceneH = 135;

    // Step 0 controls
    HWND hLblOrder = NULL, hComboOrder = NULL;
    HWND hBtnPrimR = NULL, hBtnPrimG = NULL, hBtnPrimB = NULL;

    // Step 1 controls (RGB White Balance)
    HWND hLblR = NULL, hSliderR = NULL, hValR = NULL;
    HWND hLblG = NULL, hSliderG = NULL, hValG = NULL;
    HWND hLblB = NULL, hSliderB = NULL, hValB = NULL;
    HWND hBtnResetRGB = NULL;

    // Step 2 controls (Gamma)
    HWND hLblGamma = NULL, hSliderGamma = NULL, hValGamma = NULL;

    // Step 3 controls (Sample Image)
    HWND hLblBright = NULL, hSliderBright = NULL, hValBright = NULL;
    HWND hLblSat = NULL, hSliderSat = NULL, hValSat = NULL;
    HWND hLblSmooth = NULL, hSliderSmooth = NULL, hValSmooth = NULL;

    // Footer buttons
    HWND hBtnCancel = NULL, hBtnBack = NULL, hBtnNext = NULL;

    void EnsureBackBuf(HDC ref, int w, int h) {
        if (w <= 0 || h <= 0) return;
        if (memDC && hbm && bufW == w && bufH == h) return;
        if (memDC && oldHbm) { SelectObject(memDC, oldHbm); oldHbm = NULL; }
        if (hbm)   { DeleteObject(hbm); hbm = NULL; }
        if (memDC) { DeleteDC(memDC); memDC = NULL; }
        memDC = CreateCompatibleDC(ref);
        hbm = CreateCompatibleBitmap(ref, w, h);
        if (memDC && hbm) oldHbm = SelectObject(memDC, hbm);
        bufW = w;
        bufH = h;
    }

    void EnsureSceneDIB(HDC ref) {
        if (hSceneBmp) return;
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = sceneW;
        bi.bmiHeader.biHeight = -sceneH;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        hSceneBmp = CreateDIBSection(ref, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
        if (hSceneBmp && bits) {
            DWORD* px = (DWORD*)bits;
            for (int y = 0; y < sceneH; ++y) {
                float v = (y + 0.5f) / (float)sceneH;
                for (int x = 0; x < sceneW; ++x) {
                    float u = (x + 0.5f) / (float)sceneW;
                    RGB3 c = SampleCalibrationScene(u, v);
                    px[y * sceneW + x] = ((DWORD)c.r << 16) | ((DWORD)c.g << 8) | (DWORD)c.b;
                }
            }
        }
    }

    void Release() {
        if (memDC && oldHbm) { SelectObject(memDC, oldHbm); oldHbm = NULL; }
        if (hbm)       { DeleteObject(hbm); hbm = NULL; }
        if (memDC)     { DeleteDC(memDC); memDC = NULL; }
        if (hSceneBmp) { DeleteObject(hSceneBmp); hSceneBmp = NULL; }
        bufW = bufH = 0;
    }
};

static inline HWND WizLbl(HWND parent, const wchar_t* txt, COLORREF fg = CLR_TEXT, HFONT f = NULL) {
    HWND h = CreateWindowExW(0, L"STATIC", txt, WS_CHILD | SS_LEFT | SS_NOPREFIX,
                             0, 0, 10, 10, parent, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)(f ? f : hFontUI), TRUE);
    SetPropW(h, L"fg", (HANDLE)(UINT_PTR)fg);
    return h;
}

static inline HWND WizBtn(HWND parent, const wchar_t* txt, int id, COLORREF bg = CLR_CTRL_BG) {
    HWND h = CreateWindowExW(0, L"BUTTON", txt, WS_CHILD | BS_OWNERDRAW,
                             0, 0, 10, 10, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SetBtnBg(h, bg);
    SetPropW(h, L"in_topbar", (HANDLE)1);
    AttachHoverSubclass(h);
    return h;
}

static inline HWND WizSlider(HWND parent, int id, int minV, int maxV, int val) {
    HWND h = CreateWindowExW(0, TRACKBAR_CLASSW, NULL, WS_CHILD | TBS_HORZ | TBS_NOTICKS,
                             0, 0, 10, 10, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, TBM_SETRANGE, TRUE, MAKELONG(minV, maxV));
    SendMessageW(h, TBM_SETPOS, TRUE, val);
    return h;
}

static inline void UpdateWizardLabels(WizardState* st, const Settings& s) {
    wchar_t buf[32];
    swprintf_s(buf, L"%d%%", s.gain_r); SetWindowTextW(st->hValR, buf);
    swprintf_s(buf, L"%d%%", s.gain_g); SetWindowTextW(st->hValG, buf);
    swprintf_s(buf, L"%d%%", s.gain_b); SetWindowTextW(st->hValB, buf);
    swprintf_s(buf, L"%.1f", s.gamma);  SetWindowTextW(st->hValGamma, buf);
    swprintf_s(buf, L"%d%%", s.brightness);  SetWindowTextW(st->hValBright, buf);
    swprintf_s(buf, L"%.1fx", s.saturation); SetWindowTextW(st->hValSat, buf);
    swprintf_s(buf, L"%.2f", s.smoothing);   SetWindowTextW(st->hValSmooth, buf);
}

static inline void UpdateWizardPrimButtons(WizardState* st) {
    int p = g_wizardPrimColor.load();
    SetBtnBg(st->hBtnPrimR, (p == 0) ? RGB(200, 45, 45) : CLR_CTRL_BG);
    SetBtnBg(st->hBtnPrimG, (p == 1) ? RGB(40, 165, 85) : CLR_CTRL_BG);
    SetBtnBg(st->hBtnPrimB, (p == 2) ? RGB(50, 115, 220) : CLR_CTRL_BG);
    InvalidateRect(st->hBtnPrimR, NULL, FALSE);
    InvalidateRect(st->hBtnPrimG, NULL, FALSE);
    InvalidateRect(st->hBtnPrimB, NULL, FALSE);
}

static inline void ShowWizardStep(HWND hwnd, WizardState* st, int newStep) {
    st->step = std::clamp(newStep, 0, 3);
    g_wizardStep.store(st->step, std::memory_order_relaxed);

    auto Show = [&](HWND h, bool vis) { if (h) ShowWindow(h, vis ? SW_SHOW : SW_HIDE); };

    Show(st->hLblOrder,   st->step == 0);
    Show(st->hComboOrder, st->step == 0);
    Show(st->hBtnPrimR,   st->step == 0);
    Show(st->hBtnPrimG,   st->step == 0);
    Show(st->hBtnPrimB,   st->step == 0);

    Show(st->hLblR,        st->step == 1);
    Show(st->hSliderR,     st->step == 1);
    Show(st->hValR,        st->step == 1);
    Show(st->hLblG,        st->step == 1);
    Show(st->hSliderG,     st->step == 1);
    Show(st->hValG,        st->step == 1);
    Show(st->hLblB,        st->step == 1);
    Show(st->hSliderB,     st->step == 1);
    Show(st->hValB,        st->step == 1);
    Show(st->hBtnResetRGB, st->step == 1);

    Show(st->hLblGamma,    st->step == 2);
    Show(st->hSliderGamma, st->step == 2);
    Show(st->hValGamma,    st->step == 2);

    Show(st->hLblBright,   st->step == 3);
    Show(st->hSliderBright,st->step == 3);
    Show(st->hValBright,   st->step == 3);
    Show(st->hLblSat,      st->step == 3);
    Show(st->hSliderSat,   st->step == 3);
    Show(st->hValSat,      st->step == 3);
    Show(st->hLblSmooth,   st->step == 3);
    Show(st->hSliderSmooth,st->step == 3);
    Show(st->hValSmooth,   st->step == 3);

    Show(st->hBtnCancel, true);
    Show(st->hBtnBack, st->step > 0);
    Show(st->hBtnNext, true);
    if (st->step == 3) {
        SetWindowTextW(st->hBtnNext, LStr(L"İyi, Kaydet && Bitir", L"Looks Good, Save && Finish"));
        SetBtnBg(st->hBtnNext, RGB(38, 158, 108));
    } else {
        SetWindowTextW(st->hBtnNext, LStr(L"İleri >", L"Next >"));
        SetBtnBg(st->hBtnNext, CLR_ACCENT);
    }
    InvalidateRect(st->hBtnNext, NULL, FALSE);
    InvalidateRect(hwnd, NULL, FALSE);
}

static inline void PaintWizard(HWND hwnd, WizardState* st, HDC hdc, int W, int H) {
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));

    SetDCBrushColor(hdc, CLR_BG);
    SetDCPenColor(hdc, CLR_BG);
    Rectangle(hdc, 0, 0, W, H);

    // Step pills bar
    const wchar_t* stepNames[4] = {
        LStr(L"1. Saf Kırmızı", L"1. Pure Red"),
        LStr(L"2. Tam Beyaz", L"2. Pure White"),
        LStr(L"3. Gamma", L"3. Gamma"),
        LStr(L"4. Örnek Görüntü", L"4. Sample Scene")
    };
    int pillW = (W - S(48) - S(24)) / 4;
    int pillH = S(30);
    int py = S(14);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hFontSemi);
    for (int i = 0; i < 4; ++i) {
        int px = S(24) + i * (pillW + S(8));
        bool act = (i == st->step);
        bool done = (i < st->step);
        COLORREF pbg = act ? CLR_ACCENT : (done ? RGB(38, 72, 58) : CLR_CARD);
        COLORREF pbd = act ? CLR_ACCENT : (done ? CLR_SUCCESS : CLR_BORDER);
        SetDCBrushColor(hdc, pbg);
        SetDCPenColor(hdc, pbd);
        RoundRect(hdc, px, py, px + pillW, py + pillH, S(8), S(8));
        SetTextColor(hdc, act ? RGB(255, 255, 255) : (done ? CLR_SUCCESS : CLR_TEXT2));
        RECT prc = {px, py, px + pillW, py + pillH};
        DrawTextW(hdc, stepNames[i], -1, &prc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Step title & instruction
    const wchar_t* titles[4] = {
        LStr(L"Adım 1: Ana Renkler ve Kanal Sırası Kontrolü",
             L"Step 1: Primary Colors & Channel Order"),
        LStr(L"Adım 2: Tam Beyaz Dengesi (RGB Kazanç)",
             L"Step 2: Pure White Balance (RGB Gain)"),
        LStr(L"Adım 3: Gamma & Parlaklık Geçişi",
             L"Step 3: Gamma & Luminance Curve"),
        LStr(L"Adım 4: Örnek Görüntü Karşılaştırması — İyi mi?",
             L"Step 4: Sample Scene Comparison — Looks Good?")
    };
    const wchar_t* descs[4] = {
        LStr(L"Şeridin ekrandaki saf renkle aynı yanmalı. Kırmızı yerine yeşil/mavi yanıyorsa Kanal Sırası'nı değiştir.",
             L"Your LED strip should match the pure color on screen. If red shows green or blue, change Channel Order."),
        LStr(L"Arka duvardaki ışık ekrandaki saf beyazla aynı nötr beyaz olana kadar Kırmızı, Yeşil, Mavi sürgülerini kaydır.",
             L"Adjust the Red, Green, and Blue gain sliders until the wall light matches pure neutral white."),
        LStr(L"Ekrandaki %50 orta gri tonla LED parlaklığı dengeli görünene kadar Gamma sürgüsünü kaydır.",
             L"Slide Gamma until the LED brightness feels balanced against the 50% mid-gray reference."),
        LStr(L"Duvardaki renkler bu örnek görüntüyle canlı ve uyumlu mu? İyi olana kadar Parlaklık, Doygunluk ve Yumuşatma ayarla.",
             L"Do the wall colors match this sample scene vividly? Tune Brightness, Saturation, and Smoothing until satisfied.")
    };

    SelectObject(hdc, hFontHead);
    SetTextColor(hdc, CLR_TEXT);
    RECT tRc = {S(24), S(56), W - S(24), S(80)};
    DrawTextW(hdc, titles[st->step], -1, &tRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hFontUI);
    SetTextColor(hdc, CLR_TEXT2);
    RECT dRc = {S(24), S(82), W - S(24), S(118)};
    DrawTextW(hdc, descs[st->step], -1, &dRc, DT_LEFT | DT_WORDBREAK);

    // Reference Visual Box
    RECT box = {S(24), S(124), W - S(24), S(320)};
    SetDCBrushColor(hdc, CLR_CARD);
    SetDCPenColor(hdc, CLR_BORDER);
    RoundRect(hdc, box.left, box.top, box.right, box.bottom, S(12), S(12));

    RECT inner = {box.left + S(16), box.top + S(16), box.right - S(16), box.bottom - S(16)};
    if (st->step == 0) {
        int prim = g_wizardPrimColor.load();
        COLORREF refCol = (prim == 1) ? RGB(0, 255, 0) : ((prim == 2) ? RGB(0, 0, 255) : RGB(255, 0, 0));
        const wchar_t* pName = (prim == 1) ? LStr(L"REFERANS: SAF YEŞİL", L"REFERENCE: PURE GREEN")
                             : ((prim == 2) ? LStr(L"REFERANS: SAF MAVİ", L"REFERENCE: PURE BLUE")
                                            : LStr(L"REFERANS: SAF KIRMIZI", L"REFERENCE: PURE RED"));
        SetDCBrushColor(hdc, refCol);
        SetDCPenColor(hdc, CLR_BORDER);
        RoundRect(hdc, inner.left, inner.top, inner.right, inner.bottom, S(10), S(10));

        SelectObject(hdc, hFontHead);
        SetTextColor(hdc, (prim == 1) ? RGB(10, 20, 10) : RGB(255, 255, 255));
        DrawTextW(hdc, pName, -1, &inner, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else if (st->step == 1) {
        SetDCBrushColor(hdc, RGB(255, 255, 255));
        SetDCPenColor(hdc, CLR_BORDER);
        RoundRect(hdc, inner.left, inner.top, inner.right, inner.bottom, S(10), S(10));

        SelectObject(hdc, hFontHead);
        SetTextColor(hdc, RGB(40, 44, 52));
        DrawTextW(hdc, LStr(L"REFERANS: SAF NÖTR BEYAZ (#FFFFFF)", L"REFERENCE: PURE NEUTRAL WHITE (#FFFFFF)"),
                  -1, &inner, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else if (st->step == 2) {
        int midX = (inner.left + inner.right) / 2 - S(6);
        SetDCBrushColor(hdc, RGB(128, 128, 128));
        SetDCPenColor(hdc, CLR_BORDER);
        RoundRect(hdc, inner.left, inner.top, midX, inner.bottom, S(8), S(8));
        SelectObject(hdc, hFontSemi);
        SetTextColor(hdc, RGB(240, 240, 240));
        RECT lr = {inner.left, inner.top, midX, inner.bottom};
        DrawTextW(hdc, LStr(L"%50 Orta Gri", L"50% Mid Gray"), -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        int gx0 = midX + S(12), gx1 = inner.right;
        int gw = std::max(1, gx1 - gx0);
        for (int i = 0; i < gw; ++i) {
            uint8_t v = (uint8_t)((i * 255) / std::max(1, gw - 1));
            COLORREF gc = RGB(v, v, v);
            SetDCBrushColor(hdc, gc);
            SetDCPenColor(hdc, gc);
            Rectangle(hdc, gx0 + i, inner.top, gx0 + i + 1, inner.bottom);
        }
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        SetDCPenColor(hdc, CLR_BORDER);
        Rectangle(hdc, gx0, inner.top, gx1, inner.bottom);
        SelectObject(hdc, GetStockObject(DC_BRUSH));
    } else {
        // Step 3: Draw Sample Image + LED dots around it
        st->EnsureSceneDIB(hdc);
        int areaW = inner.right - inner.left;
        int areaH = inner.bottom - inner.top;
        int sw = (areaH * 16) / 9;
        int sh = areaH - S(24);
        sw = (sh * 16) / 9;
        if (sw > areaW - S(36)) {
            sw = areaW - S(36);
            sh = (sw * 9) / 16;
        }
        int sx = inner.left + (areaW - sw) / 2;
        int sy = inner.top + (areaH - sh) / 2;

        if (st->hSceneBmp) {
            HDC sdc = CreateCompatibleDC(hdc);
            HGDIOBJ oldS = SelectObject(sdc, st->hSceneBmp);
            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(hdc, sx, sy, sw, sh, sdc, 0, 0, st->sceneW, st->sceneH, SRCCOPY);
            SelectObject(sdc, oldS);
            DeleteDC(sdc);
        }

        // Draw live sample LEDs around the sample image
        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            s = g_settings;
        }
        std::vector<RGB3> colors;
        {
            std::lock_guard<std::mutex> lk(g_colors_mtx);
            colors = g_last_colors;
        }
        int L = s.ambi_left, T = s.ambi_top, R = s.ambi_right;
        int BL = s.ambi_bottom_left, BR = s.ambi_bottom_right;
        if (L + T + R + BL + BR <= 0) R = std::max(1, s.led_count);
        auto meta = BuildOrderedLayoutMeta(
            L, T, R, BL, BR, s.start_corner, s.direction, s.led_offset,
            std::clamp(s.edge_depth_pct, 2, 30) / 100.0f,
            std::clamp(s.bottom_gap_pct, 0, 65) / 100.0f);

        int ledOff = S(9);
        int ledR = S(4);
        for (size_t k = 0; k < meta.size(); ++k) {
            const Zone& z = meta[k].zone;
            int e = meta[k].edge;
            int cx = sx, cy = sy;
            if (e == 0)      { cx = sx - ledOff; cy = sy + (int)(((z.y0 + z.y1) * 0.5f) * sh); }
            else if (e == 1) { cx = sx + (int)(((z.x0 + z.x1) * 0.5f) * sw); cy = sy - ledOff; }
            else if (e == 2) { cx = sx + sw + ledOff; cy = sy + (int)(((z.y0 + z.y1) * 0.5f) * sh); }
            else             { cx = sx + (int)(((z.x0 + z.x1) * 0.5f) * sw); cy = sy + sh + ledOff; }

            RGB3 c = (k < colors.size()) ? colors[k] : RGB3{40, 40, 45};
            SetDCBrushColor(hdc, RGB(c.r, c.g, c.b));
            SetDCPenColor(hdc, RGB(35, 38, 46));
            Ellipse(hdc, cx - ledR, cy - ledR, cx + ledR, cy + ledR);
        }
    }

    // Controls container card
    SetDCBrushColor(hdc, CLR_CARD);
    SetDCPenColor(hdc, CLR_BORDER);
    RoundRect(hdc, S(24), S(332), W - S(24), H - S(64), S(12), S(12));
}

static inline LRESULT CALLBACK WizardWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = (WizardState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        auto* cs = (CREATESTRUCTW*)lp;
        st = (WizardState*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);
        ApplyWindowChrome(hwnd);

        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            s = g_settings;
        }
        st->backup = s;

        g_wizardPrimColor.store(0, std::memory_order_relaxed);
        g_wizardStep.store(0, std::memory_order_relaxed);
        g_wizardActive.store(true, std::memory_order_release);

        // Step 0 controls
        st->hLblOrder = WizLbl(hwnd, LStr(L"Kanal Sırası", L"Channel Order"), CLR_TEXT2);
        st->hComboOrder = CreateWindowExW(0, L"COMBOBOX", NULL,
                                          WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                          0, 0, 10, 180, hwnd, (HMENU)(INT_PTR)WIZ_COMBO_ORDER,
                                          GetModuleHandleW(NULL), NULL);
        SendMessageW(st->hComboOrder, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        ApplyDarkTheme(st->hComboOrder, false);
        const wchar_t* orders[] = {
            LStr(L"RGB (Varsayılan)", L"RGB (Default)"), L"GRB", L"BGR", L"RBG", L"GBR", L"BRG"
        };
        for (auto o : orders) SendMessageW(st->hComboOrder, CB_ADDSTRING, 0, (LPARAM)o);
        SendMessageW(st->hComboOrder, CB_SETCURSEL, std::clamp(s.color_order, 0, 5), 0);

        st->hBtnPrimR = WizBtn(hwnd, LStr(L"Saf Kırmızı", L"Pure Red"),   WIZ_BTN_PRIM_R);
        st->hBtnPrimG = WizBtn(hwnd, LStr(L"Saf Yeşil", L"Pure Green"),   WIZ_BTN_PRIM_G);
        st->hBtnPrimB = WizBtn(hwnd, LStr(L"Saf Mavi", L"Pure Blue"),     WIZ_BTN_PRIM_B);
        UpdateWizardPrimButtons(st);

        // Step 1 controls
        st->hLblR = WizLbl(hwnd, LStr(L"Kırmızı (R)", L"Red (R)"), CLR_TEXT2);
        st->hSliderR = WizSlider(hwnd, WIZ_SLIDER_R, 20, 100, s.gain_r);
        st->hValR = WizLbl(hwnd, L"100%", CLR_SUCCESS, hFontSemi);

        st->hLblG = WizLbl(hwnd, LStr(L"Yeşil (G)", L"Green (G)"), CLR_TEXT2);
        st->hSliderG = WizSlider(hwnd, WIZ_SLIDER_G, 20, 100, s.gain_g);
        st->hValG = WizLbl(hwnd, L"100%", CLR_SUCCESS, hFontSemi);

        st->hLblB = WizLbl(hwnd, LStr(L"Mavi (B)", L"Blue (B)"), CLR_TEXT2);
        st->hSliderB = WizSlider(hwnd, WIZ_SLIDER_B, 20, 100, s.gain_b);
        st->hValB = WizLbl(hwnd, L"100%", CLR_SUCCESS, hFontSemi);

        st->hBtnResetRGB = WizBtn(hwnd, LStr(L"%100 Sıfırla", L"Reset 100%"), WIZ_BTN_RESET_RGB);

        // Step 2 controls
        st->hLblGamma = WizLbl(hwnd, L"Gamma", CLR_TEXT2);
        st->hSliderGamma = WizSlider(hwnd, WIZ_SLIDER_GAMMA, 10, 32, std::clamp((int)roundf(s.gamma * 10.0f), 10, 32));
        st->hValGamma = WizLbl(hwnd, L"2.2", CLR_SUCCESS, hFontSemi);

        // Step 3 controls
        st->hLblBright = WizLbl(hwnd, LStr(L"Parlaklık", L"Brightness"), CLR_TEXT2);
        st->hSliderBright = WizSlider(hwnd, WIZ_SLIDER_BRIGHT, 0, 200, std::clamp(s.brightness, 0, 200));
        st->hValBright = WizLbl(hwnd, L"100%", CLR_SUCCESS, hFontSemi);

        st->hLblSat = WizLbl(hwnd, LStr(L"Doygunluk", L"Saturation"), CLR_TEXT2);
        st->hSliderSat = WizSlider(hwnd, WIZ_SLIDER_SAT, 5, 50, std::clamp((int)roundf(s.saturation * 10.0f), 5, 50));
        st->hValSat = WizLbl(hwnd, L"1.0x", CLR_SUCCESS, hFontSemi);

        st->hLblSmooth = WizLbl(hwnd, LStr(L"Yumuşatma", L"Smoothing"), CLR_TEXT2);
        st->hSliderSmooth = WizSlider(hwnd, WIZ_SLIDER_SMOOTH, 0, 95, std::clamp((int)roundf(s.smoothing * 100.0f), 0, 95));
        st->hValSmooth = WizLbl(hwnd, L"0.30", CLR_SUCCESS, hFontSemi);

        // Footer
        st->hBtnCancel = WizBtn(hwnd, LStr(L"İptal", L"Cancel"),    WIZ_BTN_CANCEL, CLR_CTRL_BG);
        st->hBtnBack   = WizBtn(hwnd, LStr(L"< Geri", L"< Back"),   WIZ_BTN_BACK,   CLR_CTRL_BG);
        st->hBtnNext   = WizBtn(hwnd, LStr(L"İleri >", L"Next >"),  WIZ_BTN_NEXT,   CLR_ACCENT);

        // Place controls
        RECT rc;
        GetClientRect(hwnd, &rc);
        int W = rc.right, H = rc.bottom;
        auto P = [](HWND h, int x, int y, int w, int ht) {
            if (h) SetWindowPos(h, NULL, x, y, w, ht, SWP_NOZORDER | SWP_NOACTIVATE);
        };

        int cx = S(40), cw = W - S(80);
        int cy = S(350);
        int lblW = S(100), valW = S(48), rowH = S(24);

        // Step 0 layout
        P(st->hLblOrder,   cx, cy + S(4), lblW, rowH);
        P(st->hComboOrder, cx + lblW, cy, S(200), rowH);
        P(st->hBtnPrimR,   cx,            cy + S(44), S(116), S(30));
        P(st->hBtnPrimG,   cx + S(124),   cy + S(44), S(116), S(30));
        P(st->hBtnPrimB,   cx + S(248),   cy + S(44), S(116), S(30));

        // Step 1 layout
        P(st->hLblR,    cx, cy + S(3),  lblW, rowH);
        P(st->hSliderR, cx + lblW, cy,  cw - lblW - valW - S(115), rowH);
        P(st->hValR,    cx + cw - valW - S(110), cy + S(3), valW, rowH);

        P(st->hLblG,    cx, cy + S(35), lblW, rowH);
        P(st->hSliderG, cx + lblW, cy + S(32), cw - lblW - valW - S(115), rowH);
        P(st->hValG,    cx + cw - valW - S(110), cy + S(35), valW, rowH);

        P(st->hLblB,    cx, cy + S(67), lblW, rowH);
        P(st->hSliderB, cx + lblW, cy + S(64), cw - lblW - valW - S(115), rowH);
        P(st->hValB,    cx + cw - valW - S(110), cy + S(67), valW, rowH);

        P(st->hBtnResetRGB, cx + cw - S(102), cy + S(30), S(102), S(30));

        // Step 2 layout
        P(st->hLblGamma,    cx, cy + S(32), lblW, rowH);
        P(st->hSliderGamma, cx + lblW, cy + S(29), cw - lblW - valW, rowH);
        P(st->hValGamma,    cx + cw - valW + S(4), cy + S(32), valW, rowH);

        // Step 3 layout
        int cy3 = S(343);
        P(st->hLblBright,    cx, cy3 + S(3),  lblW, rowH);
        P(st->hSliderBright, cx + lblW, cy3,  cw - lblW - valW, rowH);
        P(st->hValBright,    cx + cw - valW + S(4), cy3 + S(3), valW, rowH);

        P(st->hLblSat,    cx, cy3 + S(31), lblW, rowH);
        P(st->hSliderSat, cx + lblW, cy3 + S(28), cw - lblW - valW, rowH);
        P(st->hValSat,    cx + cw - valW + S(4), cy3 + S(31), valW, rowH);

        P(st->hLblSmooth,    cx, cy3 + S(59), lblW, rowH);
        P(st->hSliderSmooth, cx + lblW, cy3 + S(56), cw - lblW - valW, rowH);
        P(st->hValSmooth,    cx + cw - valW + S(4), cy3 + S(59), valW, rowH);

        // Footer
        P(st->hBtnCancel, S(24),         H - S(46), S(96),  S(32));
        P(st->hBtnBack,   W - S(278),    H - S(46), S(96),  S(32));
        P(st->hBtnNext,   W - S(174),    H - S(46), S(150), S(32));

        UpdateWizardLabels(st, s);
        ShowWizardStep(hwnd, st, 0);
        SetTimer(hwnd, 1, 33, NULL);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        if (!st) break;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        if (rc.right > 0 && rc.bottom > 0) {
            st->EnsureBackBuf(hdc, rc.right, rc.bottom);
            if (st->memDC) {
                PaintWizard(hwnd, st, st->memDC, rc.right, rc.bottom);
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, st->memDC, 0, 0, SRCCOPY);
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        HWND ctrl = (HWND)lp;
        SetBkMode(hdc, TRANSPARENT);
        COLORREF fg = (COLORREF)(UINT_PTR)GetPropW(ctrl, L"fg");
        SetTextColor(hdc, fg ? fg : CLR_TEXT);
        return (LRESULT)hBrCard;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, CLR_CTRL_BG);
        SetTextColor(hdc, CLR_TEXT);
        return (LRESULT)hBrCtrl;
    }

    case WM_DRAWITEM:
        DrawModernButton((DRAWITEMSTRUCT*)lp);
        return TRUE;

    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)lp;
        if (nm && nm->code == NM_CUSTOMDRAW) {
            int id = (int)nm->idFrom;
            if (id == WIZ_SLIDER_R || id == WIZ_SLIDER_G || id == WIZ_SLIDER_B ||
                id == WIZ_SLIDER_GAMMA || id == WIZ_SLIDER_BRIGHT ||
                id == WIZ_SLIDER_SAT || id == WIZ_SLIDER_SMOOTH) {
                return HandleTrackbarCustomDraw((LPNMCUSTOMDRAW)lp);
            }
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (!st) break;
        int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int pillW = (rc.right - S(48) - S(24)) / 4;
        int py = S(14), pillH = S(30);
        if (my >= py && my <= py + pillH) {
            for (int i = 0; i < 4; ++i) {
                int px = S(24) + i * (pillW + S(8));
                if (mx >= px && mx <= px + pillW) {
                    ShowWizardStep(hwnd, st, i);
                    return 0;
                }
            }
        }
        return 0;
    }

    case WM_HSCROLL: {
        if (!st) break;
        HWND sl = (HWND)lp;
        int val = (int)SendMessageW(sl, TBM_GETPOS, 0, 0);
        Settings sCopy;
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            if (sl == st->hSliderR)           g_settings.gain_r = val;
            else if (sl == st->hSliderG)      g_settings.gain_g = val;
            else if (sl == st->hSliderB)      g_settings.gain_b = val;
            else if (sl == st->hSliderGamma)  g_settings.gamma  = val / 10.0f;
            else if (sl == st->hSliderBright) g_settings.brightness = val;
            else if (sl == st->hSliderSat)    g_settings.saturation = val / 10.0f;
            else if (sl == st->hSliderSmooth) g_settings.smoothing  = val / 100.0f;
            sCopy = g_settings;
        }
        UpdateWizardLabels(st, sCopy);
        return 0;
    }

    case WM_COMMAND: {
        if (!st) break;
        int id = LOWORD(wp);
        int notif = HIWORD(wp);

        if (id == WIZ_COMBO_ORDER && notif == CBN_SELCHANGE) {
            int sel = std::clamp((int)SendMessageW(st->hComboOrder, CB_GETCURSEL, 0, 0), 0, 5);
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            g_settings.color_order = sel;
        } else if (id == WIZ_BTN_PRIM_R || id == WIZ_BTN_PRIM_G || id == WIZ_BTN_PRIM_B) {
            int p = (id == WIZ_BTN_PRIM_G) ? 1 : ((id == WIZ_BTN_PRIM_B) ? 2 : 0);
            g_wizardPrimColor.store(p, std::memory_order_relaxed);
            UpdateWizardPrimButtons(st);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == WIZ_BTN_RESET_RGB) {
            Settings sCopy;
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.gain_r = 100;
                g_settings.gain_g = 100;
                g_settings.gain_b = 100;
                sCopy = g_settings;
            }
            SendMessageW(st->hSliderR, TBM_SETPOS, TRUE, 100);
            SendMessageW(st->hSliderG, TBM_SETPOS, TRUE, 100);
            SendMessageW(st->hSliderB, TBM_SETPOS, TRUE, 100);
            UpdateWizardLabels(st, sCopy);
        } else if (id == WIZ_BTN_BACK) {
            ShowWizardStep(hwnd, st, st->step - 1);
        } else if (id == WIZ_BTN_NEXT) {
            if (st->step < 3) {
                ShowWizardStep(hwnd, st, st->step + 1);
            } else {
                st->saved = true;
                g_wizardActive.store(false, std::memory_order_release);
                Settings sCopy;
                {
                    std::lock_guard<std::mutex> lk(g_settings_mtx);
                    sCopy = g_settings;
                    MarkDirty();
                }
                SaveSettings(sCopy, true);
                SyncControlsFromSettings();
                DestroyWindow(hwnd);
            }
        } else if (id == WIZ_BTN_CANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_TIMER:
        if (wp == 1 && st && st->step == 3) {
            RECT r = {S(24), S(124), S(640), S(320)};
            InvalidateRect(hwnd, &r, FALSE);
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        g_wizardActive.store(false, std::memory_order_release);
        if (st) {
            if (!st->saved) {
                {
                    std::lock_guard<std::mutex> lk(g_settings_mtx);
                    g_settings = st->backup;
                }
                SyncControlsFromSettings();
            }
            st->Release();
            if (st->parent) {
                EnableWindow(st->parent, TRUE);
                SetForegroundWindow(st->parent);
            }
        }
        return 0;

    case WM_NCDESTROY:
        if (st) {
            delete st;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static inline void OpenColorWizard(HWND parent) {
    auto* st = new WizardState();
    st->parent = parent;

    RECT pr = {};
    GetWindowRect(parent, &pr);
    int ww = S(660), wh = S(540);
    int wx = pr.left + (pr.right - pr.left - ww) / 2;
    int wy = pr.top + (pr.bottom - pr.top - wh) / 2;

    HWND wiz = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"ETDAmbilightWizard",
        LStr(L"ETDAmbilight — Renk Kalibrasyon Sihirbazı", L"ETDAmbilight — Color Calibration Wizard"),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        wx, wy, ww, wh,
        parent, NULL, GetModuleHandleW(NULL), st);

    if (wiz) {
        EnableWindow(parent, FALSE);
        ShowWindow(wiz, SW_SHOW);
        UpdateWindow(wiz);
    } else {
        delete st;
    }
}
