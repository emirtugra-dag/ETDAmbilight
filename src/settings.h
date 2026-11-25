#pragma once

#include "common.h"
#include "i18n.h"

// ─────────────────────────── TEXT SERIALIZATION ─────────────────────────────
static inline std::wstring SettingsToText(const Settings& s) {
    std::wostringstream oss;
    oss << L"# ================================================================\r\n";
    oss << L"#  ETDAmbilight v" << APP_VER << L" - Adalight Configuration / Yapılandırma\r\n";
    oss << L"#  Copyright (c) 2026 Emir Tuğra Dağ\r\n";
    oss << L"# ================================================================\r\n\r\n";

    oss << L"[Baglanti]\r\n";
    oss << L"port = " << s.port << L"\r\n";
    oss << L"baud = " << s.baud << L"\r\n\r\n";

    oss << L"[LED]\r\n";
    oss << L"leds_enabled = " << (s.leds_enabled ? 1 : 0) << L"\r\n";
    oss << L"led_sayisi = " << s.led_count << L"\r\n";
    oss << L"parlaklik = " << s.brightness << L"\r\n";
    oss << L"gamma = " << std::fixed << std::setprecision(2) << s.gamma << L"\r\n";
    oss << L"kazanc_r = " << s.gain_r << L"\r\n";
    oss << L"kazanc_g = " << s.gain_g << L"\r\n";
    oss << L"kazanc_b = " << s.gain_b << L"\r\n";
    oss << L"renk_sirasi = " << s.color_order << L"\r\n\r\n";

    oss << L"[Efekt]\r\n";
    oss << L"# 0:Ambilight, 1:Rainbow, 2:RainbowWave, 3:Fire, 4:Breathing, 5:ColorWipe,\r\n";
    oss << L"# 6:Strobe, 7:Gradient, 8:Sparkle, 9:Police, 10:SolidColor, 11:Calibration\r\n";
    oss << L"efekt = " << s.effect << L"\r\n";
    oss << L"hiz = " << s.effect_speed << L"\r\n";
    oss << L"hedef_fps = " << s.target_fps << L"\r\n";
    wchar_t c1[16], c2[16];
    swprintf_s(c1, L"#%02X%02X%02X", GetRValue(s.color1), GetGValue(s.color1), GetBValue(s.color1));
    swprintf_s(c2, L"#%02X%02X%02X", GetRValue(s.color2), GetGValue(s.color2), GetBValue(s.color2));
    oss << L"ana_renk = " << c1 << L"\r\n";
    oss << L"ikincil_renk = " << c2 << L"\r\n\r\n";

    oss << L"[Ambilight_Yerlesim]\r\n";
    oss << L"sol_led = " << s.ambi_left << L"\r\n";
    oss << L"ust_led = " << s.ambi_top << L"\r\n";
    oss << L"sag_led = " << s.ambi_right << L"\r\n";
    oss << L"alt_sol_led = " << s.ambi_bottom_left << L"\r\n";
    oss << L"alt_sag_led = " << s.ambi_bottom_right << L"\r\n";
    oss << L"alt_bosluk = " << s.bottom_gap_pct << L"\r\n";
    oss << L"# 0=Sol-Alt/BL, 1=Sol-Ust/TL, 2=Sag-Ust/TR, 3=Sag-Alt/BR, 4=Alt-Ayak/Stand\r\n";
    oss << L"baslangic_kose = " << s.start_corner << L"\r\n";
    oss << L"# 0=Clockwise, 1=Counter-Clockwise\r\n";
    oss << L"yon = " << s.direction << L"\r\n";
    oss << L"kaydirma = " << s.led_offset << L"\r\n";
    oss << L"kenar_derinlik = " << s.edge_depth_pct << L"\r\n";
    oss << L"doygunluk = " << std::fixed << std::setprecision(2) << s.saturation << L"\r\n";
    oss << L"yumusatma = " << std::fixed << std::setprecision(2) << s.smoothing << L"\r\n";
    oss << L"monitor_idx = " << s.monitor_idx << L"\r\n";
    oss << L"yakalama_yontemi = " << s.capture_method << L"\r\n\r\n";

    oss << L"[Sistem]\r\n";
    oss << L"dil = " << (s.lang == 1 ? L"en" : L"tr") << L"\r\n";
    oss << L"otomatik_baslat = " << (s.autostart ? 1 : 0) << L"\r\n";
    oss << L"tepside_baslat = " << (s.start_hidden ? 1 : 0) << L"\r\n";
    oss << L"kapatinca_tepsi = " << (s.close_to_tray ? 1 : 0) << L"\r\n";

    return oss.str();
}

static inline bool ParseBoolVal(const std::wstring& v) {
    std::wstring lv = v;
    std::transform(lv.begin(), lv.end(), lv.begin(), ::towlower);
    if (lv == L"1" || lv == L"true" || lv == L"evet" || lv == L"yes") return true;
    if (lv == L"0" || lv == L"false" || lv == L"hayir" || lv == L"no") return false;
    try { return std::stoi(lv) != 0; } catch (...) { return false; }
}

static inline bool TextToSettings(const std::wstring& text, Settings& s) {
    std::wistringstream iss(text);
    std::wstring line;
    auto trim = [](const std::wstring& str) {
        size_t first = str.find_first_not_of(L" \t\r\n");
        if (first == std::wstring::npos) return std::wstring();
        size_t last = str.find_last_not_of(L" \t\r\n");
        return str.substr(first, (last - first + 1));
    };

    bool anyParsed = false;
    while (std::getline(iss, line)) {
        size_t comm = line.find_first_of(L"#;");
        if (comm != std::wstring::npos) {
            size_t eqCheck = line.find(L'=');
            if (eqCheck == std::wstring::npos || comm < eqCheck) {
                line = line.substr(0, comm);
            } else {
                size_t semi = line.find(L';');
                if (semi != std::wstring::npos) line = line.substr(0, semi);
                size_t hash2 = line.find(L'#', eqCheck + 1);
                while (hash2 != std::wstring::npos) {
                    if (hash2 > 0 && (line[hash2 - 1] == L' ' || line[hash2 - 1] == L'\t')) {
                        bool looksHex = (hash2 + 6 < line.size());
                        if (looksHex) {
                            for (int q = 1; q <= 6; ++q) {
                                wchar_t ch = line[hash2 + q];
                                if (!iswxdigit(ch)) { looksHex = false; break; }
                            }
                        }
                        if (!looksHex) {
                            line = line.substr(0, hash2);
                            break;
                        }
                    }
                    hash2 = line.find(L'#', hash2 + 1);
                }
            }
        }
        line = trim(line);
        if (line.empty() || line.front() == L'[') continue;

        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;

        std::wstring k = trim(line.substr(0, eq));
        std::wstring v = trim(line.substr(eq + 1));
        std::transform(k.begin(), k.end(), k.begin(), ::towlower);

        try {
            if (k == L"port") {
                if (!v.empty()) wcsncpy_s(s.port, v.c_str(), _TRUNCATE);
            } else if (k == L"baud") {
                s.baud = std::clamp((DWORD)std::stoul(v), (DWORD)9600, (DWORD)2000000);
            } else if (k == L"leds_enabled") {
                s.leds_enabled = ParseBoolVal(v);
            } else if (k == L"led_sayisi" || k == L"led_count") {
                s.led_count = std::clamp(std::stoi(v), 1, MAX_LEDS);
            } else if (k == L"parlaklik" || k == L"brightness") {
                s.brightness = std::clamp(std::stoi(v), 0, 200);
            } else if (k == L"gamma") {
                s.gamma = std::clamp(std::stof(v), 0.5f, 4.0f);
            } else if (k == L"kazanc_r" || k == L"gain_r") {
                s.gain_r = std::clamp(std::stoi(v), 20, 100);
            } else if (k == L"kazanc_g" || k == L"gain_g") {
                s.gain_g = std::clamp(std::stoi(v), 20, 100);
            } else if (k == L"kazanc_b" || k == L"gain_b") {
                s.gain_b = std::clamp(std::stoi(v), 20, 100);
            } else if (k == L"renk_sirasi" || k == L"color_order") {
                s.color_order = std::clamp(std::stoi(v), 0, 5);
            } else if (k == L"efekt" || k == L"effect") {
                s.effect = std::clamp(std::stoi(v), 0, 11);
            } else if (k == L"hiz" || k == L"speed" || k == L"effect_speed") {
                s.effect_speed = std::clamp(std::stoi(v), 1, 100);
            } else if (k == L"hedef_fps" || k == L"target_fps") {
                s.target_fps = std::clamp(std::stoi(v), 1, 240);
            } else if (k == L"sol_led" || k == L"ambi_left") {
                s.ambi_left = std::clamp(std::stoi(v), 0, 300);
            } else if (k == L"ust_led" || k == L"ambi_top") {
                s.ambi_top = std::clamp(std::stoi(v), 0, 300);
            } else if (k == L"sag_led" || k == L"ambi_right") {
                s.ambi_right = std::clamp(std::stoi(v), 0, 300);
            } else if (k == L"alt_sol_led" || k == L"ambi_bottom_left") {
                s.ambi_bottom_left = std::clamp(std::stoi(v), 0, 300);
            } else if (k == L"alt_sag_led" || k == L"ambi_bottom_right") {
                s.ambi_bottom_right = std::clamp(std::stoi(v), 0, 300);
            } else if (k == L"alt_led" || k == L"ambi_bottom") {
                int totB = std::clamp(std::stoi(v), 0, 600);
                s.ambi_bottom_left = totB / 2;
                s.ambi_bottom_right = totB - s.ambi_bottom_left;
            } else if (k == L"alt_bosluk" || k == L"bottom_gap_pct") {
                s.bottom_gap_pct = std::clamp(std::stoi(v), 0, 65);
            } else if (k == L"baslangic_kose" || k == L"start_corner") {
                s.start_corner = std::clamp(std::stoi(v), 0, 4);
            } else if (k == L"yon" || k == L"direction") {
                s.direction = std::clamp(std::stoi(v), 0, 1);
            } else if (k == L"kaydirma" || k == L"led_offset") {
                s.led_offset = std::clamp(std::stoi(v), -500, 500);
            } else if (k == L"kenar_derinlik" || k == L"edge_depth_pct") {
                s.edge_depth_pct = std::clamp(std::stoi(v), 2, 30);
            } else if (k == L"doygunluk" || k == L"saturation") {
                s.saturation = std::clamp(std::stof(v), 0.1f, 5.0f);
            } else if (k == L"yumusatma" || k == L"smoothing") {
                s.smoothing = std::clamp(std::stof(v), 0.0f, 0.98f);
            } else if (k == L"monitor_idx" || k == L"monitor") {
                s.monitor_idx = std::clamp(std::stoi(v), 0, 15);
            } else if (k == L"yakalama_yontemi" || k == L"capture_method") {
                s.capture_method = std::clamp(std::stoi(v), 0, 1);
            } else if (k == L"dil" || k == L"lang" || k == L"language") {
                std::wstring lv = v;
                std::transform(lv.begin(), lv.end(), lv.begin(), ::towlower);
                if (lv == L"en" || lv == L"english" || lv == L"1") s.lang = 1;
                else if (lv == L"tr" || lv == L"turkish" || lv == L"turkce" || lv == L"0") s.lang = 0;
            } else if (k == L"otomatik_baslat" || k == L"autostart") {
                s.autostart = ParseBoolVal(v);
            } else if (k == L"tepside_baslat" || k == L"start_hidden") {
                s.start_hidden = ParseBoolVal(v);
            } else if (k == L"kapatinca_tepsi" || k == L"close_to_tray") {
                s.close_to_tray = ParseBoolVal(v);
            } else if (k == L"ana_renk" || k == L"color1") {
                if (v.size() >= 7 && v[0] == L'#') {
                    unsigned int r = 0, g = 0, b = 0;
                    if (swscanf_s(v.c_str(), L"#%02x%02x%02x", &r, &g, &b) == 3)
                        s.color1 = RGB(r, g, b);
                }
            } else if (k == L"ikincil_renk" || k == L"color2") {
                if (v.size() >= 7 && v[0] == L'#') {
                    unsigned int r = 0, g = 0, b = 0;
                    if (swscanf_s(v.c_str(), L"#%02x%02x%02x", &r, &g, &b) == 3)
                        s.color2 = RGB(r, g, b);
                }
            }
            anyParsed = true;
        } catch (...) {}
    }

    int edgeSum = s.TotalEdgeLeds();
    if (edgeSum > 0) s.led_count = std::min(edgeSum, MAX_LEDS);
    return anyParsed;
}

// ─────────────────────────── AUTOSTART (WINDOWS RUN) ────────────────────────
static inline void SetAutostart(bool enable) {
    HKEY hk = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_REG_KEY, 0, KEY_SET_VALUE, &hk) != ERROR_SUCCESS)
        return;

    // Clean up legacy LEDci Run value if present
    RegDeleteValueW(hk, L"LEDci");

    if (enable) {
        wchar_t exePath[MAX_PATH] = {};
        if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
            std::wstring cmd = std::wstring(L"\"") + exePath + L"\" --autostart";
            RegSetValueExW(hk, APP_NAME, 0, REG_SZ,
                           (const BYTE*)cmd.c_str(),
                           (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
        }
    } else {
        RegDeleteValueW(hk, APP_NAME);
    }
    RegCloseKey(hk);
}

// ─────────────────────────── EXE FOOTER PERSISTENCE (v2) ────────────────────
static const size_t LEGACY_V1_FOOTER_SIZE = 144;

#pragma pack(push, 1)
struct ExeFooterTailV2 {
    uint32_t byteLen;
    uint32_t version;
    uint64_t magic;
};
#pragma pack(pop)

static inline bool LoadSettingsFromExePath(const wchar_t* exePath, Settings& s) {
    if (!exePath || !exePath[0]) return false;

    HANDLE hFile = CreateFileW(exePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER sz = {};
    if (!GetFileSizeEx(hFile, &sz) || sz.QuadPart < (LONGLONG)sizeof(ExeFooterTailV2)) {
        CloseHandle(hFile);
        return false;
    }

    LARGE_INTEGER pos = {};
    pos.QuadPart = sz.QuadPart - (LONGLONG)sizeof(ExeFooterTailV2);
    SetFilePointerEx(hFile, pos, NULL, FILE_BEGIN);

    ExeFooterTailV2 tail = {};
    DWORD read = 0;
    bool ok = false;
    if (ReadFile(hFile, &tail, sizeof(tail), &read, NULL) && read == sizeof(tail)) {
        if (tail.magic == EXE_MAGIC && tail.version == 2 &&
            tail.byteLen >= 2 && tail.byteLen <= 1024 * 1024 &&
            (LONGLONG)(tail.byteLen + sizeof(ExeFooterTailV2)) <= sz.QuadPart) {
            LARGE_INTEGER textPos = {};
            textPos.QuadPart = sz.QuadPart - (LONGLONG)(tail.byteLen + sizeof(ExeFooterTailV2));
            SetFilePointerEx(hFile, textPos, NULL, FILE_BEGIN);

            std::wstring text(tail.byteLen / sizeof(wchar_t), L'\0');
            if (ReadFile(hFile, text.data(), tail.byteLen, &read, NULL) && read == tail.byteLen) {
                ok = TextToSettings(text, s);
            }
        }
    }

    CloseHandle(hFile);
    return ok;
}

static inline bool LoadSettingsFromExe(Settings& s) {
    wchar_t exePath[MAX_PATH] = {};
    if (!GetModuleFileNameW(NULL, exePath, MAX_PATH)) return false;

    std::wstring oldPath = std::wstring(exePath) + L".old";
    DeleteFileW(oldPath.c_str());

    return LoadSettingsFromExePath(exePath, s);
}

static inline bool SaveSettingsToExePath(const wchar_t* exePath, const Settings& s) {
    if (!exePath || !exePath[0]) return false;

    HANDLE hFile = CreateFileW(exePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER sz = {};
    if (!GetFileSizeEx(hFile, &sz) || sz.QuadPart <= 0) {
        CloseHandle(hFile);
        return false;
    }

    std::vector<uint8_t> data((size_t)sz.QuadPart);
    DWORD read = 0;
    if (!ReadFile(hFile, data.data(), (DWORD)sz.QuadPart, &read, NULL) || read != (DWORD)sz.QuadPart) {
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);

    if (data.size() >= sizeof(ExeFooterTailV2)) {
        ExeFooterTailV2 tail = {};
        memcpy(&tail, &data[data.size() - sizeof(ExeFooterTailV2)], sizeof(ExeFooterTailV2));
        if (tail.magic == EXE_MAGIC) {
            if (tail.version == 2 && tail.byteLen <= data.size() - sizeof(ExeFooterTailV2)) {
                data.resize(data.size() - sizeof(ExeFooterTailV2) - tail.byteLen);
            } else if (tail.version == 1 && data.size() >= LEGACY_V1_FOOTER_SIZE) {
                data.resize(data.size() - LEGACY_V1_FOOTER_SIZE);
            }
        }
    }

    std::wstring text = SettingsToText(s);
    uint32_t byteLen = (uint32_t)(text.size() * sizeof(wchar_t));
    ExeFooterTailV2 newTail = {byteLen, 2, EXE_MAGIC};

    size_t oldSize = data.size();
    data.resize(oldSize + byteLen + sizeof(ExeFooterTailV2));
    memcpy(&data[oldSize], text.data(), byteLen);
    memcpy(&data[oldSize + byteLen], &newTail, sizeof(ExeFooterTailV2));

    std::wstring tmpPath = std::wstring(exePath) + L".tmp";
    HANDLE hTmp = CreateFileW(tmpPath.c_str(), GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hTmp == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    bool writeOk = WriteFile(hTmp, data.data(), (DWORD)data.size(), &written, NULL) &&
                   written == (DWORD)data.size();
    CloseHandle(hTmp);
    if (!writeOk) {
        DeleteFileW(tmpPath.c_str());
        return false;
    }

    std::wstring oldPath = std::wstring(exePath) + L".old";
    DeleteFileW(oldPath.c_str());
    if (!MoveFileExW(exePath, oldPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(tmpPath.c_str());
        return false;
    }

    if (!MoveFileExW(tmpPath.c_str(), exePath, MOVEFILE_REPLACE_EXISTING)) {
        MoveFileExW(oldPath.c_str(), exePath, MOVEFILE_REPLACE_EXISTING);
        DeleteFileW(tmpPath.c_str());
        return false;
    }
    DeleteFileW(oldPath.c_str());
    return true;
}

static inline bool SaveSettingsToExe(const Settings& s) {
    wchar_t exePath[MAX_PATH] = {};
    if (!GetModuleFileNameW(NULL, exePath, MAX_PATH)) return false;
    return SaveSettingsToExePath(exePath, s);
}

// ─────────────────────────── REGISTRY PERSISTENCE ───────────────────────────
static inline void SaveSettingsToRegistry(const Settings& s) {
    HKEY hk = NULL;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS) {
        std::wstring text = SettingsToText(s);
        RegSetValueExW(hk, L"config", 0, REG_SZ,
                       (const BYTE*)text.c_str(),
                       (DWORD)((text.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hk);
    }
}

static inline bool LoadSettingsFromRegistryKey(const wchar_t* keyPath, Settings& s) {
    HKEY hk = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hk) != ERROR_SUCCESS)
        return false;

    DWORD sz = 0, type = 0;
    bool ok = false;
    if (RegQueryValueExW(hk, L"config", NULL, &type, NULL, &sz) == ERROR_SUCCESS &&
        type == REG_SZ && sz >= sizeof(wchar_t) && sz <= 1024 * 1024) {
        std::wstring text(sz / sizeof(wchar_t), L'\0');
        if (RegQueryValueExW(hk, L"config", NULL, &type, (BYTE*)text.data(), &sz) == ERROR_SUCCESS) {
            if (!text.empty() && text.back() == L'\0') text.pop_back();
            ok = TextToSettings(text, s);
        }
    }
    RegCloseKey(hk);
    return ok;
}

static inline bool LoadSettingsFromRegistry(Settings& s) {
    return LoadSettingsFromRegistryKey(REG_KEY, s);
}

static inline void SaveSettings(const Settings& s, bool forceExe = false) {
    SaveSettingsToRegistry(s);
    if (forceExe || g_dirty.load(std::memory_order_relaxed)) {
        if (SaveSettingsToExe(s)) {
            g_dirty.store(false, std::memory_order_relaxed);
        }
    }
}

static inline void LoadSettings(Settings& s) {
    if (!LoadSettingsFromExe(s)) {
        if (!LoadSettingsFromRegistryKey(REG_KEY, s)) {
            if (LoadSettingsFromRegistryKey(LEGACY_REG_KEY, s)) {
                EnsureLanguageInitialized(s);
                SaveSettingsToRegistry(s);
            }
        }
    }
    EnsureLanguageInitialized(s);
    if (s.TotalEdgeLeds() > 0) {
        s.led_count = std::min(s.TotalEdgeLeds(), MAX_LEDS);
    }
}
