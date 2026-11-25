#include "../src/common.h"
#include "../src/i18n.h"
#include "../src/settings.h"
#include "../src/serial.h"
#include "../src/theme.h"
#include "../src/updater.h"

#include <shlobj.h>
#include <shobjidl.h>

// ─────────────────────────── CONSTANTS & IDS ────────────────────────────────
static const wchar_t* UNINST_KEY        = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ETDAmbilight";
static const wchar_t* LEGACY_UNINST_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\LEDci";

#define WM_SETUP_PROGRESS (WM_USER + 10)
#define WM_SETUP_DONE     (WM_USER + 11)

enum SetupCtrlId {
    ID_BTN_LANG = 5001,
    ID_BTN_CANCEL,
    ID_BTN_BACK,
    ID_BTN_NEXT,

    // Step 0
    ID_EDIT_LICENSE,
    ID_TGL_ACCEPT,

    // Step 1
    ID_TGL_CH340,
    ID_BTN_RESCAN,

    // Step 2
    ID_EDIT_PATH,
    ID_BTN_BROWSE,
    ID_TGL_DESKTOP,
    ID_TGL_STARTMENU,
    ID_TGL_AUTOSTART,

    // Step 3
    ID_EDIT_LOG,
    ID_TGL_LAUNCH,

    // Uninstall mode
    ID_TGL_DEL_SETTINGS,
    ID_BTN_UNINST_RUN,
};

struct SetupState {
    int  step = 0; // 0=Welcome, 1=Reqs, 2=Options, 3=Install/Finish
    bool isUpdate = false;
    bool dxgiOk = false;
    bool ch340Installed = false;
    bool installing = false;
    bool finished = false;
    bool installOk = false;
    int  progress = 0;

    std::wstring existingDir;
    std::wstring existingExe;
    std::wstring installDir;

    Settings existingSettings;
    bool hasExistingSettings = false;

    HICON hIconBig = NULL;
    HICON hIconSm  = NULL;

    // Backbuffer
    HDC     memDC = NULL;
    HBITMAP hbm   = NULL;
    HGDIOBJ oldHbm = NULL;
    int     bufW = 0, bufH = 0;

    // Controls
    HWND hBtnLang = NULL;
    HWND hBtnCancel = NULL, hBtnBack = NULL, hBtnNext = NULL;

    HWND hEditLic = NULL, hTglAccept = NULL;
    HWND hTglCh340 = NULL, hBtnRescan = NULL;
    HWND hLblPath = NULL, hEditPath = NULL, hBtnBrowse = NULL;
    HWND hTglDesktop = NULL, hTglStartMenu = NULL, hTglAutostart = NULL;
    HWND hEditLog = NULL, hTglLaunch = NULL;

    void EnsureBackBuf(HDC ref, int w, int h) {
        if (w <= 0 || h <= 0) return;
        if (memDC && hbm && bufW == w && bufH == h) return;
        ReleaseBackBuf();
        memDC = CreateCompatibleDC(ref);
        hbm = CreateCompatibleBitmap(ref, w, h);
        if (memDC && hbm) oldHbm = SelectObject(memDC, hbm);
        bufW = w;
        bufH = h;
    }

    void ReleaseBackBuf() {
        if (memDC && oldHbm) { SelectObject(memDC, oldHbm); oldHbm = NULL; }
        if (hbm)   { DeleteObject(hbm); hbm = NULL; }
        if (memDC) { DeleteDC(memDC); memDC = NULL; }
        bufW = bufH = 0;
    }
};

static SetupState g_st;

// ─────────────────────────── HELPERS ────────────────────────────────────────
static inline bool FileExistsW(const std::wstring& p) {
    DWORD attr = GetFileAttributesW(p.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

static inline bool DirExistsW(const std::wstring& p) {
    DWORD attr = GetFileAttributesW(p.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

static inline void EnsureDirRecursive(const std::wstring& path) {
    if (path.empty() || DirExistsW(path)) return;
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos && pos > 2) {
        EnsureDirRecursive(path.substr(0, pos));
    }
    CreateDirectoryW(path.c_str(), NULL);
}

static inline std::wstring DefaultInstallDir() {
    wchar_t buf[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buf))) {
        return std::wstring(buf) + L"\\Programs\\ETDAmbilight";
    }
    return L"C:\\ETDAmbilight";
}

static inline std::wstring ReadRegString(HKEY root, const wchar_t* subkey, const wchar_t* valName) {
    HKEY hk = NULL;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hk) != ERROR_SUCCESS) return L"";
    wchar_t buf[1024] = {};
    DWORD sz = sizeof(buf), type = 0;
    std::wstring out;
    if (RegQueryValueExW(hk, valName, NULL, &type, (BYTE*)buf, &sz) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ)) {
        out = buf;
    }
    RegCloseKey(hk);
    return out;
}

static inline void DetectExistingInstallation(SetupState& st) {
    std::vector<std::wstring> candidateDirs;
    std::wstring loc1 = ReadRegString(HKEY_CURRENT_USER, UNINST_KEY, L"InstallLocation");
    if (!loc1.empty()) candidateDirs.push_back(loc1);

    std::wstring loc2 = ReadRegString(HKEY_CURRENT_USER, LEGACY_UNINST_KEY, L"InstallLocation");
    if (!loc2.empty()) candidateDirs.push_back(loc2);

    candidateDirs.push_back(DefaultInstallDir());
    wchar_t localApp[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localApp))) {
        candidateDirs.push_back(std::wstring(localApp) + L"\\Programs\\LEDci");
    }

    for (const auto& dir : candidateDirs) {
        std::wstring d = dir;
        while (!d.empty() && (d.back() == L'\\' || d.back() == L'/')) d.pop_back();
        if (d.empty()) continue;

        std::wstring e1 = d + L"\\ETDAmbilight.exe";
        std::wstring e2 = d + L"\\LEDci.exe";
        if (FileExistsW(e1)) {
            st.isUpdate = true;
            st.existingDir = d;
            st.existingExe = e1;
            break;
        }
        if (FileExistsW(e2)) {
            st.isUpdate = true;
            st.existingDir = d;
            st.existingExe = e2;
            break;
        }
    }

    st.installDir = st.isUpdate ? st.existingDir : DefaultInstallDir();
    // If upgrading from legacy .../Programs/LEDci, prefer .../Programs/ETDAmbilight unless user wants old path
    if (st.isUpdate && st.existingDir.size() >= 6 &&
        _wcsicmp(st.existingDir.c_str() + st.existingDir.size() - 6, L"\\LEDci") == 0) {
        st.installDir = DefaultInstallDir();
    }

    // Load existing settings (Registry first, then EXE footer overlay so latest edits win)
    Settings tmp;
    bool regOk = LoadSettingsFromRegistryKey(REG_KEY, tmp);
    if (!regOk) regOk = LoadSettingsFromRegistryKey(LEGACY_REG_KEY, tmp);
    bool exeOk = (!st.existingExe.empty()) && LoadSettingsFromExePath(st.existingExe.c_str(), tmp);

    st.hasExistingSettings = regOk || exeOk;
    if (st.hasExistingSettings) {
        st.existingSettings = tmp;
    }
}

static inline void ScanSystemRequirements(SetupState& st) {
    // 1. DXGI / D3D11 check
    HMODULE hDxgi = LoadLibraryW(L"dxgi.dll");
    HMODULE hD3d  = LoadLibraryW(L"d3d11.dll");
    st.dxgiOk = (hDxgi != NULL && hD3d != NULL);
    if (hDxgi) FreeLibrary(hDxgi);
    if (hD3d)  FreeLibrary(hD3d);

    // 2. CH340 / CH341 driver detection
    bool found = false;
    // Check known kernel services
    const wchar_t* svcKeys[] = {
        L"SYSTEM\\CurrentControlSet\\Services\\CH341SER_A64",
        L"SYSTEM\\CurrentControlSet\\Services\\CH341S64",
        L"SYSTEM\\CurrentControlSet\\Services\\CH341SER",
        L"SYSTEM\\CurrentControlSet\\Services\\CH340"
    };
    for (auto k : svcKeys) {
        HKEY hk = NULL;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, k, 0, KEY_READ, &hk) == ERROR_SUCCESS) {
            RegCloseKey(hk);
            found = true;
            break;
        }
    }
    // Check DriverStore FileRepository
    if (!found) {
        wchar_t winDir[MAX_PATH] = {};
        GetWindowsDirectoryW(winDir, MAX_PATH);
        std::wstring pat = std::wstring(winDir) + L"\\System32\\DriverStore\\FileRepository\\ch34*";
        WIN32_FIND_DATAW fd = {};
        HANDLE hf = FindFirstFileW(pat.c_str(), &fd);
        if (hf != INVALID_HANDLE_VALUE) {
            found = true;
            FindClose(hf);
        }
    }
    // Check active COM ports
    if (!found) {
        auto ports = ScanPorts();
        for (const auto& p : ports) {
            std::wstring dl = p.desc;
            std::transform(dl.begin(), dl.end(), dl.begin(), ::towlower);
            if (dl.find(L"ch340") != std::wstring::npos || dl.find(L"ch341") != std::wstring::npos) {
                found = true;
                break;
            }
        }
    }
    st.ch340Installed = found;
}

static inline bool CreateShortcutLnk(const std::wstring& lnkPath, const std::wstring& targetPath,
                                     const std::wstring& workDir, const std::wstring& desc) {
    IShellLinkW* psl = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IShellLinkW, (void**)&psl);
    if (FAILED(hr) || !psl) return false;

    psl->SetPath(targetPath.c_str());
    psl->SetWorkingDirectory(workDir.c_str());
    psl->SetDescription(desc.c_str());
    psl->SetIconLocation(targetPath.c_str(), 0);

    IPersistFile* ppf = nullptr;
    bool ok = false;
    if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
        ok = SUCCEEDED(ppf->Save(lnkPath.c_str(), TRUE));
        ppf->Release();
    }
    psl->Release();
    return ok;
}

static inline void CloseRunningAppInstances() {
    const wchar_t* classes[] = {L"ETDAmbilightMain", L"LEDciMain"};
    for (auto cls : classes) {
        for (int attempt = 0; attempt < 5; ++attempt) {
            HWND hApp = FindWindowW(cls, NULL);
            if (!hApp) break;
            DWORD_PTR res = 0;
            SendMessageTimeoutW(hApp, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG, 1000, &res);
            // If close_to_tray was enabled, WM_CLOSE hides instead of exiting; send WM_COMMAND ID_TRAY_EXIT (3003)
            PostMessageW(hApp, WM_COMMAND, 3003, 0);
            Sleep(350);
        }
    }
}

static inline bool ExtractPayloadExe(const std::wstring& destPath) {
    HMODULE hMod = GetModuleHandleW(NULL);
    HRSRC hRes = FindResourceW(hMod, L"APP_PAYLOAD", RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hMem = LoadResource(hMod, hRes);
    if (!hMem) return false;
    DWORD sz = SizeofResource(hMod, hRes);
    const void* ptr = LockResource(hMem);
    if (!ptr || sz == 0) return false;

    std::wstring tmp = destPath + L".tmp";
    HANDLE hf = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(hf, ptr, sz, &written, NULL) && written == sz;
    CloseHandle(hf);
    if (!ok) { DeleteFileW(tmp.c_str()); return false; }

    if (!MoveFileExW(tmp.c_str(), destPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(tmp.c_str());
        return false;
    }
    return true;
}

static inline void SetRegSz(HKEY root, const wchar_t* subkey, const wchar_t* name, const std::wstring& val) {
    HKEY hk = NULL;
    if (RegCreateKeyExW(root, subkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hk, name, 0, REG_SZ, (const BYTE*)val.c_str(), (DWORD)((val.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hk);
    }
}

static inline HWND MakeSetupLbl(HWND parent, const wchar_t* txt, COLORREF fg = CLR_TEXT, HFONT f = NULL) {
    HWND h = CreateWindowExW(0, L"STATIC", txt, WS_CHILD | SS_LEFT | SS_NOPREFIX,
                             0, 0, 10, 10, parent, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)(f ? f : hFontUI), TRUE);
    SetPropW(h, L"fg", (HANDLE)(UINT_PTR)fg);
    return h;
}

static inline HWND MakeSetupBtn(HWND parent, const wchar_t* txt, int id, COLORREF bg = CLR_CTRL_BG, bool topbar = false) {
    HWND h = CreateWindowExW(0, L"BUTTON", txt, WS_CHILD | BS_OWNERDRAW,
                             0, 0, 10, 10, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SetBtnBg(h, bg);
    if (topbar) SetPropW(h, L"in_topbar", (HANDLE)1);
    AttachHoverSubclass(h);
    return h;
}

static inline HWND MakeSetupToggle(HWND parent, const wchar_t* txt, int id, bool on) {
    HWND h = CreateWindowExW(0, L"BUTTON", txt, WS_CHILD | BS_OWNERDRAW,
                             0, 0, 10, 10, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SetToggleState(h, on);
    AttachHoverSubclass(h);
    return h;
}

static inline std::wstring LicenseSummaryText() {
    if (CurrentLang() == 1) {
        return
            L"ETDAmbilight v2.0 — Independent Developer Software\r\n"
            L"Developer & Rights Holder: Emir Tuğra Dağ\r\n\r\n"
            L"1. MIT CODE LICENSE (Source Code):\r\n"
            L"Copyright (c) 2026 Emir Tuğra Dağ\r\n"
            L"Source code and runtime logic are open source under the standard MIT License.\r\n\r\n"
            L"2. BRAND & LOGO IP POLICY (Excluded from MIT):\r\n"
            L"The project name, official logos, and icons are NOT covered by the MIT License. "
            L"Intellectual property rights belong exclusively to Emir Tuğra Dağ. Forks/derivatives must replace brand names and icons.\r\n\r\n"
            L"3. ABSOLUTE DISCLAIMER OF LIABILITY (AS IS):\r\n"
            L"Provided \"AS IS\", without warranty of any kind. Emir Tuğra Dağ is not liable for any data loss, "
            L"system issues, hardware/LED strip damage, or any direct or indirect damages. All usage risk is yours.";
    }
    return
        L"ETDAmbilight v2.0 — Bağımsız Geliştirici Yazılımı\r\n"
        L"Geliştirici & Hak Sahibi: Emir Tuğra Dağ\r\n\r\n"
        L"1. KAYNAK KOD LİSANSI (MIT Lisansı):\r\n"
        L"Copyright (c) 2026 Emir Tuğra Dağ\r\n"
        L"Tüm kaynak kodları standart MIT Lisansı altında açık kaynaklıdır.\r\n\r\n"
        L"2. MARKA, LOGO VE FİKRİ MÜLKİYET (MIT Kapsamı Dışında):\r\n"
        L"Proje ismi, resmi logolar ve uygulama simgeleri MIT Lisansı kapsamı dışındadır. "
        L"Fikri mülkiyet hakkı Emir Tuğra Dağ'a aittir. Türev projelerde değiştirilmesi zorunludur.\r\n\r\n"
        L"3. EKSİKSİZ SORUMLULUK REDDİ (AS IS):\r\n"
        L"Yazılım \"olduğu gibi\" ve hiçbir garanti olmaksızın sunulur. Veri kaybı, sistem/donanım hasarı veya "
        L"doğrudan/dolaylı hiçbir zarardan Emir Tuğra Dağ sorumlu tutulamaz. Tüm kullanım riski kullanıcıya aittir.";
}

// ─────────────────────────── WIZARD TEXT & STEP SYNC ────────────────────────
static inline void UpdateSetupTexts(HWND hwnd) {
    SetWindowTextW(hwnd, LStr(L"ETDAmbilight v2.0 — Kurulum Sihirbazı",
                              L"ETDAmbilight v2.0 — Setup Wizard"));
    SetWindowTextW(g_st.hBtnLang, CurrentLang() == 1 ? L"TR | [EN]" : L"[TR] | EN");
    SetWindowTextW(g_st.hEditLic, LicenseSummaryText().c_str());
    SetWindowTextW(g_st.hTglAccept, LStr(L"Lisans ve sorumluluk reddi koşullarını okudum, kabul ediyorum",
                                         L"I accept the license and disclaimer terms"));
    SetWindowTextW(g_st.hTglCh340, LStr(L"CH340 USB-Seri sürücüsünü indir/kur (Klon Arduino için)",
                                        L"Download/install CH340 USB-Serial driver (clone Arduinos)"));
    SetWindowTextW(g_st.hBtnRescan, LStr(L"Yeniden Tara", L"Re-scan"));
    SetWindowTextW(g_st.hLblPath, LStr(L"Kurulum Klasörü", L"Install Folder"));
    SetWindowTextW(g_st.hBtnBrowse, LStr(L"Gözat...", L"Browse..."));
    SetWindowTextW(g_st.hTglDesktop, LStr(L"Masaüstü kısayolu oluştur", L"Create Desktop shortcut"));
    SetWindowTextW(g_st.hTglStartMenu, LStr(L"Başlat Menüsü kısayolu oluştur", L"Create Start Menu shortcut"));
    SetWindowTextW(g_st.hTglAutostart, LStr(L"Windows ile otomatik başlat", L"Start automatically with Windows"));
    SetWindowTextW(g_st.hTglLaunch, LStr(L"ETDAmbilight'ı şimdi başlat", L"Launch ETDAmbilight now"));
    SetWindowTextW(g_st.hBtnCancel, LStr(L"İptal", L"Cancel"));
    SetWindowTextW(g_st.hBtnBack, LStr(L"< Geri", L"< Back"));

    if (g_st.step == 2) {
        SetWindowTextW(g_st.hBtnNext,
                       g_st.isUpdate ? LStr(L"Güncelle", L"Update") : LStr(L"Kur", L"Install"));
    } else if (g_st.step == 3) {
        SetWindowTextW(g_st.hBtnNext, LStr(L"Bitir", L"Finish"));
    } else {
        SetWindowTextW(g_st.hBtnNext, LStr(L"İleri >", L"Next >"));
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

static inline void ShowSetupStep(HWND hwnd, int step) {
    g_st.step = std::clamp(step, 0, 3);
    auto Show = [](HWND h, bool v) { if (h) ShowWindow(h, v ? SW_SHOW : SW_HIDE); };

    Show(g_st.hEditLic,      g_st.step == 0);
    Show(g_st.hTglAccept,    g_st.step == 0);
    Show(g_st.hTglCh340,     g_st.step == 1);
    Show(g_st.hBtnRescan,    g_st.step == 1);
    Show(g_st.hLblPath,      g_st.step == 2);
    Show(g_st.hEditPath,     g_st.step == 2);
    Show(g_st.hBtnBrowse,    g_st.step == 2);
    Show(g_st.hTglDesktop,   g_st.step == 2);
    Show(g_st.hTglStartMenu, g_st.step == 2);
    Show(g_st.hTglAutostart, g_st.step == 2);
    Show(g_st.hEditLog,      g_st.step == 3);
    Show(g_st.hTglLaunch,    g_st.step == 3 && g_st.finished && g_st.installOk);

    Show(g_st.hBtnCancel, !g_st.finished);
    Show(g_st.hBtnBack,   g_st.step > 0 && g_st.step < 3);
    Show(g_st.hBtnNext,   true);

    EnableWindow(g_st.hBtnCancel, !g_st.installing);
    EnableWindow(g_st.hBtnNext,   !g_st.installing);
    SetBtnBg(g_st.hBtnNext, (g_st.step >= 2) ? RGB(38, 158, 108) : CLR_ACCENT);

    UpdateSetupTexts(hwnd);
}

static inline void AppendLogLine(HWND hEdit, const std::wstring& line) {
    if (!hEdit) return;
    int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, len, len);
    std::wstring s = line + L"\r\n";
    SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
}

// ─────────────────────────── INSTALL WORKER ─────────────────────────────────
struct InstallJob {
    HWND         hwnd;
    std::wstring targetDir;
    bool         doDesktop;
    bool         doStartMenu;
    bool         doAutostart;
    bool         doCh340;
    int          lang;
};

static inline void PostStepLog(HWND hwnd, int pct, const wchar_t* tr, const wchar_t* en) {
    auto* msg = new std::wstring(LStr(tr, en));
    PostMessageW(hwnd, WM_SETUP_PROGRESS, (WPARAM)pct, (LPARAM)msg);
}

static inline void RunInstallThread(InstallJob job) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    PostStepLog(job.hwnd, 10,
                L"[1/6] Çalışan örnekler kontrol ediliyor...",
                L"[1/6] Checking for running instances...");
    CloseRunningAppInstances();

    PostStepLog(job.hwnd, 25,
                L"[2/6] Mevcut ayarlar korunuyor ve aktarılıyor...",
                L"[2/6] Preserving and migrating existing settings...");
    Settings s;
    bool foundAny = false;
    if (LoadSettingsFromRegistryKey(REG_KEY, s)) foundAny = true;
    else if (LoadSettingsFromRegistryKey(LEGACY_REG_KEY, s)) foundAny = true;

    std::wstring prevExe1 = job.targetDir + L"\\ETDAmbilight.exe";
    std::wstring prevExe2 = job.targetDir + L"\\LEDci.exe";
    if (FileExistsW(prevExe1) && LoadSettingsFromExePath(prevExe1.c_str(), s)) foundAny = true;
    else if (FileExistsW(prevExe2) && LoadSettingsFromExePath(prevExe2.c_str(), s)) foundAny = true;
    else if (!g_st.existingExe.empty() && LoadSettingsFromExePath(g_st.existingExe.c_str(), s)) foundAny = true;

    if (!foundAny) {
        s.lang = job.lang;
    } else if (s.lang != 0 && s.lang != 1) {
        s.lang = job.lang;
    }
    s.autostart = job.doAutostart;
    SaveSettingsToRegistry(s);

    PostStepLog(job.hwnd, 45,
                L"[3/6] ETDAmbilight.exe kuruluyor...",
                L"[3/6] Installing ETDAmbilight.exe...");
    EnsureDirRecursive(job.targetDir);
    std::wstring exeOut = job.targetDir + L"\\ETDAmbilight.exe";
    bool ok = ExtractPayloadExe(exeOut);
    if (!ok) {
        PostStepLog(job.hwnd, 45,
                    L"HATA: ETDAmbilight.exe yazılamadı. Klasör yazma iznini kontrol edin.",
                    L"ERROR: Failed to write ETDAmbilight.exe. Check folder permissions.");
        CoUninitialize();
        PostMessageW(job.hwnd, WM_SETUP_DONE, 0, 0);
        return;
    }

    // Re-embed preserved settings into the newly installed EXE footer
    SaveSettingsToExePath(exeOut.c_str(), s);

    PostStepLog(job.hwnd, 62,
                L"[4/6] Kaldırıcı ve Windows kaydı oluşturuluyor...",
                L"[4/6] Creating Uninstaller and Windows registration...");
    wchar_t selfPath[MAX_PATH] = {};
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);
    std::wstring uninstExe = job.targetDir + L"\\Uninstall.exe";
    CopyFileW(selfPath, uninstExe.c_str(), FALSE);

    SetRegSz(HKEY_CURRENT_USER, UNINST_KEY, L"DisplayName", L"ETDAmbilight");
    SetRegSz(HKEY_CURRENT_USER, UNINST_KEY, L"DisplayVersion", APP_VER);
    SetRegSz(HKEY_CURRENT_USER, UNINST_KEY, L"Publisher", APP_AUTHOR);
    SetRegSz(HKEY_CURRENT_USER, UNINST_KEY, L"InstallLocation", job.targetDir);
    SetRegSz(HKEY_CURRENT_USER, UNINST_KEY, L"DisplayIcon", exeOut + L",0");
    SetRegSz(HKEY_CURRENT_USER, UNINST_KEY, L"UninstallString", L"\"" + uninstExe + L"\" --uninstall");
    SetRegSz(HKEY_CURRENT_USER, UNINST_KEY, L"URLInfoAbout", L"https://github.com/emirtugra-dag/ETDAmbilight");

    // Autostart registry
    HKEY hRun = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_REG_KEY, 0, KEY_SET_VALUE, &hRun) == ERROR_SUCCESS) {
        RegDeleteValueW(hRun, L"LEDci");
        if (job.doAutostart) {
            std::wstring cmd = L"\"" + exeOut + L"\" --autostart";
            RegSetValueExW(hRun, APP_NAME, 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hRun, APP_NAME);
        }
        RegCloseKey(hRun);
    }

    PostStepLog(job.hwnd, 78,
                L"[5/6] Kısayollar oluşturuluyor...",
                L"[5/6] Creating shortcuts...");
    if (job.doDesktop) {
        wchar_t desk[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desk))) {
            CreateShortcutLnk(std::wstring(desk) + L"\\ETDAmbilight.lnk", exeOut, job.targetDir,
                              L"ETDAmbilight — Adalight Ambilight Controller");
        }
    }
    if (job.doStartMenu) {
        wchar_t sm[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, sm))) {
            std::wstring smDir = std::wstring(sm) + L"\\ETDAmbilight";
            EnsureDirRecursive(smDir);
            CreateShortcutLnk(smDir + L"\\ETDAmbilight.lnk", exeOut, job.targetDir,
                              L"ETDAmbilight — Adalight Ambilight Controller");
            CreateShortcutLnk(smDir + L"\\Uninstall ETDAmbilight.lnk", uninstExe, job.targetDir,
                              L"Uninstall ETDAmbilight");
        }
    }

    if (job.doCh340) {
        PostStepLog(job.hwnd, 90,
                    L"[6/6] CH340 USB-Seri sürücüsü indiriliyor...",
                    L"[6/6] Downloading CH340 USB-Serial driver...");
        std::string drvBin;
        bool drvOk = WinHttpFetchBytes(L"https://www.wch-ic.com/downloads/file/65.html", drvBin) &&
                     drvBin.size() > 65536 && drvBin[0] == 'M' && drvBin[1] == 'Z';
        if (drvOk) {
            wchar_t tmpDir[MAX_PATH] = {};
            GetTempPathW(MAX_PATH, tmpDir);
            std::wstring drvPath = std::wstring(tmpDir) + L"CH341SER.EXE";
            HANDLE hf = CreateFileW(drvPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hf != INVALID_HANDLE_VALUE) {
                DWORD wr = 0;
                WriteFile(hf, drvBin.data(), (DWORD)drvBin.size(), &wr, NULL);
                CloseHandle(hf);
                ShellExecuteW(NULL, L"open", drvPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        } else {
            PostStepLog(job.hwnd, 92,
                        L"Resmi WCH CH341SER sürücü indirme sayfası tarayıcıda açılıyor...",
                        L"Opening official WCH CH341SER driver download page in browser...");
            ShellExecuteW(NULL, L"open", L"https://www.wch-ic.com/downloads/CH341SER_EXE.html",
                          NULL, NULL, SW_SHOWNORMAL);
        }
    } else {
        PostStepLog(job.hwnd, 95,
                    L"[6/6] Gereklilik kontrolü tamamlandı.",
                    L"[6/6] Requirements verified.");
    }

    PostStepLog(job.hwnd, 100,
                L"Kurulum başarıyla tamamlandı! Tüm ayarlarınız hazır.",
                L"Setup completed successfully! All your settings are ready.");
    CoUninitialize();
    PostMessageW(job.hwnd, WM_SETUP_DONE, 1, 0);
}

// ─────────────────────────── PAINT WIZARD ───────────────────────────────────
static inline void PaintSetupWindow(HWND hwnd, HDC hdc, int W, int H) {
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));

    SetDCBrushColor(hdc, CLR_BG);
    SetDCPenColor(hdc, CLR_BG);
    Rectangle(hdc, 0, 0, W, H);

    // Header
    int hdrH = S(64);
    if (g_st.hIconBig) {
        DrawIconEx(hdc, S(20), S(12), g_st.hIconBig, S(40), S(40), 0, NULL, DI_NORMAL);
    }
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hFontTitle);
    SetTextColor(hdc, CLR_TEXT);
    RECT tRc = {S(70), S(10), W - S(120), S(36)};
    DrawTextW(hdc, L"ETDAmbilight", -1, &tRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, CLR_TEXT2);
    RECT subRc = {S(70), S(35), W - S(120), S(54)};
    DrawTextW(hdc, LStr(L"Sürüm 2.0  •  Geliştirici: Emir Tuğra Dağ",
                        L"Version 2.0  •  Developer: Emir Tuğra Dağ"),
              -1, &subRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetDCBrushColor(hdc, CLR_BORDER);
    SetDCPenColor(hdc, CLR_BORDER);
    Rectangle(hdc, 0, hdrH - 1, W, hdrH);

    // Step pills
    const wchar_t* pills[4] = {
        LStr(L"1. Hoş Geldiniz", L"1. Welcome"),
        LStr(L"2. Gereklilikler", L"2. Requirements"),
        LStr(L"3. Konum & Seçenek", L"3. Location"),
        LStr(L"4. Kurulum", L"4. Install")
    };
    int pw = (W - S(40) - S(24)) / 4;
    int py = hdrH + S(12), ph = S(28);
    SelectObject(hdc, hFontSemi);
    for (int i = 0; i < 4; ++i) {
        int px = S(20) + i * (pw + S(8));
        bool act = (i == g_st.step);
        bool done = (i < g_st.step) || (i == 3 && g_st.finished && g_st.installOk);
        COLORREF pbg = act ? CLR_ACCENT : (done ? RGB(38, 72, 58) : CLR_CARD);
        COLORREF pbd = act ? CLR_ACCENT : (done ? CLR_SUCCESS : CLR_BORDER);
        SetDCBrushColor(hdc, pbg);
        SetDCPenColor(hdc, pbd);
        RoundRect(hdc, px, py, px + pw, py + ph, S(8), S(8));
        SetTextColor(hdc, act ? RGB(255, 255, 255) : (done ? CLR_SUCCESS : CLR_TEXT2));
        RECT pr = {px, py, px + pw, py + ph};
        DrawTextW(hdc, pills[i], -1, &pr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Content card
    RECT card = {S(20), py + ph + S(12), W - S(20), H - S(60)};
    SetDCBrushColor(hdc, CLR_CARD);
    SetDCPenColor(hdc, CLR_BORDER);
    RoundRect(hdc, card.left, card.top, card.right, card.bottom, S(12), S(12));

    if (g_st.step == 0) {
        // Update / Fresh install banner
        RECT bnr = {card.left + S(16), card.top + S(14), card.right - S(16), card.top + S(58)};
        COLORREF bBg = g_st.isUpdate ? RGB(28, 58, 46) : RGB(28, 44, 68);
        COLORREF bBd = g_st.isUpdate ? CLR_SUCCESS : CLR_ACCENT;
        SetDCBrushColor(hdc, bBg);
        SetDCPenColor(hdc, bBd);
        RoundRect(hdc, bnr.left, bnr.top, bnr.right, bnr.bottom, S(8), S(8));
        SelectObject(hdc, hFontUI);
        SetTextColor(hdc, CLR_TEXT);
        RECT bt = {bnr.left + S(12), bnr.top + S(4), bnr.right - S(12), bnr.bottom - S(4)};
        DrawTextW(hdc,
                  g_st.isUpdate
                      ? LStr(L"Güncelleme Modu: Mevcut kurulum tespit edildi.\nTüm ayarlarınız silinmeden korunacak ve üzerine yazılacaktır.",
                             L"Update Mode: Existing installation detected.\nAll your settings will be preserved and updated in place.")
                      : LStr(L"Yeni Kurulum: ETDAmbilight tek parça (EXE) olarak kurulacaktır.\nAyarlarınız hem Registry'de hem EXE içinde güvenle saklanır.",
                             L"Fresh Install: ETDAmbilight installs as a fast standalone EXE.\nYour settings are safely stored in Registry and inside the EXE."),
                  -1, &bt, DT_LEFT | DT_VCENTER | DT_WORDBREAK);
    } else if (g_st.step == 1) {
        // Requirements rows
        int rx = card.left + S(18), rw = (card.right - card.left) - S(36);
        int ry = card.top + S(18);
        auto DrawReqRow = [&](const wchar_t* title, const wchar_t* status, bool ok) {
            RECT rr = {rx, ry, rx + rw, ry + S(48)};
            SetDCBrushColor(hdc, CLR_CTRL_BG);
            SetDCPenColor(hdc, ok ? RGB(45, 95, 72) : RGB(120, 90, 40));
            RoundRect(hdc, rr.left, rr.top, rr.right, rr.bottom, S(8), S(8));

            SelectObject(hdc, hFontSemi);
            SetTextColor(hdc, CLR_TEXT);
            RECT tr = {rr.left + S(14), rr.top + S(6), rr.right - S(14), rr.top + S(26)};
            DrawTextW(hdc, title, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hFontSmall);
            SetTextColor(hdc, ok ? CLR_SUCCESS : CLR_WARN);
            RECT sr = {rr.left + S(14), rr.top + S(24), rr.right - S(14), rr.bottom - S(4)};
            DrawTextW(hdc, status, -1, &sr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            ry += S(58);
        };

        DrawReqRow(
            LStr(L"Ekran Yakalama (DXGI Desktop Duplication / Direct3D 11)",
                 L"Screen Capture (DXGI Desktop Duplication / Direct3D 11)"),
            g_st.dxgiOk
                ? LStr(L"✓ Hazır — Donanım hızlandırmalı düşük CPU yakalama destekleniyor",
                       L"✓ Ready — Hardware-accelerated low-CPU capture supported")
                : LStr(L"⚠ DXGI bulunamadı — Otomatik GDI uyumluluk modu kullanılacak",
                       L"⚠ DXGI unavailable — Automatic GDI fallback will be used"),
            g_st.dxgiOk);

        DrawReqRow(
            LStr(L"Çalışma Zamanı & Bağımlılıklar (Static C++ Win32)",
                 L"Runtime & Dependencies (Static C++ Win32)"),
            LStr(L"✓ Hazır — Harici .NET veya VC++ paketi gerekmez",
                 L"✓ Ready — No external .NET or VC++ redistributable required"),
            true);

        DrawReqRow(
            LStr(L"Arduino USB-Seri Sürücüsü (CH340 / CH341)",
                 L"Arduino USB-Serial Driver (CH340 / CH341)"),
            g_st.ch340Installed
                ? LStr(L"✓ Sistemde tespit edildi — Bağlantı için hazır",
                       L"✓ Detected on system — Ready to connect")
                : LStr(L"⚠ Tespit edilemedi — Klon Arduino kartları için kurulum önerilir",
                       L"⚠ Not detected — Recommended for clone Arduino boards"),
            g_st.ch340Installed);
    } else if (g_st.step == 3) {
        // Progress bar
        int px = card.left + S(18), pw2 = (card.right - card.left) - S(36);
        int py2 = card.top + S(18), ph2 = S(14);
        SetDCBrushColor(hdc, CLR_CTRL_BG);
        SetDCPenColor(hdc, CLR_BORDER);
        RoundRect(hdc, px, py2, px + pw2, py2 + ph2, S(7), S(7));
        int fw = (pw2 * std::clamp(g_st.progress, 0, 100)) / 100;
        if (fw > S(4)) {
            COLORREF pCol = g_st.finished ? (g_st.installOk ? CLR_SUCCESS : CLR_ERROR) : CLR_ACCENT;
            SetDCBrushColor(hdc, pCol);
            SetDCPenColor(hdc, pCol);
            RoundRect(hdc, px, py2, px + fw, py2 + ph2, S(7), S(7));
        }
    }
}

// ─────────────────────────── WNDPROC ────────────────────────────────────────
static inline LRESULT CALLBACK SetupWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        UpdateDpiForWindow(hwnd);
        RebuildThemeResources();
        ApplyWindowChrome(hwnd);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int W = rc.right, H = rc.bottom;

        g_st.hBtnLang = MakeSetupBtn(hwnd, L"[TR] | EN", ID_BTN_LANG, CLR_CTRL_BG, true);
        SetWindowPos(g_st.hBtnLang, NULL, W - S(110), S(18), S(90), S(28), SWP_NOZORDER);
        ShowWindow(g_st.hBtnLang, SW_SHOW);

        int cardL = S(20), cardT = S(116), cardW = W - S(40), cardB = H - S(60);

        // Step 0
        g_st.hEditLic = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                        WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
                                        cardL + S(16), cardT + S(68), cardW - S(32), cardB - cardT - S(118),
                                        hwnd, (HMENU)ID_EDIT_LICENSE, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_st.hEditLic, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        ApplyDarkTheme(g_st.hEditLic, true);

        g_st.hTglAccept = MakeSetupToggle(hwnd, L"", ID_TGL_ACCEPT, true);
        SetWindowPos(g_st.hTglAccept, NULL, cardL + S(16), cardB - S(40), cardW - S(32), S(26), SWP_NOZORDER);

        // Step 1
        g_st.hTglCh340 = MakeSetupToggle(hwnd, L"", ID_TGL_CH340, !g_st.ch340Installed);
        SetWindowPos(g_st.hTglCh340, NULL, cardL + S(18), cardT + S(200), cardW - S(150), S(28), SWP_NOZORDER);
        g_st.hBtnRescan = MakeSetupBtn(hwnd, L"", ID_BTN_RESCAN, CLR_CTRL_BG);
        SetWindowPos(g_st.hBtnRescan, NULL, cardL + cardW - S(124), cardT + S(200), S(106), S(28), SWP_NOZORDER);

        // Step 2
        g_st.hLblPath = MakeSetupLbl(hwnd, L"", CLR_TEXT2);
        SetWindowPos(g_st.hLblPath, NULL, cardL + S(18), cardT + S(22), cardW - S(36), S(22), SWP_NOZORDER);
        g_st.hEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_st.installDir.c_str(),
                                         WS_CHILD | ES_AUTOHSCROLL,
                                         cardL + S(18), cardT + S(48), cardW - S(142), S(26),
                                         hwnd, (HMENU)ID_EDIT_PATH, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_st.hEditPath, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        ApplyDarkTheme(g_st.hEditPath, false);

        g_st.hBtnBrowse = MakeSetupBtn(hwnd, L"", ID_BTN_BROWSE, CLR_CTRL_BG);
        SetWindowPos(g_st.hBtnBrowse, NULL, cardL + cardW - S(116), cardT + S(47), S(98), S(28), SWP_NOZORDER);

        g_st.hTglDesktop   = MakeSetupToggle(hwnd, L"", ID_TGL_DESKTOP, true);
        g_st.hTglStartMenu = MakeSetupToggle(hwnd, L"", ID_TGL_STARTMENU, true);
        bool defAuto = g_st.hasExistingSettings ? g_st.existingSettings.autostart : false;
        g_st.hTglAutostart = MakeSetupToggle(hwnd, L"", ID_TGL_AUTOSTART, defAuto);
        SetWindowPos(g_st.hTglDesktop,   NULL, cardL + S(18), cardT + S(96),  cardW - S(36), S(28), SWP_NOZORDER);
        SetWindowPos(g_st.hTglStartMenu, NULL, cardL + S(18), cardT + S(132), cardW - S(36), S(28), SWP_NOZORDER);
        SetWindowPos(g_st.hTglAutostart, NULL, cardL + S(18), cardT + S(168), cardW - S(36), S(28), SWP_NOZORDER);

        // Step 3
        g_st.hEditLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                        WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
                                        cardL + S(18), cardT + S(44), cardW - S(36), cardB - cardT - S(96),
                                        hwnd, (HMENU)ID_EDIT_LOG, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_st.hEditLog, WM_SETFONT, (WPARAM)hFontMono, TRUE);
        ApplyDarkTheme(g_st.hEditLog, true);

        g_st.hTglLaunch = MakeSetupToggle(hwnd, L"", ID_TGL_LAUNCH, true);
        SetWindowPos(g_st.hTglLaunch, NULL, cardL + S(18), cardB - S(40), cardW - S(36), S(28), SWP_NOZORDER);

        // Footer
        g_st.hBtnCancel = MakeSetupBtn(hwnd, L"", ID_BTN_CANCEL, CLR_CTRL_BG, true);
        g_st.hBtnBack   = MakeSetupBtn(hwnd, L"", ID_BTN_BACK,   CLR_CTRL_BG, true);
        g_st.hBtnNext   = MakeSetupBtn(hwnd, L"", ID_BTN_NEXT,   CLR_ACCENT,  true);
        SetWindowPos(g_st.hBtnCancel, NULL, S(20),      H - S(46), S(96),  S(32), SWP_NOZORDER);
        SetWindowPos(g_st.hBtnBack,   NULL, W - S(238), H - S(46), S(96),  S(32), SWP_NOZORDER);
        SetWindowPos(g_st.hBtnNext,   NULL, W - S(134), H - S(46), S(114), S(32), SWP_NOZORDER);

        ShowSetupStep(hwnd, 0);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        if (rc.right > 0 && rc.bottom > 0) {
            g_st.EnsureBackBuf(hdc, rc.right, rc.bottom);
            if (g_st.memDC) {
                PaintSetupWindow(hwnd, g_st.memDC, rc.right, rc.bottom);
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, g_st.memDC, 0, 0, SRCCOPY);
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

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, CLR_CTRL_BG);
        SetTextColor(hdc, CLR_TEXT);
        return (LRESULT)hBrCtrl;
    }

    case WM_DRAWITEM:
        DrawModernButton((DRAWITEMSTRUCT*)lp);
        return TRUE;

    case WM_SETUP_PROGRESS: {
        g_st.progress = (int)wp;
        auto* msgStr = (std::wstring*)lp;
        if (msgStr) {
            AppendLogLine(g_st.hEditLog, *msgStr);
            delete msgStr;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_SETUP_DONE: {
        g_st.installing = false;
        g_st.finished = true;
        g_st.installOk = (wp != 0);
        ShowSetupStep(hwnd, 3);
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id == ID_BTN_LANG) {
            int nextL = (CurrentLang() == 1) ? 0 : 1;
            g_lang.store(nextL, std::memory_order_relaxed);
            UpdateSetupTexts(hwnd);
        } else if (id == ID_TGL_ACCEPT || id == ID_TGL_CH340 || id == ID_TGL_DESKTOP ||
                   id == ID_TGL_STARTMENU || id == ID_TGL_AUTOSTART || id == ID_TGL_LAUNCH) {
            HWND hT = (HWND)lp;
            SetToggleState(hT, !GetToggleState(hT));
        } else if (id == ID_BTN_RESCAN) {
            ScanSystemRequirements(g_st);
            SetToggleState(g_st.hTglCh340, !g_st.ch340Installed);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == ID_BTN_BROWSE) {
            BROWSEINFOW bi = {};
            bi.hwndOwner = hwnd;
            bi.lpszTitle = LStr(L"Kurulum klasörünü seçin:", L"Select installation folder:");
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                wchar_t path[MAX_PATH] = {};
                if (SHGetPathFromIDListW(pidl, path)) {
                    std::wstring p = path;
                    if (p.size() < 12 || _wcsicmp(p.c_str() + p.size() - 12, L"ETDAmbilight") != 0) {
                        if (p.back() != L'\\') p += L'\\';
                        p += L"ETDAmbilight";
                    }
                    SetWindowTextW(g_st.hEditPath, p.c_str());
                }
                CoTaskMemFree(pidl);
            }
        } else if (id == ID_BTN_CANCEL) {
            if (!g_st.installing) DestroyWindow(hwnd);
        } else if (id == ID_BTN_BACK) {
            if (g_st.step > 0 && g_st.step < 3) ShowSetupStep(hwnd, g_st.step - 1);
        } else if (id == ID_BTN_NEXT) {
            if (g_st.step == 0) {
                if (!GetToggleState(g_st.hTglAccept)) {
                    MessageBoxW(hwnd,
                                LStr(L"Devam etmek için lisans koşullarını kabul etmelisiniz.",
                                     L"You must accept the license terms to continue."),
                                L"ETDAmbilight Setup", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                ShowSetupStep(hwnd, 1);
            } else if (g_st.step == 1) {
                ShowSetupStep(hwnd, 2);
            } else if (g_st.step == 2) {
                wchar_t dirBuf[MAX_PATH] = {};
                GetWindowTextW(g_st.hEditPath, dirBuf, MAX_PATH);
                std::wstring target = dirBuf;
                while (!target.empty() && (target.back() == L'\\' || target.back() == L'/')) target.pop_back();
                if (target.empty()) {
                    MessageBoxW(hwnd, LStr(L"Geçerli bir klasör girin.", L"Please enter a valid folder."),
                                L"ETDAmbilight Setup", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                g_st.installDir = target;
                g_st.installing = true;
                g_st.finished = false;
                g_st.progress = 0;
                SetWindowTextW(g_st.hEditLog, L"");
                ShowSetupStep(hwnd, 3);

                InstallJob job;
                job.hwnd        = hwnd;
                job.targetDir   = target;
                job.doDesktop   = GetToggleState(g_st.hTglDesktop);
                job.doStartMenu = GetToggleState(g_st.hTglStartMenu);
                job.doAutostart = GetToggleState(g_st.hTglAutostart);
                job.doCh340     = GetToggleState(g_st.hTglCh340);
                job.lang        = CurrentLang();
                std::thread(RunInstallThread, job).detach();
            } else if (g_st.step == 3 && g_st.finished) {
                if (g_st.installOk && GetToggleState(g_st.hTglLaunch)) {
                    std::wstring exe = g_st.installDir + L"\\ETDAmbilight.exe";
                    ShellExecuteW(NULL, L"open", exe.c_str(), NULL, g_st.installDir.c_str(), SW_SHOWNORMAL);
                }
                DestroyWindow(hwnd);
            }
        }
        return 0;
    }

    case WM_CLOSE:
        if (!g_st.installing) DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        g_st.ReleaseBackBuf();
        DestroyThemeResources();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ─────────────────────────── UNINSTALL FLOW ─────────────────────────────────
static inline void RunUninstallFlow() {
    int sysLang = DetectSystemLanguage();
    g_lang.store(sysLang, std::memory_order_relaxed);

    int res = MessageBoxW(
        NULL,
        LStr(L"ETDAmbilight bilgisayarınızdan kaldırılsın mı?\n\n"
             L"Not: Kayıtlı LED/Ambilight ayarlarınız (Registry) varsayılan olarak korunacaktır.",
             L"Do you want to uninstall ETDAmbilight from your computer?\n\n"
             L"Note: Your saved LED/Ambilight settings (Registry) will be preserved by default."),
        L"ETDAmbilight Uninstall",
        MB_YESNO | MB_ICONQUESTION);
    if (res != IDYES) return;

    int delSet = MessageBoxW(
        NULL,
        LStr(L"Kayıtlı tüm ETDAmbilight ayarlarınız da silinsin mi?",
             L"Also delete all your saved ETDAmbilight settings?"),
        L"ETDAmbilight Uninstall",
        MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);

    CloseRunningAppInstances();

    std::wstring dir = ReadRegString(HKEY_CURRENT_USER, UNINST_KEY, L"InstallLocation");
    if (dir.empty()) {
        wchar_t self[MAX_PATH] = {};
        GetModuleFileNameW(NULL, self, MAX_PATH);
        dir = self;
        size_t pos = dir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) dir = dir.substr(0, pos);
    }

    // Remove autostart
    HKEY hRun = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_REG_KEY, 0, KEY_SET_VALUE, &hRun) == ERROR_SUCCESS) {
        RegDeleteValueW(hRun, APP_NAME);
        RegCloseKey(hRun);
    }

    // Remove shortcuts
    wchar_t desk[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desk))) {
        DeleteFileW((std::wstring(desk) + L"\\ETDAmbilight.lnk").c_str());
    }
    wchar_t sm[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, sm))) {
        std::wstring smDir = std::wstring(sm) + L"\\ETDAmbilight";
        DeleteFileW((smDir + L"\\ETDAmbilight.lnk").c_str());
        DeleteFileW((smDir + L"\\Uninstall ETDAmbilight.lnk").c_str());
        RemoveDirectoryW(smDir.c_str());
    }

    // Remove Uninstall key (and optionally settings)
    RegDeleteTreeW(HKEY_CURRENT_USER, UNINST_KEY);
    if (delSet == IDYES) {
        RegDeleteTreeW(HKEY_CURRENT_USER, REG_KEY);
    }

    if (!dir.empty()) {
        DeleteFileW((dir + L"\\ETDAmbilight.exe").c_str());
        DeleteFileW((dir + L"\\ETDAmbilight.exe.old").c_str());
        // Schedule self-delete of Uninstall.exe and folder
        std::wstring cmd = L"/c timeout /t 2 /nobreak >nul & del /f /q \"" + dir + L"\\Uninstall.exe\" & rmdir \"" + dir + L"\"";
        ShellExecuteW(NULL, L"open", L"cmd.exe", cmd.c_str(), NULL, SW_HIDE);
    }

    MessageBoxW(NULL,
                LStr(L"ETDAmbilight başarıyla kaldırıldı.", L"ETDAmbilight has been uninstalled."),
                L"ETDAmbilight Uninstall", MB_OK | MB_ICONINFORMATION);
}

// ─────────────────────────── ENTRY POINT ────────────────────────────────────
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow) {
    if (lpCmdLine && wcsstr(lpCmdLine, L"--uninstall") != NULL) {
        RunUninstallFlow();
        return 0;
    }

    // DPI awareness
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* SetDpiCtxFn)(HANDLE);
        auto fn = (SetDpiCtxFn)GetProcAddress(hUser, "SetProcessDpiAwarenessContext");
        if (fn) fn((HANDLE)(INT_PTR)-4);
    }

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    g_lang.store(DetectSystemLanguage(), std::memory_order_relaxed);
    DetectExistingInstallation(g_st);
    if (g_st.hasExistingSettings && (g_st.existingSettings.lang == 0 || g_st.existingSettings.lang == 1)) {
        g_lang.store(g_st.existingSettings.lang, std::memory_order_relaxed);
    }
    ScanSystemRequirements(g_st);

    g_st.hIconBig = (HICON)LoadImageW(hInst, L"MAINICON", IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
    g_st.hIconSm  = (HICON)LoadImageW(hInst, L"MAINICON", IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc   = SetupWndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"ETDAmbilightSetup";
    wc.hbrBackground = CreateSolidBrush(CLR_BG);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = g_st.hIconBig;
    wc.hIconSm       = g_st.hIconSm;
    RegisterClassExW(&wc);

    UpdateDpiForWindow(NULL);
    int ww = S(640), wh = S(490);
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"ETDAmbilightSetup",
        L"ETDAmbilight Setup",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        sx, sy, ww, wh,
        NULL, NULL, hInst, NULL);

    if (!hwnd) return 1;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_st.hIconBig) DestroyIcon(g_st.hIconBig);
    if (g_st.hIconSm)  DestroyIcon(g_st.hIconSm);
    return (int)msg.wParam;
}
