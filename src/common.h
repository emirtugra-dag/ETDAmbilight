#pragma once

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define _USE_MATH_DEFINES

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <dbt.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <winhttp.h>

#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// ─────────────────────────── CONSTANTS ──────────────────────────────────────
[[maybe_unused]] static const wchar_t* APP_NAME       = L"ETDAmbilight";
[[maybe_unused]] static const wchar_t* APP_VER        = L"2.0";
[[maybe_unused]] static const wchar_t* APP_AUTHOR     = L"Emir Tuğra Dağ";
[[maybe_unused]] static const wchar_t* APP_REPO       = L"emirtugra-dag/ETDAmbilight";
[[maybe_unused]] static const wchar_t* REG_KEY        = L"Software\\ETDAmbilight";
[[maybe_unused]] static const wchar_t* LEGACY_REG_KEY = L"Software\\LEDci";
[[maybe_unused]] static const wchar_t* RUN_REG_KEY    = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const DWORD    BAUD_DEFAULT   = 1000000;
static const int      MAX_LEDS       = 1200;

static const uint64_t EXE_MAGIC      = 0x4C45446369434647ULL; // "LEDciCFG" (preserved for seamless upgrades)

// Custom window messages
#define WM_LED_UPDATE        (WM_USER + 1)
#define WM_STATUS_UPDATE     (WM_USER + 2)
#define WM_TRAYICON          (WM_USER + 3)
#define WM_UPDATE_CHECK_DONE (WM_USER + 4)

// ─────────────────────────── COLOR UTILS ────────────────────────────────────
struct RGB3 {
    uint8_t r, g, b;
    bool operator==(const RGB3& o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const RGB3& o) const { return !(*this == o); }
};

static inline RGB3 HsvToRgb(float h, float s, float v) {
    h = fmodf(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    float r, g, b;
    int i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return {(uint8_t)(r * 255.0f), (uint8_t)(g * 255.0f), (uint8_t)(b * 255.0f)};
}

static inline void RgbToHsv(float r, float g, float b, float& h, float& s, float& v) {
    float mx = std::max({r, g, b});
    float mn = std::min({r, g, b});
    float d = mx - mn;
    v = mx;
    s = (mx < 1e-6f) ? 0.0f : d / mx;
    if (d < 1e-6f) {
        h = 0.0f;
    } else if (mx == r) {
        h = fmodf((g - b) / d, 6.0f) / 6.0f;
    } else if (mx == g) {
        h = ((b - r) / d + 2.0f) / 6.0f;
    } else {
        h = ((r - g) / d + 4.0f) / 6.0f;
    }
    if (h < 0.0f) h += 1.0f;
}

static inline RGB3 WinColorToRGB3(COLORREF c) {
    return {GetRValue(c), GetGValue(c), GetBValue(c)};
}

static inline uint8_t GammaCorrect(uint8_t v, float gamma) {
    if (gamma <= 0.01f) return v;
    return (uint8_t)(powf(v / 255.0f, gamma) * 255.0f + 0.5f);
}

static inline RGB3 BoostSaturation(RGB3 c, float sat) {
    if (fabsf(sat - 1.0f) < 0.01f) return c;
    float h, s, v;
    RgbToHsv(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, h, s, v);
    s = std::min(1.0f, s * sat);
    return HsvToRgb(h, s, v);
}

// ─────────────────────────── SETTINGS STRUCT ────────────────────────────────
struct Settings {
    wchar_t  port[32]       = L"AUTO";
    DWORD    baud           = 1000000;
    int      led_count      = 60;
    int      brightness     = 100;
    int      effect         = 0;   // 0: Ambilight, ..., 11: Calibration
    int      effect_speed   = 50;
    COLORREF color1         = RGB(255, 69, 0);
    COLORREF color2         = RGB(0, 128, 255);
    int      gain_r         = 100; // 20..100 (%) white balance red gain
    int      gain_g         = 100; // 20..100 (%) white balance green gain
    int      gain_b         = 100; // 20..100 (%) white balance blue gain
    int      color_order    = 0;   // 0=RGB, 1=GRB, 2=BGR, 3=RBG, 4=GBR, 5=BRG
    int      ambi_left         = 0;
    int      ambi_top          = 0;
    int      ambi_right        = 60;
    int      ambi_bottom_left  = 0;
    int      ambi_bottom_right = 0;
    int      bottom_gap_pct    = 0;   // 0..60 (% monitor stand gap on bottom edge)
    int      start_corner      = 3;   // 0=Sol-Alt, 1=Sol-Ust, 2=Sag-Ust, 3=Sag-Alt, 4=Alt-Ayak
    int      direction         = 1;   // 0=Saat yonu, 1=Saat yonu tersi
    int      led_offset        = 0;   // -500..500
    int      edge_depth_pct    = 8;   // 2..30 (%)
    float    saturation        = 1.0f;
    float    smoothing         = 0.30f;
    int      monitor_idx       = 0;
    int      target_fps        = 60;
    float    gamma             = 2.2f;
    int      capture_method    = 0;   // 0=Otomatik(DXGI), 1=GDI
    bool     autostart         = false;
    bool     start_hidden      = true;
    bool     close_to_tray     = true;
    bool     leds_enabled      = true;
    int      lang              = -1;  // -1=auto, 0=TR, 1=EN

    int TotalEdgeLeds() const {
        return std::max(0, ambi_left) + std::max(0, ambi_top) +
               std::max(0, ambi_right) + std::max(0, ambi_bottom_left) +
               std::max(0, ambi_bottom_right);
    }
};

// ─────────────────────────── MONITOR INFO ───────────────────────────────────
struct MonitorEntry {
    HMONITOR hMon = NULL;
    RECT     rect = {};
    bool     primary = false;
    int      width() const { return rect.right - rect.left; }
    int      height() const { return rect.bottom - rect.top; }
};

// ─────────────────────────── GLOBAL STATE ───────────────────────────────────
static Settings               g_settings;
static std::mutex             g_settings_mtx;

static std::atomic<int>       g_lang{0};              // 0=TR, 1=EN
static std::atomic<bool>      g_running{false};
static std::atomic<bool>      g_connected{false};
static std::atomic<float>     g_fps_actual{0.0f};
static std::atomic<bool>      g_dirty{false};
static std::atomic<uint32_t>  g_layoutVersion{1};
static std::atomic<bool>      g_rescan{false};
static std::atomic<int>       g_activeCaptureMode{0}; // 0=Beklemede, 1=DXGI, 2=GDI

static std::vector<RGB3>      g_last_colors;
static std::mutex             g_colors_mtx;

static HANDLE                 g_captureWakeEvent = NULL;
[[maybe_unused]] static HWND                   g_hwnd = NULL;
static std::atomic<bool>      g_uiVisible{true};

static std::atomic<bool>      g_wizardActive{false};
static std::atomic<int>       g_wizardStep{0};      // 0=Primary/Order, 1=White, 2=Gamma, 3=SampleImage
static std::atomic<int>       g_wizardPrimColor{0}; // 0=Red, 1=Green, 2=Blue

static inline void WakeCapturer() {
    if (g_captureWakeEvent) SetEvent(g_captureWakeEvent);
}

static inline void MarkLayoutChanged() {
    g_layoutVersion.fetch_add(1, std::memory_order_release);
    g_dirty.store(true, std::memory_order_relaxed);
    WakeCapturer();
}

static inline void MarkDirty() {
    g_dirty.store(true, std::memory_order_relaxed);
}
