#pragma once

#include "common.h"
#include "i18n.h"
#include "layout.h"
#include "settings.h"
#include "serial.h"
#include "capture.h"
#include "effects.h"
#include "theme.h"
#include "wizard.h"
#include "updater.h"

// ─────────────────────────── CONTROL IDs ────────────────────────────────────
enum CtrlId {
    ID_TOGGLE_LEDS = 1001,
    ID_COMBO_PORT,
    ID_BTN_REFRESH,
    ID_COMBO_BAUD,
    ID_BTN_CONNECT,

    ID_SLIDER_BRIGHT,
    ID_COMBO_FPS,
    ID_COMBO_GAMMA,

    ID_COMBO_EFFECT,
    ID_SLIDER_SPEED,
    ID_BTN_COLOR1,
    ID_BTN_COLOR2,

    ID_EDIT_AMBI_L,
    ID_SPIN_AMBI_L,
    ID_EDIT_AMBI_T,
    ID_SPIN_AMBI_T,
    ID_EDIT_AMBI_R,
    ID_SPIN_AMBI_R,
    ID_EDIT_AMBI_BL,
    ID_SPIN_AMBI_BL,
    ID_EDIT_AMBI_BR,
    ID_SPIN_AMBI_BR,
    ID_SLIDER_GAP,

    ID_COMBO_CORNER,
    ID_BTN_DIR_CW,
    ID_BTN_DIR_CCW,
    ID_EDIT_OFFSET,
    ID_SPIN_OFFSET,
    ID_SLIDER_DEPTH,
    ID_SLIDER_SAT,
    ID_SLIDER_SMOOTH,
    ID_COMBO_MON,
    ID_TOGGLE_CALIB,

    ID_TOGGLE_AUTOSTART,
    ID_TOGGLE_STARTHIDDEN,
    ID_TOGGLE_CLOSETRAY,
    ID_COMBO_CAPTURE,
    ID_COMBO_LANG,

    ID_BTN_SAVE,
    ID_BTN_TEXTEDIT,
    ID_BTN_ARDUINO,
    ID_BTN_CH340,
    ID_BTN_UPDATE,
    ID_BTN_COLORWIZ,

    ID_PREVIEW_BIG,
    ID_PREVIEW_SMALL,
};

// Tray menu IDs
enum TrayMenuId {
    ID_TRAY_SHOW = 3001,
    ID_TRAY_TOGGLE_LEDS,
    ID_TRAY_EXIT,
    ID_TRAY_EFFECT_BASE = 3100, // + 0..11
};

// ─────────────────────────── UI STATE ───────────────────────────────────────
extern SerialPort    g_serial;
extern Capturer      g_capturer;
extern EffectsEngine g_effects;

static UINT WM_APP_SHOW        = 0;
static UINT WM_TASKBAR_CREATED = 0;
static HICON g_hAppIcon       = NULL;
static HICON g_hSmallIcon     = NULL;
static NOTIFYICONDATAW g_nid  = {};
static bool  g_trayAdded      = false;

static int   g_activeTab          = 0; // 0=Önizleme, 1=Ambilight Düzeni, 2=Sistem
static int   g_prevNonCalibEffect = 0;
static bool  g_suppressSync       = false;
static int   g_uiTickCounter      = 0;

static std::vector<PortInfo>     g_ports;
static std::vector<MonitorEntry> g_monitors;
static std::vector<HWND>         g_pages[3];
static RECT                      g_tabRects[3] = {};
static RECT                      g_cardRects[5] = {}; // 3 left cards + right page cards

// Main controls
static HWND hToggleLeds = NULL;
static HWND hPortCB = NULL, hBtnRefresh = NULL, hBaudCB = NULL, hConnectBtn = NULL;
static HWND hBrightSlider = NULL, hBrightVal = NULL, hFpsCB = NULL, hGammaCB = NULL;
static HWND hEffectCB = NULL, hSpeedSlider = NULL, hSpeedVal = NULL;
static HWND hColor1Btn = NULL, hColor2Btn = NULL, hBtnColorWiz = NULL;

// Left labels (for repositioning on DPI change)
static HWND hLblPort = NULL, hLblBaud = NULL;
static HWND hLblBright = NULL, hLblFps = NULL, hLblGamma = NULL;
static HWND hLblEffect = NULL, hLblSpeed = NULL, hLblColor1 = NULL, hLblColor2 = NULL;

// Page 0: Preview
static HWND hPreviewBig = NULL;

// Page 1: Ambilight layout
static HWND hAmbiEdit[5] = {}, hAmbiSpin[5] = {}, hAmbiLbl[5] = {}; // 0=Sol, 1=Ust, 2=Sag, 3=Alt-Sol, 4=Alt-Sag
static HWND hLblTotalLeds = NULL;
static HWND hLblGap = NULL, hGapSlider = NULL, hGapVal = NULL;
static HWND hLblCorner = NULL, hCornerCB = NULL;
static HWND hLblDir = NULL, hBtnDirCW = NULL, hBtnDirCCW = NULL;
static HWND hLblOffset = NULL, hOffsetEdit = NULL, hOffsetSpin = NULL;
static HWND hLblDepth = NULL, hDepthSlider = NULL, hDepthVal = NULL;
static HWND hLblSat = NULL, hSatSlider = NULL, hSatVal = NULL;
static HWND hLblSmooth = NULL, hSmoothSlider = NULL, hSmoothVal = NULL;
static HWND hLblMon = NULL, hMonCB = NULL;
static HWND hToggleCalib = NULL;
static HWND hPreviewSmall = NULL;

// Page 2: System
static HWND hToggleAutostart = NULL, hToggleStartHidden = NULL, hToggleCloseTray = NULL;
static HWND hLblCapture = NULL, hCaptureCB = NULL;
static HWND hLblLang = NULL, hLangCB = NULL;
static HWND hInfoTitle[5] = {}, hInfoVal[5] = {};
static HWND hBtnSave = NULL, hBtnTextEdit = NULL, hBtnArduino = NULL, hBtnCh340 = NULL, hBtnUpdate = NULL;
static HWND hLblCredit = NULL;
static bool g_updateCheckBusy = false;

// Forward declarations
static void SyncControlsFromSettings();
static void UpdateConnectButton();
static void UpdateDirectionButtons(int dir);
static void LayoutAllControls(HWND hwnd);

// ─────────────────────────── CONTROL FACTORIES ──────────────────────────────
static inline HWND MakeLbl(HWND parent, const wchar_t* txt, COLORREF fg = CLR_TEXT, HFONT f = NULL) {
    HWND h = CreateWindowExW(0, L"STATIC", txt, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                             0, 0, 10, 10, parent, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)(f ? f : hFontUI), TRUE);
    SetPropW(h, L"fg", (HANDLE)(UINT_PTR)fg);
    return h;
}

static inline HWND MakeBtn(HWND parent, const wchar_t* txt, int id, COLORREF bg = CLR_CTRL_BG) {
    HWND h = CreateWindowExW(0, L"BUTTON", txt, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                             0, 0, 10, 10, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SetBtnBg(h, bg);
    AttachHoverSubclass(h);
    return h;
}

static inline HWND MakeToggle(HWND parent, const wchar_t* txt, int id, bool on) {
    HWND h = CreateWindowExW(0, L"BUTTON", txt, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                             0, 0, 10, 10, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SetToggleState(h, on);
    AttachHoverSubclass(h);
    return h;
}

static inline HWND MakeCombo(HWND parent, int id) {
    HWND h = CreateWindowExW(0, L"COMBOBOX", NULL,
                             WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                             0, 0, 10, 200, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    ApplyDarkTheme(h, false);
    return h;
}

static inline HWND MakeSlider(HWND parent, int id, int minV, int maxV, int val) {
    HWND h = CreateWindowExW(0, TRACKBAR_CLASSW, NULL,
                             WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                             0, 0, 10, 10, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(h, TBM_SETRANGE, TRUE, MAKELONG(minV, maxV));
    SendMessageW(h, TBM_SETPOS, TRUE, val);
    return h;
}

static inline void MakeSpinnerPair(HWND parent, int editId, int spinId, int minV, int maxV, int val,
                                   HWND& outEdit, HWND& outSpin) {
    outEdit = CreateWindowExW(0, L"EDIT", L"0",
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_RIGHT,
                              0, 0, 10, 10, parent, (HMENU)(INT_PTR)editId, GetModuleHandleW(NULL), NULL);
    SendMessageW(outEdit, WM_SETFONT, (WPARAM)hFontMono, TRUE);
    ApplyDarkTheme(outEdit, false);

    outSpin = CreateWindowExW(0, UPDOWN_CLASSW, NULL,
                              WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT |
                              UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_HOTTRACK,
                              0, 0, 10, 10, parent, (HMENU)(INT_PTR)spinId, GetModuleHandleW(NULL), NULL);
    SendMessageW(outSpin, UDM_SETBUDDY, (WPARAM)outEdit, 0);
    SendMessageW(outSpin, UDM_SETRANGE32, minV, maxV);
    SendMessageW(outSpin, UDM_SETPOS32, 0, val);
    ApplyDarkTheme(outSpin, false);
}

// ─────────────────────────── PREVIEW DRAWING ────────────────────────────────
struct PreviewLedPt {
    int x, y, edge;
};

struct PreviewBuf {
    HDC     memDC  = NULL;
    HBITMAP hbm    = NULL;
    HGDIOBJ oldHbm = NULL;
    int     w      = 0;
    int     h      = 0;
    std::vector<PreviewLedPt> pts;
    int     hoverLed = -1;

    void Ensure(HDC ref, int nw, int nh) {
        if (nw <= 0 || nh <= 0) return;
        if (memDC && hbm && w == nw && h == nh) return;
        Release();
        memDC = CreateCompatibleDC(ref);
        hbm   = CreateCompatibleBitmap(ref, nw, nh);
        if (memDC && hbm) {
            oldHbm = SelectObject(memDC, hbm);
        }
        w = nw;
        h = nh;
    }

    void Release() {
        if (memDC && oldHbm) { SelectObject(memDC, oldHbm); oldHbm = NULL; }
        if (hbm)   { DeleteObject(hbm); hbm = NULL; }
        if (memDC) { DeleteDC(memDC); memDC = NULL; }
        w = h = 0;
    }

    int HitTest(int mx, int my) const {
        int bestIdx = -1;
        int bestD2 = S(16) * S(16);
        for (int i = 0; i < (int)pts.size(); ++i) {
            int dx = mx - pts[i].x;
            int dy = my - pts[i].y;
            int d2 = dx * dx + dy * dy;
            if (d2 <= bestD2) {
                bestD2 = d2;
                bestIdx = i;
            }
        }
        return bestIdx;
    }
};

static inline void DrawMonitorPreview(
    HDC dc, int W, int H,
    const Settings& s,
    const std::vector<RGB3>& colors,
    bool showStrip,
    std::vector<PreviewLedPt>& outPts,
    int hoverLed)
{
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(DC_PEN));

    // Card background
    SetDCBrushColor(dc, CLR_CARD);
    SetDCPenColor(dc, CLR_BORDER);
    RoundRect(dc, 0, 0, W, H, S(12), S(12));

    int stripH = showStrip ? S(42) : 0;
    int areaH  = H - stripH;

    int padX = S(56), padY = S(46);
    int maxMW = std::max(S(80), W - 2 * padX);
    int maxMH = std::max(S(60), areaH - 2 * padY);

    // 16:9 aspect monitor inside available area
    int monW = maxMW;
    int monH = (monW * 9) / 16;
    if (monH > maxMH) {
        monH = maxMH;
        monW = (monH * 16) / 9;
    }
    int monL = (W - monW) / 2;
    int monT = (areaH - monH) / 2;
    int monR = monL + monW;
    int monB = monT + monH;

    int L  = s.ambi_left, T = s.ambi_top, R = s.ambi_right;
    int BL = s.ambi_bottom_left, BR = s.ambi_bottom_right;
    if (L + T + R + BL + BR <= 0) R = std::max(1, s.led_count);
    float depth = std::clamp(s.edge_depth_pct, 2, 30) / 100.0f;
    float gapF  = std::clamp(s.bottom_gap_pct, 0, 65) / 100.0f;

    // Monitor stand silhouette strictly confined inside the bottom gap span
    BottomGapSpan gap = ComputeBottomGapSpan(BL, BR, gapF);
    if (gap.hasGap) {
        int gx0 = monL + (int)(gap.x0 * monW) + S(4);
        int gx1 = monL + (int)(gap.x1 * monW) - S(4);
        if (gx1 > gx0 + S(12)) {
            int gcx = (gx0 + gx1) / 2;
            int gw  = gx1 - gx0;
            int neckW = std::min(gw, std::max(S(28), gw * 3 / 4));
            int neckH = S(22);

            SetDCBrushColor(dc, RGB(40, 44, 52));
            SetDCPenColor(dc, RGB(68, 74, 86));
            RoundRect(dc, gcx - neckW / 2, monB - S(2), gcx + neckW / 2, monB + neckH, S(5), S(5));

            int footW = std::min(gw, neckW + S(10));
            RoundRect(dc, gcx - footW / 2, monB + neckH - S(5), gcx + footW / 2, monB + neckH + S(2), S(3), S(3));

            SetBkMode(dc, TRANSPARENT);
            SelectObject(dc, hFontSmall);
            SetTextColor(dc, CLR_TEXT2);
            RECT gapTxt = {gx0, monB + S(1), gx1, monB + neckH - S(3)};
            DrawTextW(dc, LStr(L"AYAK", L"STAND"), -1, &gapTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    // Monitor frame
    SetDCBrushColor(dc, RGB(18, 21, 28));
    SetDCPenColor(dc, RGB(68, 74, 86));
    RoundRect(dc, monL, monT, monR, monB, S(10), S(10));

    // Inner screen info & interactive hint
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, hFontSemi);
    const wchar_t* dirTxt = (s.direction == 0)
        ? LStr(L"Sola (Saat Yönü)", L"Left (Clockwise)")
        : LStr(L"Sağa (Ters Yön)", L"Right (CCW)");
    const wchar_t* crnTxt[5] = {
        LStr(L"Sol-Alt", L"Bottom-Left"),
        LStr(L"Sol-Üst", L"Top-Left"),
        LStr(L"Sağ-Üst", L"Top-Right"),
        LStr(L"Sağ-Alt", L"Bottom-Right"),
        LStr(L"Alt-Ayak", L"Bottom-Stand")
    };
    wchar_t line1[128];
    if (hoverLed >= 0) {
        SetTextColor(dc, CLR_ACCENT);
        swprintf_s(line1, LStr(L"Tıkla: LED #%d başlangıç (LED #1) olsun",
                               L"Click: Set LED #%d as start (LED #1)"), hoverLed + 1);
    } else {
        SetTextColor(dc, RGB(115, 122, 138));
        swprintf_s(line1, LStr(L"Başlangıç: %s  •  %s", L"Start: %s  •  %s"),
                   crnTxt[std::clamp(s.start_corner, 0, 4)], dirTxt);
    }
    int midY = (monT + monB) / 2;
    RECT r1 = {monL + S(8), midY - S(18), monR - S(8), midY};
    DrawTextW(dc, line1, -1, &r1, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(dc, hFontSmall);
    SetTextColor(dc, RGB(78, 84, 98));
    RECT r2 = {monL + S(8), midY + S(2), monR - S(8), midY + S(20)};
    DrawTextW(dc, LStr(L"LED'e sol tık: Başlangıç seç  •  Sağ tık: Yönü çevir",
                       L"Left-click LED: Set start  •  Right-click: Flip dir"),
              -1, &r2, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    auto meta = BuildOrderedLayoutMeta(L, T, R, BL, BR, s.start_corner, s.direction, s.led_offset, depth, gapF);
    int N = (int)meta.size();
    outPts.resize(N);

    if (N > 0) {
        int ledOff = S(14);
        int ledR   = (N > 140) ? S(4) : (N > 80 ? S(5) : S(6));
        int glowR  = ledR + S(5);

        for (int k = 0; k < N; ++k) {
            const Zone& z = meta[k].zone;
            int e = meta[k].edge;
            int cx = 0, cy = 0;
            if (e == 0) { // Left
                cx = monL - ledOff;
                cy = monT + (int)(((z.y0 + z.y1) * 0.5f) * monH);
            } else if (e == 1) { // Top
                cx = monL + (int)(((z.x0 + z.x1) * 0.5f) * monW);
                cy = monT - ledOff;
            } else if (e == 2) { // Right
                cx = monR + ledOff;
                cy = monT + (int)(((z.y0 + z.y1) * 0.5f) * monH);
            } else { // Bottom
                cx = monL + (int)(((z.x0 + z.x1) * 0.5f) * monW);
                cy = monB + ledOff;
            }
            outPts[k] = {cx, cy, e};
        }

        // Compute minimum distance between adjacent LEDs on same segment for label density
        int minStep = 9999;
        for (int k = 1; k < N; ++k) {
            if (meta[k].segment == meta[k - 1].segment) {
                int d = abs(outPts[k].x - outPts[k - 1].x) + abs(outPts[k].y - outPts[k - 1].y);
                if (d > 0 && d < minStep) minStep = d;
            }
        }
        bool showAllNums = (minStep >= S(18));

        SelectObject(dc, hFontSmall);

        for (int k = 0; k < N; ++k) {
            RGB3 c = {45, 48, 56};
            if (!colors.empty()) {
                size_t ci = std::min((size_t)k, colors.size() - 1);
                c = colors[ci];
            }

            int cx = outPts[k].x;
            int cy = outPts[k].y;

            // Single soft glow circle when lit
            if (c.r > 24 || c.g > 24 || c.b > 24) {
                COLORREF gc = RGB(c.r / 3, c.g / 3, c.b / 3);
                SetDCBrushColor(dc, gc);
                SetDCPenColor(dc, gc);
                Ellipse(dc, cx - glowR, cy - glowR, cx + glowR, cy + glowR);
            }

            // LED core
            SetDCBrushColor(dc, RGB(c.r, c.g, c.b));
            SetDCPenColor(dc, (k == 0) ? CLR_ACCENT : RGB(38, 42, 50));
            Ellipse(dc, cx - ledR, cy - ledR, cx + ledR, cy + ledR);

            // Hover highlight ring
            if (k == hoverLed) {
                int hr = ledR + S(5);
                SelectObject(dc, GetStockObject(NULL_BRUSH));
                SetDCPenColor(dc, CLR_WARN);
                Ellipse(dc, cx - hr, cy - hr, cx + hr, cy + hr);
                SelectObject(dc, GetStockObject(DC_BRUSH));
            }

            // Prominent ring around LED #1
            if (k == 0) {
                int rr = ledR + S(4);
                SelectObject(dc, GetStockObject(NULL_BRUSH));
                SetDCPenColor(dc, RGB(255, 255, 255));
                Ellipse(dc, cx - rr, cy - rr, cx + rr, cy + rr);
                SelectObject(dc, GetStockObject(DC_BRUSH));
            }

            // Number label placed OUTSIDE the LED ring so corner labels never overlap
            bool drawNum = showAllNums || (k == 0) || (k == N - 1) || (k == hoverLed) || (((k + 1) % 5) == 0);
            if (drawNum) {
                wchar_t numBuf[12];
                swprintf_s(numBuf, L"%d", k + 1);
                int nx = cx, ny = cy;
                int outset = ledR + S(11);
                if (outPts[k].edge == 0)      nx -= outset + S(4);
                else if (outPts[k].edge == 1) ny -= outset;
                else if (outPts[k].edge == 2) nx += outset + S(4);
                else                          ny += outset;

                RECT nrc = {nx - S(14), ny - S(8), nx + S(14), ny + S(8)};
                COLORREF numCol = (k == 0) ? CLR_ACCENT : ((k == hoverLed) ? CLR_WARN : CLR_TEXT2);
                SetTextColor(dc, numCol);
                DrawTextW(dc, numBuf, -1, &nrc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }

        // Direction arrow inside monitor edge next to LED #1 pointing toward LED #2
        if (N >= 2) {
            float dx = (float)(outPts[1].x - outPts[0].x);
            float dy = (float)(outPts[1].y - outPts[0].y);
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 1.0f) {
                dx /= len;
                dy /= len;
                float ux = -dy, uy = dx;
                int ix = 0, iy = 0;
                int inOff = ledOff + S(13);
                if (outPts[0].edge == 0)      ix = inOff;
                else if (outPts[0].edge == 1) iy = inOff;
                else if (outPts[0].edge == 2) ix = -inOff;
                else                          iy = -inOff;

                int ax = outPts[0].x + ix;
                int ay = outPts[0].y + iy;
                int tail = S(9), tip = S(9), wing = S(5);

                POINT shaft0 = {ax - (int)(dx * tail), ay - (int)(dy * tail)};
                POINT shaft1 = {ax + (int)(dx * tip),  ay + (int)(dy * tip)};
                POINT w1 = {shaft1.x - (int)(dx * wing - ux * wing), shaft1.y - (int)(dy * wing - uy * wing)};
                POINT w2 = {shaft1.x - (int)(dx * wing + ux * wing), shaft1.y - (int)(dy * wing + uy * wing)};

                SetDCPenColor(dc, CLR_ACCENT);
                MoveToEx(dc, shaft0.x, shaft0.y, NULL);
                LineTo(dc, shaft1.x, shaft1.y);
                LineTo(dc, w1.x, w1.y);
                MoveToEx(dc, shaft1.x, shaft1.y, NULL);
                LineTo(dc, w2.x, w2.y);
            }
        }
    }

    // Bottom thin strip preview
    if (showStrip) {
        int sx = S(16), sy = H - S(28);
        int sw = W - S(32), sh = S(15);
        SetDCBrushColor(dc, RGB(15, 16, 20));
        SetDCPenColor(dc, CLR_BORDER);
        RoundRect(dc, sx - 1, sy - 1, sx + sw + 1, sy + sh + 1, S(6), S(6));

        int n = (int)colors.size();
        if (n > 0 && sw > 0) {
            float cw = (float)sw / (float)n;
            for (int i = 0; i < n; ++i) {
                int x0 = sx + (int)(i * cw);
                int x1 = sx + (int)((i + 1) * cw);
                if (x1 <= x0) x1 = x0 + 1;
                COLORREF col = RGB(colors[i].r, colors[i].g, colors[i].b);
                SetDCBrushColor(dc, col);
                SetDCPenColor(dc, col);
                Rectangle(dc, x0, sy + 1, x1, sy + sh - 1);
            }
        }
    }
}

static inline LRESULT CALLBACK PreviewCanvasProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CREATE) {
        SetPropW(hwnd, L"pbuf", new PreviewBuf());
        return 0;
    }
    if (msg == WM_NCDESTROY) {
        auto* pb = (PreviewBuf*)GetPropW(hwnd, L"pbuf");
        if (pb) {
            pb->Release();
            delete pb;
            RemovePropW(hwnd, L"pbuf");
        }
        RemovePropW(hwnd, L"show_strip");
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        return 1;
    }
    if (msg == WM_MOUSEMOVE) {
        auto* pb = (PreviewBuf*)GetPropW(hwnd, L"pbuf");
        if (pb) {
            int hit = pb->HitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            if (hit != pb->hoverLed) {
                pb->hoverLed = hit;
                TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tme);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    if (msg == WM_MOUSELEAVE) {
        auto* pb = (PreviewBuf*)GetPropW(hwnd, L"pbuf");
        if (pb && pb->hoverLed != -1) {
            pb->hoverLed = -1;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    if (msg == WM_SETCURSOR) {
        auto* pb = (PreviewBuf*)GetPropW(hwnd, L"pbuf");
        if (pb && pb->hoverLed >= 0) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    if (msg == WM_LBUTTONDOWN) {
        auto* pb = (PreviewBuf*)GetPropW(hwnd, L"pbuf");
        if (pb) {
            int hit = pb->HitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            int N = (int)pb->pts.size();
            if (hit >= 0 && N > 0) {
                int newOff = 0;
                {
                    std::lock_guard<std::mutex> lk(g_settings_mtx);
                    newOff = LayoutPosMod(g_settings.led_offset + hit, N);
                    g_settings.led_offset = newOff;
                    MarkLayoutChanged();
                }
                g_suppressSync = true;
                if (hOffsetSpin) SendMessageW(hOffsetSpin, UDM_SETPOS32, 0, newOff);
                g_suppressSync = false;
                pb->hoverLed = 0;
                if (hPreviewBig)   InvalidateRect(hPreviewBig, NULL, FALSE);
                if (hPreviewSmall) InvalidateRect(hPreviewSmall, NULL, FALSE);
            }
        }
        return 0;
    }
    if (msg == WM_RBUTTONDOWN) {
        int newDir = 0, newOff = 0;
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            int oldDir = g_settings.direction;
            newDir = (oldDir == 0) ? 1 : 0;
            if (!(g_settings.start_corner == 4 && g_settings.led_offset == 0)) {
                int L = g_settings.ambi_left, T = g_settings.ambi_top, R = g_settings.ambi_right;
                int BL = g_settings.ambi_bottom_left, BR = g_settings.ambi_bottom_right;
                if (L + T + R + BL + BR <= 0) R = std::max(1, g_settings.led_count);
                newOff = ComputeOffsetToKeepFirstLed(L, T, R, BL, BR, g_settings.start_corner, oldDir, g_settings.led_offset, newDir);
                g_settings.led_offset = newOff;
            }
            g_settings.direction = newDir;
            MarkLayoutChanged();
        }
        g_suppressSync = true;
        if (hOffsetSpin) SendMessageW(hOffsetSpin, UDM_SETPOS32, 0, newOff);
        UpdateDirectionButtons(newDir);
        g_suppressSync = false;
        if (hPreviewBig)   InvalidateRect(hPreviewBig, NULL, FALSE);
        if (hPreviewSmall) InvalidateRect(hPreviewSmall, NULL, FALSE);
        return 0;
    }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int W = rc.right, H = rc.bottom;

        auto* pb = (PreviewBuf*)GetPropW(hwnd, L"pbuf");
        if (pb && W > 0 && H > 0) {
            pb->Ensure(hdc, W, H);
            if (pb->memDC) {
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
                bool showStrip = GetPropW(hwnd, L"show_strip") != NULL;
                DrawMonitorPreview(pb->memDC, W, H, s, colors, showStrip, pb->pts, pb->hoverLed);
                BitBlt(hdc, 0, 0, W, H, pb->memDC, 0, 0, SRCCOPY);
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ─────────────────────────── CUSTOM MODAL DIALOG ────────────────────────────
enum DialogMode {
    DLG_TEXT_EDITOR = 1,
    DLG_ARDUINO_CODE = 2,
};

struct DialogState {
    HWND       parent = NULL;
    DialogMode mode = DLG_TEXT_EDITOR;
    HWND       hEdit = NULL;
    HWND       hBtnPrimary = NULL;
    HWND       hBtnClose = NULL;
    std::wstring codeText;
};

static inline std::wstring GenerateArduinoSketch(const Settings& s) {
    int numLeds = std::max(1, s.led_count);
    std::wostringstream oss;
    oss << L"// ================================================================\r\n";
    oss << L"// ETDAmbilight - Arduino FastLED Adalight Receiver\r\n";
    oss << L"// Baud Rate: " << s.baud << L"  |  LEDs: " << numLeds << L"\r\n";
    oss << L"// Library / Kütüphane: FastLED (Arduino IDE Library Manager)\r\n";
    oss << L"// ================================================================\r\n\r\n";
    oss << L"#include <FastLED.h>\r\n\r\n";
    oss << L"#define NUM_LEDS     " << numLeds << L"\r\n";
    oss << L"#define DATA_PIN     6\r\n";
    oss << L"#define LED_TYPE     WS2812B\r\n";
    oss << L"#define COLOR_ORDER  GRB\r\n";
    oss << L"#define BAUD_RATE    " << s.baud << L"\r\n\r\n";
    oss << L"CRGB leds[NUM_LEDS];\r\n\r\n";
    oss << L"void setup() {\r\n";
    oss << L"  Serial.begin(BAUD_RATE);\r\n";
    oss << L"  Serial.setTimeout(50);\r\n";
    oss << L"  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);\r\n";
    oss << L"  FastLED.setBrightness(255);\r\n";
    oss << L"  FastLED.clear(true);\r\n";
    oss << L"  Serial.print(\"Ada\\n\");\r\n";
    oss << L"}\r\n\r\n";
    oss << L"void loop() {\r\n";
    oss << L"  if (Serial.available() < 6) return;\r\n";
    oss << L"  if (Serial.read() != 'A') return;\r\n";
    oss << L"  if (Serial.read() != 'd') return;\r\n";
    oss << L"  if (Serial.read() != 'a') return;\r\n\r\n";
    oss << L"  uint8_t hi  = Serial.read();\r\n";
    oss << L"  uint8_t lo  = Serial.read();\r\n";
    oss << L"  uint8_t chk = Serial.read();\r\n";
    oss << L"  if (chk != (hi ^ lo ^ 0x55)) return;\r\n\r\n";
    oss << L"  uint16_t count = ((uint16_t)hi << 8 | lo) + 1;\r\n";
    oss << L"  uint16_t readCount = (count <= NUM_LEDS) ? count : NUM_LEDS;\r\n";
    oss << L"  size_t need = (size_t)readCount * 3;\r\n";
    oss << L"  if (Serial.readBytes((char*)leds, need) != need) return;\r\n\r\n";
    oss << L"  // Discard excess bytes if count > NUM_LEDS\r\n";
    oss << L"  for (uint16_t i = readCount; i < count; ++i) {\r\n";
    oss << L"    uint8_t dump[3];\r\n";
    oss << L"    if (Serial.readBytes((char*)dump, 3) != 3) break;\r\n";
    oss << L"  }\r\n\r\n";
    oss << L"  FastLED.show();\r\n";
    oss << L"}\r\n";
    return oss.str();
}

static inline LRESULT CALLBACK DialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = (DialogState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        auto* cs = (CREATESTRUCTW*)lp;
        st = (DialogState*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);
        ApplyWindowChrome(hwnd);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int W = rc.right, H = rc.bottom;

        std::wstring initialText;
        const wchar_t* primaryTitle = L"";
        if (st->mode == DLG_TEXT_EDITOR) {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            initialText = SettingsToText(g_settings);
            primaryTitle = LStr(L"Uygula && EXE'ye Kaydet", L"Apply && Save to EXE");
        } else {
            initialText = st->codeText;
            primaryTitle = LStr(L"Panoya Kopyala", L"Copy to Clipboard");
        }

        DWORD editStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                          ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL;
        if (st->mode == DLG_ARDUINO_CODE) editStyle |= ES_READONLY;

        st->hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", initialText.c_str(), editStyle,
                                    S(12), S(12), W - S(24), H - S(62),
                                    hwnd, (HMENU)200, GetModuleHandleW(NULL), NULL);
        SendMessageW(st->hEdit, WM_SETFONT, (WPARAM)hFontMono, TRUE);
        ApplyDarkTheme(st->hEdit, true);

        st->hBtnPrimary = MakeBtn(hwnd, primaryTitle, 201, CLR_ACCENT);
        SetPropW(st->hBtnPrimary, L"in_topbar", (HANDLE)1);
        SetWindowPos(st->hBtnPrimary, NULL, S(12), H - S(40), S(200), S(30), SWP_NOZORDER);

        st->hBtnClose = MakeBtn(hwnd, LStr(L"Kapat", L"Close"), 202, CLR_CTRL_BG);
        SetPropW(st->hBtnClose, L"in_topbar", (HANDLE)1);
        SetWindowPos(st->hBtnClose, NULL, W - S(112), H - S(40), S(100), S(30), SWP_NOZORDER);
        return 0;
    }

    case WM_SIZE: {
        if (!st) break;
        int W = LOWORD(lp), H = HIWORD(lp);
        if (st->hEdit) SetWindowPos(st->hEdit, NULL, S(12), S(12), W - S(24), H - S(62), SWP_NOZORDER);
        if (st->hBtnPrimary) SetWindowPos(st->hBtnPrimary, NULL, S(12), H - S(40), S(200), S(30), SWP_NOZORDER);
        if (st->hBtnClose) SetWindowPos(st->hBtnClose, NULL, W - S(112), H - S(40), S(100), S(30), SWP_NOZORDER);
        return 0;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wp;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, hBrBg);
        return 1;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, CLR_CARD);
        SetTextColor(hdc, CLR_TEXT);
        return (LRESULT)hBrCard;
    }

    case WM_DRAWITEM:
        DrawModernButton((DRAWITEMSTRUCT*)lp);
        return TRUE;

    case WM_COMMAND: {
        if (!st) break;
        int id = LOWORD(wp);
        if (id == 202) {
            DestroyWindow(hwnd);
        } else if (id == 201) {
            if (st->mode == DLG_TEXT_EDITOR) {
                int len = GetWindowTextLengthW(st->hEdit);
                std::wstring buf(len, L'\0');
                if (len > 0) GetWindowTextW(st->hEdit, buf.data(), len + 1);
                Settings sCopy;
                {
                    std::lock_guard<std::mutex> lk(g_settings_mtx);
                    TextToSettings(buf, g_settings);
                    sCopy = g_settings;
                    MarkLayoutChanged();
                }
                SetAutostart(sCopy.autostart);
                SaveSettings(sCopy, true);
                SyncControlsFromSettings();
                MessageBoxW(hwnd,
                            LStr(L"Ayarlar güncellendi ve EXE içine kaydedildi.",
                                 L"Settings updated and saved into EXE."),
                            APP_NAME, MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hwnd);
            } else {
                if (OpenClipboard(hwnd)) {
                    EmptyClipboard();
                    size_t bytes = (st->codeText.size() + 1) * sizeof(wchar_t);
                    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, bytes);
                    if (hg) {
                        memcpy(GlobalLock(hg), st->codeText.c_str(), bytes);
                        GlobalUnlock(hg);
                        SetClipboardData(CF_UNICODETEXT, hg);
                    }
                    CloseClipboard();
                    MessageBoxW(hwnd,
                                LStr(L"Arduino kodu panoya kopyalandı.",
                                     L"Arduino sketch copied to clipboard."),
                                APP_NAME, MB_OK | MB_ICONINFORMATION);
                }
            }
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (st && st->parent) {
            EnableWindow(st->parent, TRUE);
            SetForegroundWindow(st->parent);
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

static inline void OpenModalDialog(HWND parent, DialogMode mode) {
    auto* st = new DialogState();
    st->parent = parent;
    st->mode = mode;
    if (mode == DLG_ARDUINO_CODE) {
        std::lock_guard<std::mutex> lk(g_settings_mtx);
        st->codeText = GenerateArduinoSketch(g_settings);
    }

    const wchar_t* title = (mode == DLG_TEXT_EDITOR)
        ? LStr(L"ETDAmbilight — Yapılandırma Metin Editörü", L"ETDAmbilight — Configuration Text Editor")
        : LStr(L"ETDAmbilight — Arduino Kodu (FastLED + Adalight)", L"ETDAmbilight — Arduino Sketch (FastLED + Adalight)");

    RECT pr = {};
    GetWindowRect(parent, &pr);
    int dw = S(720), dh = S(580);
    int dx = pr.left + (pr.right - pr.left - dw) / 2;
    int dy = pr.top + (pr.bottom - pr.top - dh) / 2;

    HWND dlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"ETDAmbilightDialog", title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        dx, dy, dw, dh,
        parent, NULL, GetModuleHandleW(NULL), st);

    if (dlg) {
        EnableWindow(parent, FALSE);
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
    } else {
        delete st;
    }
}

static inline void ShowCH340Helper(HWND parent) {
    auto ports = ScanPorts();
    std::wstring detected = AutoDetectArduino(ports);
    if (!detected.empty()) {
        std::wstring msg = std::wstring(LStr(L"Arduino / USB-Seri Port aygıtı tespit edildi:\n\nPort: ",
                                             L"Arduino / USB-Serial Port device detected:\n\nPort: "))
                           + detected
                           + LStr(L"\nSürücü hazır ve bağlantıya uygun.",
                                  L"\nDriver is ready for connection.");
        MessageBoxW(parent, msg.c_str(), LStr(L"CH340 / Port Durumu", L"CH340 / Port Status"), MB_OK | MB_ICONINFORMATION);
    } else {
        const wchar_t* msg = LStr(
            L"Herhangi bir CH340 veya Arduino portu algılanamadı.\n\n"
            L"1. USB kablosunun veri aktardığından emin olun.\n"
            L"2. Klon Arduino kartları için CH340 sürücüsü gerekir.\n\n"
            L"Resmi WCH CH341SER sürücü indirme sayfasını açmak ister misiniz?",
            L"No CH340 or Arduino port detected.\n\n"
            L"1. Make sure your USB cable supports data transfer.\n"
            L"2. Clone Arduino boards require the CH340 USB-Serial driver.\n\n"
            L"Open the official WCH CH341SER driver download page?");
        if (MessageBoxW(parent, msg, LStr(L"CH340 Sürücü Yardımı", L"CH340 Driver Help"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            ShellExecuteW(NULL, L"open", L"https://www.wch-ic.com/downloads/CH341SER_EXE.html",
                          NULL, NULL, SW_SHOWNORMAL);
        }
    }
}

// ─────────────────────────── TRAY & VISIBILITY ──────────────────────────────
static inline void AddOrUpdateTrayIcon(HWND hwnd) {
    if (!g_hSmallIcon) return;
    g_nid = {};
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_hSmallIcon;
    wcsncpy_s(g_nid.szTip,
              LStr(L"ETDAmbilight — Ambilight Kontrolcüsü", L"ETDAmbilight — Ambilight Controller"),
              _TRUNCATE);
    if (Shell_NotifyIconW(g_trayAdded ? NIM_MODIFY : NIM_ADD, &g_nid)) {
        g_trayAdded = true;
    }
}

static inline void RemoveTrayIcon() {
    if (g_trayAdded) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_trayAdded = false;
    }
}

static inline void HideToTray(HWND hwnd) {
    KillTimer(hwnd, 1);
    g_uiVisible.store(false, std::memory_order_relaxed);
    ShowWindow(hwnd, SW_HIDE);
    if (g_dirty.load(std::memory_order_relaxed)) {
        Settings sCopy;
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            sCopy = g_settings;
        }
        SaveSettings(sCopy, false);
    }
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
}

static inline void ShowFromTray(HWND hwnd) {
    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    g_uiVisible.store(true, std::memory_order_relaxed);
    SetTimer(hwnd, 1, 33, NULL);
    InvalidateRect(hwnd, NULL, FALSE);
}

static inline void ShowTrayMenu(HWND hwnd) {
    HMENU hMenu = CreatePopupMenu();
    HMENU hEffSub = CreatePopupMenu();

    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settings_mtx);
        s = g_settings;
    }

    for (int i = 0; i < EFFECT_COUNT; ++i) {
        UINT flags = MF_STRING | ((s.effect == i) ? MF_CHECKED : 0);
        AppendMenuW(hEffSub, flags, ID_TRAY_EFFECT_BASE + i, EffectName(i));
    }

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SHOW, LStr(L"Göster", L"Show"));
    AppendMenuW(hMenu, MF_STRING | (s.leds_enabled ? MF_CHECKED : 0),
                ID_TRAY_TOGGLE_LEDS, LStr(L"LED'ler Açık", L"LEDs Enabled"));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hEffSub, LStr(L"Efekt", L"Effect"));
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, LStr(L"Çıkış", L"Exit"));

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

// ─────────────────────────── PORT & MONITOR COMBOS ──────────────────────────
static inline void PopulatePortCombo() {
    g_ports = ScanPorts();
    SendMessageW(hPortCB, CB_RESETCONTENT, 0, 0);
    SendMessageW(hPortCB, CB_ADDSTRING, 0, (LPARAM)L"AUTO");
    for (const auto& p : g_ports) {
        wchar_t buf[160];
        swprintf_s(buf, L"%s — %s", p.name.c_str(), p.desc.c_str());
        SendMessageW(hPortCB, CB_ADDSTRING, 0, (LPARAM)buf);
    }

    std::wstring curPort;
    {
        std::lock_guard<std::mutex> lk(g_settings_mtx);
        curPort = g_settings.port;
    }
    if (curPort == L"AUTO") {
        SendMessageW(hPortCB, CB_SETCURSEL, 0, 0);
    } else {
        int sel = 0;
        for (int i = 0; i < (int)g_ports.size(); ++i) {
            if (g_ports[i].name == curPort) { sel = i + 1; break; }
        }
        SendMessageW(hPortCB, CB_SETCURSEL, sel, 0);
    }
}

static inline void PopulateMonitorCombo() {
    g_monitors = EnumerateMonitors();
    SendMessageW(hMonCB, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < (int)g_monitors.size(); ++i) {
        const auto& m = g_monitors[i];
        wchar_t buf[96];
        swprintf_s(buf, L"%d — %d×%d%s", i + 1, m.width(), m.height(),
                   m.primary ? LStr(L" (Birincil)", L" (Primary)") : L"");
        SendMessageW(hMonCB, CB_ADDSTRING, 0, (LPARAM)buf);
    }
    int idx = 0;
    {
        std::lock_guard<std::mutex> lk(g_settings_mtx);
        idx = std::clamp(g_settings.monitor_idx, 0, std::max(0, (int)g_monitors.size() - 1));
    }
    SendMessageW(hMonCB, CB_SETCURSEL, idx, 0);
}

static inline void UpdateConnectButton() {
    if (!hConnectBtn) return;
    bool conn = g_connected.load();
    SetBtnBg(hConnectBtn, conn ? RGB(170, 52, 52) : CLR_ACCENT);
    SetWindowTextW(hConnectBtn, conn ? LStr(L"Bağlantıyı Kes", L"Disconnect") : LStr(L"Bağlan", L"Connect"));
    InvalidateRect(hConnectBtn, NULL, FALSE);
}

static inline void UpdateDirectionButtons(int dir) {
    if (!hBtnDirCW || !hBtnDirCCW) return;
    SetBtnBg(hBtnDirCW,  (dir == 0) ? CLR_ACCENT : CLR_CTRL_BG);
    SetBtnBg(hBtnDirCCW, (dir == 1) ? CLR_ACCENT : CLR_CTRL_BG);
    InvalidateRect(hBtnDirCW, NULL, FALSE);
    InvalidateRect(hBtnDirCCW, NULL, FALSE);
}

static inline void UpdateLabelsFromSettings(const Settings& s) {
    wchar_t buf[64];
    swprintf_s(buf, L"%d%%", s.brightness);
    SetWindowTextW(hBrightVal, buf);
    swprintf_s(buf, L"%d", s.effect_speed);
    SetWindowTextW(hSpeedVal, buf);
    swprintf_s(buf, LStr(L"Toplam: %d LED", L"Total: %d LEDs"), std::max(1, s.led_count));
    SetWindowTextW(hLblTotalLeds, buf);
    swprintf_s(buf, L"%%%d", s.bottom_gap_pct);
    SetWindowTextW(hGapVal, buf);
    swprintf_s(buf, L"%%%d", s.edge_depth_pct);
    SetWindowTextW(hDepthVal, buf);
    swprintf_s(buf, L"%.1fx", s.saturation);
    SetWindowTextW(hSatVal, buf);
    swprintf_s(buf, L"%.2f", s.smoothing);
    SetWindowTextW(hSmoothVal, buf);
}

static inline void SyncControlsFromSettings() {
    g_suppressSync = true;
    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settings_mtx);
        s = g_settings;
    }

    PopulatePortCombo();
    PopulateMonitorCombo();

    DWORD bauds[] = {9600, 57600, 115200, 500000, 1000000, 2000000};
    int bSel = 4;
    for (int i = 0; i < 6; ++i) if (bauds[i] == s.baud) { bSel = i; break; }
    SendMessageW(hBaudCB, CB_SETCURSEL, bSel, 0);

    SendMessageW(hBrightSlider, TBM_SETPOS, TRUE, s.brightness);

    int fpsOpts[] = {30, 60, 90, 120, 144, 180};
    int fSel = 1;
    for (int i = 0; i < 6; ++i) if (fpsOpts[i] == s.target_fps) { fSel = i; break; }
    SendMessageW(hFpsCB, CB_SETCURSEL, fSel, 0);

    float gammas[] = {1.0f, 1.8f, 2.0f, 2.2f, 2.4f};
    int gSel = 3;
    float bestDiff = 99.0f;
    for (int i = 0; i < 5; ++i) {
        float d = fabsf(gammas[i] - s.gamma);
        if (d < bestDiff) { bestDiff = d; gSel = i; }
    }
    SendMessageW(hGammaCB, CB_SETCURSEL, gSel, 0);

    SendMessageW(hEffectCB, CB_SETCURSEL, std::clamp(s.effect, 0, EFFECT_COUNT - 1), 0);
    SendMessageW(hSpeedSlider, TBM_SETPOS, TRUE, s.effect_speed);
    SetBtnColorSwatch(hColor1Btn, s.color1);
    SetBtnColorSwatch(hColor2Btn, s.color2);
    InvalidateRect(hColor1Btn, NULL, FALSE);
    InvalidateRect(hColor2Btn, NULL, FALSE);

    int edges[5] = {s.ambi_left, s.ambi_top, s.ambi_right, s.ambi_bottom_left, s.ambi_bottom_right};
    for (int i = 0; i < 5; ++i) {
        SendMessageW(hAmbiSpin[i], UDM_SETPOS32, 0, edges[i]);
    }
    SendMessageW(hGapSlider, TBM_SETPOS, TRUE, s.bottom_gap_pct);
    SendMessageW(hCornerCB, CB_SETCURSEL, std::clamp(s.start_corner, 0, 4), 0);
    UpdateDirectionButtons(std::clamp(s.direction, 0, 1));
    SendMessageW(hOffsetSpin, UDM_SETPOS32, 0, s.led_offset);
    SendMessageW(hDepthSlider, TBM_SETPOS, TRUE, s.edge_depth_pct);
    SendMessageW(hSatSlider, TBM_SETPOS, TRUE, (int)(s.saturation * 10.0f + 0.5f));
    SendMessageW(hSmoothSlider, TBM_SETPOS, TRUE, (int)(s.smoothing * 100.0f + 0.5f));

    SetToggleState(hToggleLeds, s.leds_enabled);
    SetToggleState(hToggleCalib, s.effect == 11);
    SetToggleState(hToggleAutostart, s.autostart);
    SetToggleState(hToggleStartHidden, s.start_hidden);
    SetToggleState(hToggleCloseTray, s.close_to_tray);
    SendMessageW(hCaptureCB, CB_SETCURSEL, std::clamp(s.capture_method, 0, 1), 0);
    if (hLangCB) SendMessageW(hLangCB, CB_SETCURSEL, CurrentLang(), 0);

    UpdateLabelsFromSettings(s);
    UpdateConnectButton();
    g_suppressSync = false;
}

static inline void PopulateLocalizedCombos(const Settings& s) {
    SendMessageW(hEffectCB, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < EFFECT_COUNT; ++i)
        SendMessageW(hEffectCB, CB_ADDSTRING, 0, (LPARAM)EffectName(i));
    SendMessageW(hEffectCB, CB_SETCURSEL, std::clamp(s.effect, 0, EFFECT_COUNT - 1), 0);

    SendMessageW(hCornerCB, CB_RESETCONTENT, 0, 0);
    const wchar_t* corners[5] = {
        LStr(L"Sol-Alt", L"Bottom-Left"),
        LStr(L"Sol-Üst", L"Top-Left"),
        LStr(L"Sağ-Üst", L"Top-Right"),
        LStr(L"Sağ-Alt", L"Bottom-Right"),
        LStr(L"Alt-Ayak (Orta)", L"Bottom-Stand (Center)")
    };
    for (auto c : corners) SendMessageW(hCornerCB, CB_ADDSTRING, 0, (LPARAM)c);
    SendMessageW(hCornerCB, CB_SETCURSEL, std::clamp(s.start_corner, 0, 4), 0);

    SendMessageW(hCaptureCB, CB_RESETCONTENT, 0, 0);
    SendMessageW(hCaptureCB, CB_ADDSTRING, 0, (LPARAM)LStr(L"Otomatik (DXGI)", L"Auto (DXGI)"));
    SendMessageW(hCaptureCB, CB_ADDSTRING, 0, (LPARAM)LStr(L"GDI (Uyumluluk)", L"GDI (Compatibility)"));
    SendMessageW(hCaptureCB, CB_SETCURSEL, std::clamp(s.capture_method, 0, 1), 0);

    PopulateMonitorCombo();
}

static inline void ApplyLanguageToUI(HWND hwnd) {
    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settings_mtx);
        s = g_settings;
    }
    g_suppressSync = true;

    SetWindowTextW(hwnd, LStr(L"ETDAmbilight — Adalight Ambilight Kontrolcüsü",
                              L"ETDAmbilight — Adalight Ambilight Controller"));
    AddOrUpdateTrayIcon(hwnd);

    SetWindowTextW(hLblPort, LStr(L"COM Port", L"COM Port"));
    SetWindowTextW(hBtnRefresh, LStr(L"Yenile", L"Refresh"));
    SetWindowTextW(hLblBaud, LStr(L"Baud Rate", L"Baud Rate"));
    UpdateConnectButton();

    SetWindowTextW(hLblBright, LStr(L"Parlaklık", L"Brightness"));
    SetWindowTextW(hLblFps, LStr(L"Hedef FPS", L"Target FPS"));
    SetWindowTextW(hLblGamma, L"Gamma");

    SetWindowTextW(hLblEffect, LStr(L"Efekt", L"Effect"));
    SetWindowTextW(hLblSpeed, LStr(L"Hız", L"Speed"));
    SetWindowTextW(hLblColor1, LStr(L"Ana Renk", L"Primary Color"));
    SetWindowTextW(hLblColor2, LStr(L"İkincil Renk", L"Secondary Color"));
    SetWindowTextW(hBtnColorWiz, LStr(L"Renk Sihirbazı", L"Color Wizard"));

    const wchar_t* edgeNames[5] = {
        LStr(L"Sol", L"Left"),
        LStr(L"Üst", L"Top"),
        LStr(L"Sağ", L"Right"),
        LStr(L"Alt-Sol", L"Bot-Left"),
        LStr(L"Alt-Sağ", L"Bot-Right")
    };
    for (int i = 0; i < 5; ++i) SetWindowTextW(hAmbiLbl[i], edgeNames[i]);
    SetWindowTextW(hLblGap, LStr(L"Alt Boşluk", L"Stand Gap"));
    SetWindowTextW(hLblCorner, LStr(L"Başlangıç", L"Start Pos"));
    SetWindowTextW(hLblDir, LStr(L"Yön", L"Direction"));
    SetWindowTextW(hBtnDirCW, LStr(L"← Sola", L"← Left"));
    SetWindowTextW(hBtnDirCCW, LStr(L"Sağa →", L"Right →"));
    SetWindowTextW(hLblOffset, LStr(L"Kaydırma", L"Offset"));
    SetWindowTextW(hLblDepth, LStr(L"Derinlik", L"Depth"));
    SetWindowTextW(hLblSat, LStr(L"Doygunluk", L"Saturation"));
    SetWindowTextW(hLblSmooth, LStr(L"Yumuşatma", L"Smoothing"));
    SetWindowTextW(hLblMon, LStr(L"Monitör", L"Monitor"));
    SetWindowTextW(hToggleCalib, LStr(L"Yön Testi (Kalibrasyon)", L"Direction Test (Calibration)"));

    SetWindowTextW(hToggleAutostart, LStr(L"Windows ile otomatik başlat", L"Start automatically with Windows"));
    SetWindowTextW(hToggleStartHidden, LStr(L"Başlangıçta tepside gizli başla", L"Start minimized to system tray"));
    SetWindowTextW(hToggleCloseTray, LStr(L"Kapatınca sistem tepsisine küçült", L"Minimize to tray on close"));
    SetWindowTextW(hLblCapture, LStr(L"Yakalama Yöntemi", L"Capture Method"));
    SetWindowTextW(hLblLang, LStr(L"Dil / Language", L"Language / Dil"));

    const wchar_t* infoTitles[5] = {
        LStr(L"Gerçek FPS:", L"Actual FPS:"),
        LStr(L"Seri Port:", L"Serial Port:"),
        LStr(L"Yakalama:", L"Capture:"),
        LStr(L"LED Sayısı:", L"LED Count:"),
        LStr(L"Aktif Efekt:", L"Active Effect:")
    };
    for (int i = 0; i < 5; ++i) SetWindowTextW(hInfoTitle[i], infoTitles[i]);

    SetWindowTextW(hBtnSave, LStr(L"EXE'ye Kaydet", L"Save to EXE"));
    SetWindowTextW(hBtnTextEdit, LStr(L"Metin Editörü", L"Config Editor"));
    SetWindowTextW(hBtnArduino, LStr(L"Arduino Kodu", L"Arduino Code"));
    SetWindowTextW(hBtnCh340, LStr(L"CH340 Yardım", L"CH340 Driver"));
    SetWindowTextW(hBtnUpdate, g_updateCheckBusy
        ? LStr(L"Denetleniyor...", L"Checking...")
        : LStr(L"Güncelleme Denetle", L"Check Updates"));
    SetWindowTextW(hLblCredit, LStr(L"ETDAmbilight v2.0  •  Geliştirici: Emir Tuğra Dağ  •  MIT Kod Lisansı",
                                    L"ETDAmbilight v2.0  •  Developer: Emir Tuğra Dağ  •  MIT Code License"));

    PopulateLocalizedCombos(s);
    UpdateLabelsFromSettings(s);
    g_suppressSync = false;

    InvalidateRect(hwnd, NULL, TRUE);
    if (hPreviewBig) InvalidateRect(hPreviewBig, NULL, FALSE);
    if (hPreviewSmall) InvalidateRect(hPreviewSmall, NULL, FALSE);
}

// ─────────────────────────── PAGE SWITCHING ─────────────────────────────────
static inline void SwitchTab(HWND hwnd, int tab) {
    g_activeTab = std::clamp(tab, 0, 2);
    for (int p = 0; p < 3; ++p) {
        int cmd = (p == g_activeTab) ? SW_SHOW : SW_HIDE;
        for (HWND h : g_pages[p]) {
            if (h) ShowWindow(h, cmd);
        }
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

// ─────────────────────────── CREATE & LAYOUT CONTROLS ───────────────────────
static inline void CreateAllControls(HWND hwnd) {
    // Top bar toggle
    hToggleLeds = MakeToggle(hwnd, L"LED", ID_TOGGLE_LEDS, g_settings.leds_enabled);
    SetPropW(hToggleLeds, L"in_topbar", (HANDLE)1);

    // Left Card 1: Bağlantı
    hLblPort = MakeLbl(hwnd, LStr(L"COM Port", L"COM Port"), CLR_TEXT2);
    hPortCB  = MakeCombo(hwnd, ID_COMBO_PORT);
    hBtnRefresh = MakeBtn(hwnd, LStr(L"Yenile", L"Refresh"), ID_BTN_REFRESH);
    hLblBaud = MakeLbl(hwnd, LStr(L"Baud Rate", L"Baud Rate"), CLR_TEXT2);
    hBaudCB  = MakeCombo(hwnd, ID_COMBO_BAUD);
    const wchar_t* bauds[] = {L"9600", L"57600", L"115200", L"500000", L"1000000", L"2000000"};
    for (auto b : bauds) SendMessageW(hBaudCB, CB_ADDSTRING, 0, (LPARAM)b);
    hConnectBtn = MakeBtn(hwnd, LStr(L"Bağlan", L"Connect"), ID_BTN_CONNECT, CLR_ACCENT);

    // Left Card 2: Genel
    hLblBright = MakeLbl(hwnd, LStr(L"Parlaklık", L"Brightness"), CLR_TEXT2);
    hBrightSlider = MakeSlider(hwnd, ID_SLIDER_BRIGHT, 0, 200, std::clamp(g_settings.brightness, 0, 200));
    hBrightVal = MakeLbl(hwnd, L"100%", CLR_SUCCESS, hFontSemi);
    hLblFps = MakeLbl(hwnd, LStr(L"Hedef FPS", L"Target FPS"), CLR_TEXT2);
    hFpsCB  = MakeCombo(hwnd, ID_COMBO_FPS);
    const wchar_t* fpsOpts[] = {L"30", L"60", L"90", L"120", L"144", L"180"};
    for (auto f : fpsOpts) SendMessageW(hFpsCB, CB_ADDSTRING, 0, (LPARAM)f);
    hLblGamma = MakeLbl(hwnd, L"Gamma", CLR_TEXT2);
    hGammaCB  = MakeCombo(hwnd, ID_COMBO_GAMMA);
    const wchar_t* gammas[] = {L"1.0", L"1.8", L"2.0", L"2.2", L"2.4"};
    for (auto g : gammas) SendMessageW(hGammaCB, CB_ADDSTRING, 0, (LPARAM)g);

    // Left Card 3: Efekt
    hLblEffect = MakeLbl(hwnd, LStr(L"Efekt", L"Effect"), CLR_TEXT2);
    hEffectCB  = MakeCombo(hwnd, ID_COMBO_EFFECT);
    for (int i = 0; i < EFFECT_COUNT; ++i)
        SendMessageW(hEffectCB, CB_ADDSTRING, 0, (LPARAM)EffectName(i));
    hLblSpeed = MakeLbl(hwnd, LStr(L"Hız", L"Speed"), CLR_TEXT2);
    hSpeedSlider = MakeSlider(hwnd, ID_SLIDER_SPEED, 1, 100, g_settings.effect_speed);
    hSpeedVal = MakeLbl(hwnd, L"50", CLR_SUCCESS, hFontSemi);
    hLblColor1 = MakeLbl(hwnd, LStr(L"Ana Renk", L"Primary Color"), CLR_TEXT2);
    hColor1Btn = MakeBtn(hwnd, L"", ID_BTN_COLOR1);
    SetBtnColorSwatch(hColor1Btn, g_settings.color1);
    hLblColor2 = MakeLbl(hwnd, LStr(L"İkincil Renk", L"Secondary Color"), CLR_TEXT2);
    hColor2Btn = MakeBtn(hwnd, L"", ID_BTN_COLOR2);
    SetBtnColorSwatch(hColor2Btn, g_settings.color2);
    hBtnColorWiz = MakeBtn(hwnd, LStr(L"Renk Sihirbazı", L"Color Wizard"), ID_BTN_COLORWIZ);

    // ─── Page 0: Önizleme ─────────────────────────────────────────────────
    hPreviewBig = CreateWindowExW(0, L"ETDAmbilightPreviewCanvas", NULL, WS_CHILD | WS_VISIBLE,
                                  0, 0, 100, 100, hwnd, (HMENU)ID_PREVIEW_BIG, GetModuleHandleW(NULL), NULL);
    SetPropW(hPreviewBig, L"show_strip", (HANDLE)1);
    g_pages[0].push_back(hPreviewBig);

    // ─── Page 1: Ambilight Düzeni ─────────────────────────────────────────
    const wchar_t* edgeNames[5] = {
        LStr(L"Sol", L"Left"),
        LStr(L"Üst", L"Top"),
        LStr(L"Sağ", L"Right"),
        LStr(L"Alt-Sol", L"Bot-Left"),
        LStr(L"Alt-Sağ", L"Bot-Right")
    };
    int editIds[5] = {ID_EDIT_AMBI_L, ID_EDIT_AMBI_T, ID_EDIT_AMBI_R, ID_EDIT_AMBI_BL, ID_EDIT_AMBI_BR};
    int spinIds[5] = {ID_SPIN_AMBI_L, ID_SPIN_AMBI_T, ID_SPIN_AMBI_R, ID_SPIN_AMBI_BL, ID_SPIN_AMBI_BR};
    int edgeVals[5] = {g_settings.ambi_left, g_settings.ambi_top, g_settings.ambi_right,
                       g_settings.ambi_bottom_left, g_settings.ambi_bottom_right};

    for (int i = 0; i < 5; ++i) {
        hAmbiLbl[i] = MakeLbl(hwnd, edgeNames[i], CLR_TEXT2);
        MakeSpinnerPair(hwnd, editIds[i], spinIds[i], 0, 300, edgeVals[i], hAmbiEdit[i], hAmbiSpin[i]);
        g_pages[1].push_back(hAmbiLbl[i]);
        g_pages[1].push_back(hAmbiEdit[i]);
        g_pages[1].push_back(hAmbiSpin[i]);
    }

    hLblTotalLeds = MakeLbl(hwnd, LStr(L"Toplam: 60 LED", L"Total: 60 LEDs"), CLR_SUCCESS, hFontSemi);
    g_pages[1].push_back(hLblTotalLeds);

    hLblGap = MakeLbl(hwnd, LStr(L"Alt Boşluk", L"Stand Gap"), CLR_TEXT2);
    hGapSlider = MakeSlider(hwnd, ID_SLIDER_GAP, 0, 65, g_settings.bottom_gap_pct);
    hGapVal = MakeLbl(hwnd, L"%0", CLR_SUCCESS, hFontSemi);
    g_pages[1].push_back(hLblGap);
    g_pages[1].push_back(hGapSlider);
    g_pages[1].push_back(hGapVal);

    hLblCorner = MakeLbl(hwnd, LStr(L"Başlangıç", L"Start Pos"), CLR_TEXT2);
    hCornerCB  = MakeCombo(hwnd, ID_COMBO_CORNER);
    const wchar_t* corners[] = {
        LStr(L"Sol-Alt", L"Bottom-Left"),
        LStr(L"Sol-Üst", L"Top-Left"),
        LStr(L"Sağ-Üst", L"Top-Right"),
        LStr(L"Sağ-Alt", L"Bottom-Right"),
        LStr(L"Alt-Ayak (Orta)", L"Bottom-Stand (Center)")
    };
    for (auto c : corners) SendMessageW(hCornerCB, CB_ADDSTRING, 0, (LPARAM)c);
    g_pages[1].push_back(hLblCorner);
    g_pages[1].push_back(hCornerCB);

    hLblDir = MakeLbl(hwnd, LStr(L"Yön", L"Direction"), CLR_TEXT2);
    hBtnDirCW  = MakeBtn(hwnd, LStr(L"← Sola", L"← Left"), ID_BTN_DIR_CW, CLR_CTRL_BG);
    hBtnDirCCW = MakeBtn(hwnd, LStr(L"Sağa →", L"Right →"), ID_BTN_DIR_CCW, CLR_ACCENT);
    g_pages[1].push_back(hLblDir);
    g_pages[1].push_back(hBtnDirCW);
    g_pages[1].push_back(hBtnDirCCW);

    hLblOffset = MakeLbl(hwnd, LStr(L"Kaydırma", L"Offset"), CLR_TEXT2);
    MakeSpinnerPair(hwnd, ID_EDIT_OFFSET, ID_SPIN_OFFSET, -500, 500, g_settings.led_offset, hOffsetEdit, hOffsetSpin);
    g_pages[1].push_back(hLblOffset);
    g_pages[1].push_back(hOffsetEdit);
    g_pages[1].push_back(hOffsetSpin);

    hLblDepth = MakeLbl(hwnd, LStr(L"Derinlik", L"Depth"), CLR_TEXT2);
    hDepthSlider = MakeSlider(hwnd, ID_SLIDER_DEPTH, 2, 30, g_settings.edge_depth_pct);
    hDepthVal = MakeLbl(hwnd, L"%8", CLR_SUCCESS, hFontSemi);
    g_pages[1].push_back(hLblDepth);
    g_pages[1].push_back(hDepthSlider);
    g_pages[1].push_back(hDepthVal);

    hLblSat = MakeLbl(hwnd, LStr(L"Doygunluk", L"Saturation"), CLR_TEXT2);
    hSatSlider = MakeSlider(hwnd, ID_SLIDER_SAT, 5, 50, std::clamp((int)(g_settings.saturation * 10.0f + 0.5f), 5, 50));
    hSatVal = MakeLbl(hwnd, L"1.0x", CLR_SUCCESS, hFontSemi);
    g_pages[1].push_back(hLblSat);
    g_pages[1].push_back(hSatSlider);
    g_pages[1].push_back(hSatVal);

    hLblSmooth = MakeLbl(hwnd, LStr(L"Yumuşatma", L"Smoothing"), CLR_TEXT2);
    hSmoothSlider = MakeSlider(hwnd, ID_SLIDER_SMOOTH, 0, 95, (int)(g_settings.smoothing * 100));
    hSmoothVal = MakeLbl(hwnd, L"0.30", CLR_SUCCESS, hFontSemi);
    g_pages[1].push_back(hLblSmooth);
    g_pages[1].push_back(hSmoothSlider);
    g_pages[1].push_back(hSmoothVal);

    hLblMon = MakeLbl(hwnd, LStr(L"Monitör", L"Monitor"), CLR_TEXT2);
    hMonCB  = MakeCombo(hwnd, ID_COMBO_MON);
    g_pages[1].push_back(hLblMon);
    g_pages[1].push_back(hMonCB);

    hToggleCalib = MakeToggle(hwnd, LStr(L"Yön Testi (Kalibrasyon)", L"Direction Test (Calibration)"),
                              ID_TOGGLE_CALIB, g_settings.effect == 11);
    g_pages[1].push_back(hToggleCalib);

    hPreviewSmall = CreateWindowExW(0, L"ETDAmbilightPreviewCanvas", NULL, WS_CHILD,
                                    0, 0, 100, 100, hwnd, (HMENU)ID_PREVIEW_SMALL, GetModuleHandleW(NULL), NULL);
    g_pages[1].push_back(hPreviewSmall);

    // ─── Page 2: Sistem ───────────────────────────────────────────────────
    hToggleAutostart   = MakeToggle(hwnd, LStr(L"Windows ile otomatik başlat", L"Start automatically with Windows"),
                                    ID_TOGGLE_AUTOSTART, g_settings.autostart);
    hToggleStartHidden = MakeToggle(hwnd, LStr(L"Başlangıçta tepside gizli başla", L"Start minimized to system tray"),
                                    ID_TOGGLE_STARTHIDDEN, g_settings.start_hidden);
    hToggleCloseTray   = MakeToggle(hwnd, LStr(L"Kapatınca sistem tepsisine küçült", L"Minimize to tray on close"),
                                    ID_TOGGLE_CLOSETRAY, g_settings.close_to_tray);
    g_pages[2].push_back(hToggleAutostart);
    g_pages[2].push_back(hToggleStartHidden);
    g_pages[2].push_back(hToggleCloseTray);

    hLblCapture = MakeLbl(hwnd, LStr(L"Yakalama Yöntemi", L"Capture Method"), CLR_TEXT2);
    hCaptureCB  = MakeCombo(hwnd, ID_COMBO_CAPTURE);
    SendMessageW(hCaptureCB, CB_ADDSTRING, 0, (LPARAM)LStr(L"Otomatik (DXGI)", L"Auto (DXGI)"));
    SendMessageW(hCaptureCB, CB_ADDSTRING, 0, (LPARAM)LStr(L"GDI (Uyumluluk)", L"GDI (Compatibility)"));
    g_pages[2].push_back(hLblCapture);
    g_pages[2].push_back(hCaptureCB);

    hLblLang = MakeLbl(hwnd, LStr(L"Dil / Language", L"Language / Dil"), CLR_TEXT2);
    hLangCB  = MakeCombo(hwnd, ID_COMBO_LANG);
    SendMessageW(hLangCB, CB_ADDSTRING, 0, (LPARAM)L"Türkçe");
    SendMessageW(hLangCB, CB_ADDSTRING, 0, (LPARAM)L"English");
    g_pages[2].push_back(hLblLang);
    g_pages[2].push_back(hLangCB);

    const wchar_t* infoTitles[5] = {
        LStr(L"Gerçek FPS:", L"Actual FPS:"),
        LStr(L"Seri Port:", L"Serial Port:"),
        LStr(L"Yakalama:", L"Capture:"),
        LStr(L"LED Sayısı:", L"LED Count:"),
        LStr(L"Aktif Efekt:", L"Active Effect:")
    };
    for (int i = 0; i < 5; ++i) {
        hInfoTitle[i] = MakeLbl(hwnd, infoTitles[i], CLR_TEXT2);
        hInfoVal[i]   = MakeLbl(hwnd, L"--", CLR_SUCCESS, hFontMono);
        g_pages[2].push_back(hInfoTitle[i]);
        g_pages[2].push_back(hInfoVal[i]);
    }

    hBtnSave     = MakeBtn(hwnd, LStr(L"EXE'ye Kaydet", L"Save to EXE"), ID_BTN_SAVE, CLR_ACCENT);
    hBtnTextEdit = MakeBtn(hwnd, LStr(L"Metin Editörü", L"Config Editor"), ID_BTN_TEXTEDIT);
    hBtnArduino  = MakeBtn(hwnd, LStr(L"Arduino Kodu", L"Arduino Code"), ID_BTN_ARDUINO);
    hBtnCh340    = MakeBtn(hwnd, LStr(L"CH340 Yardım", L"CH340 Driver"), ID_BTN_CH340);
    hBtnUpdate   = MakeBtn(hwnd, LStr(L"Güncelleme Denetle", L"Check Updates"), ID_BTN_UPDATE);
    hLblCredit   = MakeLbl(hwnd,
                           LStr(L"ETDAmbilight v2.0  •  Geliştirici: Emir Tuğra Dağ  •  MIT Kod Lisansı",
                                L"ETDAmbilight v2.0  •  Developer: Emir Tuğra Dağ  •  MIT Code License"),
                           CLR_TEXT2, hFontSmall);
    SetPropW(hLblCredit, L"in_topbar", (HANDLE)1);
    g_pages[2].push_back(hBtnSave);
    g_pages[2].push_back(hBtnTextEdit);
    g_pages[2].push_back(hBtnArduino);
    g_pages[2].push_back(hBtnCh340);
    g_pages[2].push_back(hBtnUpdate);
    g_pages[2].push_back(hLblCredit);

    SyncControlsFromSettings();
    SwitchTab(hwnd, 0);
}

static inline void UpdateFontsOnAllChildren(HWND hwnd) {
    SendMessageW(hToggleLeds, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLblPort, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hPortCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hBtnRefresh, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblBaud, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hBaudCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hConnectBtn, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblBright, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hBrightVal, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblFps, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hFpsCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLblGamma, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hGammaCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLblEffect, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hEffectCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLblSpeed, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hSpeedVal, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblColor1, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLblColor2, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hBtnColorWiz, WM_SETFONT, (WPARAM)hFontSemi, TRUE);

    for (int i = 0; i < 5; ++i) {
        SendMessageW(hAmbiLbl[i], WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hAmbiEdit[i], WM_SETFONT, (WPARAM)hFontMono, TRUE);
    }
    SendMessageW(hLblTotalLeds, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblGap, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hGapVal, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblCorner, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hCornerCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLblDir, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hBtnDirCW, WM_SETFONT, (WPARAM)hFontSmall, TRUE);
    SendMessageW(hBtnDirCCW, WM_SETFONT, (WPARAM)hFontSmall, TRUE);
    SendMessageW(hLblOffset, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hOffsetEdit, WM_SETFONT, (WPARAM)hFontMono, TRUE);
    SendMessageW(hLblDepth, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hDepthVal, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblSat, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hSatVal, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblSmooth, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hSmoothVal, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblMon, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hMonCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hToggleCalib, WM_SETFONT, (WPARAM)hFontUI, TRUE);

    SendMessageW(hToggleAutostart, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hToggleStartHidden, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hToggleCloseTray, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLblCapture, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hCaptureCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLblLang, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    SendMessageW(hLangCB, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    for (int i = 0; i < 5; ++i) {
        SendMessageW(hInfoTitle[i], WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hInfoVal[i], WM_SETFONT, (WPARAM)hFontMono, TRUE);
    }
    SendMessageW(hBtnSave, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hBtnTextEdit, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hBtnArduino, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hBtnCh340, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hBtnUpdate, WM_SETFONT, (WPARAM)hFontSemi, TRUE);
    SendMessageW(hLblCredit, WM_SETFONT, (WPARAM)hFontSmall, TRUE);
}

static inline void LayoutAllControls(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int W = rc.right, H = rc.bottom;
    if (W <= 0 || H <= 0) return;

    auto Place = [](HWND h, int x, int y, int w, int ht) {
        if (h) SetWindowPos(h, NULL, x, y, w, ht, SWP_NOZORDER | SWP_NOACTIVATE);
    };
    auto PlaceSpin = [&](HWND ed, HWND sp, int x, int y, int w, int ht) {
        int spinW = S(18);
        int edW = std::max(S(24), w - spinW);
        Place(ed, x, y, edW, ht);
        Place(sp, x + edW, y, spinW, ht);
    };

    int topH = S(52);
    Place(hToggleLeds, W - S(162), S(14), S(76), S(24));

    int lx = S(14), lw = S(330);
    int cPad = S(14);
    int innerX = lx + cPad;
    int innerW = lw - 2 * cPad;
    int lblW = S(84);
    int ctrlX = innerX + lblW;
    int ctrlW = innerW - lblW;
    int rowH = S(24);

    // Card 1: Bağlantı
    int c1y = topH + S(10), c1h = S(148);
    g_cardRects[0] = {lx, c1y, lx + lw, c1y + c1h};
    Place(hLblPort, innerX, c1y + S(38), lblW, rowH);
    Place(hPortCB, ctrlX, c1y + S(35), ctrlW - S(62), rowH);
    Place(hBtnRefresh, innerX + innerW - S(56), c1y + S(35), S(56), rowH);
    Place(hLblBaud, innerX, c1y + S(70), lblW, rowH);
    Place(hBaudCB, ctrlX, c1y + S(67), ctrlW, rowH);
    Place(hConnectBtn, innerX, c1y + S(103), innerW, S(30));

    // Card 2: Genel
    int c2y = c1y + c1h + S(10), c2h = S(140);
    g_cardRects[1] = {lx, c2y, lx + lw, c2y + c2h};
    Place(hLblBright, innerX, c2y + S(38), lblW, rowH);
    Place(hBrightSlider, ctrlX, c2y + S(35), ctrlW - S(46), rowH);
    Place(hBrightVal, innerX + innerW - S(42), c2y + S(38), S(42), rowH);
    Place(hLblFps, innerX, c2y + S(70), lblW, rowH);
    Place(hFpsCB, ctrlX, c2y + S(67), ctrlW, rowH);
    Place(hLblGamma, innerX, c2y + S(102), lblW, rowH);
    Place(hGammaCB, ctrlX, c2y + S(99), ctrlW, rowH);

    // Card 3: Efekt
    int c3y = c2y + c2h + S(10), c3h = S(172);
    g_cardRects[2] = {lx, c3y, lx + lw, c3y + c3h};
    Place(hLblEffect, innerX, c3y + S(38), lblW, rowH);
    Place(hEffectCB, ctrlX, c3y + S(35), ctrlW, rowH);
    Place(hLblSpeed, innerX, c3y + S(70), lblW, rowH);
    Place(hSpeedSlider, ctrlX, c3y + S(67), ctrlW - S(46), rowH);
    Place(hSpeedVal, innerX + innerW - S(42), c3y + S(70), S(42), rowH);
    Place(hLblColor1, innerX, c3y + S(104), lblW, rowH);
    Place(hColor1Btn, ctrlX, c3y + S(101), S(48), S(24));
    Place(hLblColor2, innerX, c3y + S(136), lblW, rowH);
    Place(hColor2Btn, ctrlX, c3y + S(133), S(48), S(24));
    Place(hBtnColorWiz, ctrlX + S(58), c3y + S(116), ctrlW - S(58), S(30));

    // Right area
    int rx = lx + lw + S(14);
    int rw = std::max(S(200), W - rx - S(14));
    int tabY = topH + S(10), tabH = S(34);
    int tabW = S(140);
    for (int i = 0; i < 3; ++i) {
        g_tabRects[i] = {rx + i * (tabW + S(8)), tabY, rx + i * (tabW + S(8)) + tabW, tabY + tabH};
    }

    int py = tabY + tabH + S(10);
    int ph = std::max(S(200), H - py - S(14));

    // Page 0: Preview
    Place(hPreviewBig, rx, py, rw, ph);

    // Page 1: Ambilight Layout
    int formW = std::min(S(350), std::max(S(270), rw / 2));
    g_cardRects[3] = {rx, py, rx + formW, py + ph};
    int fx = rx + cPad, fw = formW - 2 * cPad;
    int halfW = (fw - S(10)) / 2;
    int fy = py + S(36);

    // 2-column grid: Row 1 (Sol, Üst), Row 2 (Sağ, Alt-Sol), Row 3 (Alt-Sağ, Toplam)
    int spLblW = S(48);
    int spBoxW = halfW - spLblW;
    Place(hAmbiLbl[0], fx, fy + S(3), spLblW, rowH);
    PlaceSpin(hAmbiEdit[0], hAmbiSpin[0], fx + spLblW, fy, spBoxW, rowH);
    Place(hAmbiLbl[1], fx + halfW + S(10), fy + S(3), spLblW, rowH);
    PlaceSpin(hAmbiEdit[1], hAmbiSpin[1], fx + halfW + S(10) + spLblW, fy, spBoxW, rowH);
    fy += S(30);

    Place(hAmbiLbl[2], fx, fy + S(3), spLblW, rowH);
    PlaceSpin(hAmbiEdit[2], hAmbiSpin[2], fx + spLblW, fy, spBoxW, rowH);
    Place(hAmbiLbl[3], fx + halfW + S(10), fy + S(3), spLblW, rowH);
    PlaceSpin(hAmbiEdit[3], hAmbiSpin[3], fx + halfW + S(10) + spLblW, fy, spBoxW, rowH);
    fy += S(30);

    Place(hAmbiLbl[4], fx, fy + S(3), spLblW, rowH);
    PlaceSpin(hAmbiEdit[4], hAmbiSpin[4], fx + spLblW, fy, spBoxW, rowH);
    Place(hLblTotalLeds, fx + halfW + S(10), fy + S(3), halfW, rowH);
    fy += S(32);

    int fLblW = S(80);
    int fCtrlX = fx + fLblW;
    int fCtrlW = fw - fLblW;

    Place(hLblGap, fx, fy + S(3), fLblW, rowH);
    Place(hGapSlider, fCtrlX, fy, fCtrlW - S(40), rowH);
    Place(hGapVal, fx + fw - S(36), fy + S(3), S(36), rowH);
    fy += S(30);

    Place(hLblCorner, fx, fy + S(3), fLblW, rowH);
    Place(hCornerCB, fCtrlX, fy, fCtrlW, rowH);
    fy += S(32);

    Place(hLblDir, fx, fy + S(3), fLblW, rowH);
    int dirBtnW = (fCtrlW - S(6)) / 2;
    Place(hBtnDirCW,  fCtrlX, fy, dirBtnW, S(26));
    Place(hBtnDirCCW, fCtrlX + dirBtnW + S(6), fy, fCtrlW - dirBtnW - S(6), S(26));
    fy += S(34);

    Place(hLblOffset, fx, fy + S(3), fLblW, rowH);
    PlaceSpin(hOffsetEdit, hOffsetSpin, fCtrlX, fy, fCtrlW, rowH);
    fy += S(32);

    Place(hLblDepth, fx, fy + S(3), fLblW, rowH);
    Place(hDepthSlider, fCtrlX, fy, fCtrlW - S(40), rowH);
    Place(hDepthVal, fx + fw - S(36), fy + S(3), S(36), rowH);
    fy += S(30);

    Place(hLblSat, fx, fy + S(3), fLblW, rowH);
    Place(hSatSlider, fCtrlX, fy, fCtrlW - S(40), rowH);
    Place(hSatVal, fx + fw - S(36), fy + S(3), S(36), rowH);
    fy += S(30);

    Place(hLblSmooth, fx, fy + S(3), fLblW, rowH);
    Place(hSmoothSlider, fCtrlX, fy, fCtrlW - S(40), rowH);
    Place(hSmoothVal, fx + fw - S(36), fy + S(3), S(36), rowH);
    fy += S(32);

    Place(hLblMon, fx, fy + S(3), fLblW, rowH);
    Place(hMonCB, fCtrlX, fy, fCtrlW, rowH);
    fy += S(34);

    Place(hToggleCalib, fx, fy, fw, S(26));

    int smallX = rx + formW + S(12);
    int smallW = std::max(S(120), rw - formW - S(12));
    Place(hPreviewSmall, smallX, py, smallW, ph);

    // Page 2: Sistem
    int s1h = S(206);
    g_cardRects[4] = {rx, py, rx + rw, py + s1h};
    int sx = rx + cPad, sw = rw - 2 * cPad;
    int sy = py + S(38);
    Place(hToggleAutostart,   sx, sy, sw, S(26)); sy += S(30);
    Place(hToggleStartHidden, sx, sy, sw, S(26)); sy += S(30);
    Place(hToggleCloseTray,   sx, sy, sw, S(26)); sy += S(34);
    Place(hLblCapture, sx, sy + S(3), S(140), rowH);
    Place(hCaptureCB,  sx + S(146), sy, std::min(S(240), sw - S(146)), rowH);
    sy += S(32);
    Place(hLblLang,    sx, sy + S(3), S(140), rowH);
    Place(hLangCB,     sx + S(146), sy, std::min(S(240), sw - S(146)), rowH);

    int infoY = py + s1h + S(18);
    for (int i = 0; i < 5; ++i) {
        Place(hInfoTitle[i], sx, infoY + i * S(24), S(130), rowH);
        Place(hInfoVal[i],   sx + S(136), infoY + i * S(24), sw - S(136), rowH);
    }

    int btnY = infoY + 5 * S(24) + S(16);
    int bw = std::min(S(180), (sw - S(16)) / 3);
    Place(hBtnSave,     sx,                 btnY, bw, S(34));
    Place(hBtnTextEdit, sx + bw + S(8),     btnY, bw, S(34));
    Place(hBtnArduino,  sx + (bw + S(8))*2, btnY, bw, S(34));
    int btnY2 = btnY + S(42);
    Place(hBtnCh340,    sx,                 btnY2, bw, S(34));
    Place(hBtnUpdate,   sx + bw + S(8),     btnY2, bw, S(34));
    Place(hLblCredit,   sx,                 py + ph - S(22), sw, S(20));
}

// ─────────────────────────── MAIN WINDOW PAINT ──────────────────────────────
static inline void PaintMainWindow(HWND hwnd, HDC hdc, int W, int H) {
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));

    // Background
    SetDCBrushColor(hdc, CLR_BG);
    SetDCPenColor(hdc, CLR_BG);
    Rectangle(hdc, 0, 0, W, H);

    // Topbar divider
    int topH = S(52);
    SetDCBrushColor(hdc, CLR_BORDER);
    SetDCPenColor(hdc, CLR_BORDER);
    Rectangle(hdc, 0, topH - 1, W, topH);

    // Logo icon + Title
    if (g_hAppIcon) {
        DrawIconEx(hdc, S(16), (topH - S(28)) / 2, g_hAppIcon, S(28), S(28), 0, NULL, DI_NORMAL);
    }
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hFontTitle);
    SetTextColor(hdc, CLR_TEXT);
    RECT titleRc = {S(52), 0, S(180), topH};
    DrawTextW(hdc, L"ETDAmbilight", -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Connection dot & text
    bool conn = g_connected.load();
    COLORREF dotCol = conn ? CLR_SUCCESS : CLR_ERROR;
    int dx = S(186), dy = topH / 2, dr = S(5);
    SetDCBrushColor(hdc, dotCol);
    SetDCPenColor(hdc, dotCol);
    Ellipse(hdc, dx - dr, dy - dr, dx + dr, dy + dr);

    wchar_t stBuf[160] = {};
    {
        std::lock_guard<std::mutex> lk(g_settings_mtx);
        if (conn) {
            swprintf_s(stBuf, L"%s @ %lu baud", g_settings.port, g_settings.baud);
        } else {
            wcscpy_s(stBuf, LStr(L"Bağlı değil", L"Disconnected"));
            if (!g_serial.lastError.empty()) {
                wcscat_s(stBuf, L" — ");
                wcsncat_s(stBuf, g_serial.lastError.c_str(), 48);
            }
        }
    }
    SelectObject(hdc, hFontUI);
    SetTextColor(hdc, CLR_TEXT2);
    RECT stRc = {S(200), 0, W - S(200), topH};
    DrawTextW(hdc, stBuf, -1, &stRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // FPS counter at top right
    SelectObject(hdc, hFontMono);
    SetTextColor(hdc, CLR_SUCCESS);
    wchar_t fpsBuf[32];
    swprintf_s(fpsBuf, L"%.0f FPS", g_fps_actual.load());
    RECT fpsRc = {W - S(82), 0, W - S(16), topH};
    DrawTextW(hdc, fpsBuf, -1, &fpsRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // Left cards
    auto DrawCard = [&](const RECT& r, const wchar_t* title) {
        SetDCBrushColor(hdc, CLR_CARD);
        SetDCPenColor(hdc, CLR_BORDER);
        RoundRect(hdc, r.left, r.top, r.right, r.bottom, S(12), S(12));
        SelectObject(hdc, hFontHead);
        SetTextColor(hdc, CLR_TEXT);
        RECT tr = {r.left + S(14), r.top + S(10), r.right - S(14), r.top + S(32)};
        DrawTextW(hdc, title, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    };

    DrawCard(g_cardRects[0], LStr(L"Bağlantı", L"Connection"));
    DrawCard(g_cardRects[1], LStr(L"Genel", L"General"));
    DrawCard(g_cardRects[2], LStr(L"Efekt", L"Effect"));

    if (g_activeTab == 1) {
        DrawCard(g_cardRects[3], LStr(L"LED Yerleşimi & Yön", L"LED Layout & Direction"));
    } else if (g_activeTab == 2) {
        DrawCard(g_cardRects[4], LStr(L"Sistem & Başlangıç", L"System & Startup"));
    }

    // Segmented pill tabs
    const wchar_t* tabLabels[3] = {
        LStr(L"Önizleme", L"Preview"),
        LStr(L"Ambilight Düzeni", L"Ambilight Layout"),
        LStr(L"Sistem", L"System")
    };
    SelectObject(hdc, hFontSemi);
    for (int i = 0; i < 3; ++i) {
        const RECT& tr = g_tabRects[i];
        bool active = (i == g_activeTab);
        SetDCBrushColor(hdc, active ? CLR_ACCENT : CLR_CARD);
        SetDCPenColor(hdc, active ? CLR_ACCENT : CLR_BORDER);
        RoundRect(hdc, tr.left, tr.top, tr.right, tr.bottom, S(8), S(8));
        SetTextColor(hdc, active ? RGB(255, 255, 255) : CLR_TEXT2);
        RECT trCopy = tr;
        DrawTextW(hdc, tabLabels[i], -1, &trCopy, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

// ─────────────────────────── MAIN WNDPROC ────────────────────────────────────
static PreviewBuf g_mainBackBuf;

static inline LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (WM_APP_SHOW && msg == WM_APP_SHOW) {
        ShowFromTray(hwnd);
        return 0;
    }
    if (WM_TASKBAR_CREATED && msg == WM_TASKBAR_CREATED) {
        AddOrUpdateTrayIcon(hwnd);
        return 0;
    }

    switch (msg) {
    case WM_CREATE: {
        UpdateDpiForWindow(hwnd);
        RebuildThemeResources();
        ApplyWindowChrome(hwnd);
        CreateAllControls(hwnd);
        LayoutAllControls(hwnd);
        AddOrUpdateTrayIcon(hwnd);
        return 0;
    }

    case WM_DPICHANGED: {
        g_dpi = HIWORD(wp);
        RebuildThemeResources();
        UpdateFontsOnAllChildren(hwnd);
        RECT* sug = (RECT*)lp;
        if (sug) {
            SetWindowPos(hwnd, NULL, sug->left, sug->top,
                         sug->right - sug->left, sug->bottom - sug->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        LayoutAllControls(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_SIZE: {
        int W = LOWORD(lp), H = HIWORD(lp);
        if (wp == SIZE_MINIMIZED) {
            KillTimer(hwnd, 1);
            g_uiVisible.store(false, std::memory_order_relaxed);
        } else {
            if (IsWindowVisible(hwnd)) {
                g_uiVisible.store(true, std::memory_order_relaxed);
                SetTimer(hwnd, 1, 33, NULL);
            }
            LayoutAllControls(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        (void)W; (void)H;
        return 0;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lp;
        mmi->ptMinTrackSize = {S(980), S(640)};
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
            g_mainBackBuf.Ensure(hdc, rc.right, rc.bottom);
            if (g_mainBackBuf.memDC) {
                PaintMainWindow(hwnd, g_mainBackBuf.memDC, rc.right, rc.bottom);
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, g_mainBackBuf.memDC, 0, 0, SRCCOPY);
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
            if (id == ID_SLIDER_BRIGHT || id == ID_SLIDER_SPEED || id == ID_SLIDER_GAP ||
                id == ID_SLIDER_DEPTH  || id == ID_SLIDER_SAT   || id == ID_SLIDER_SMOOTH) {
                return HandleTrackbarCustomDraw((LPNMCUSTOMDRAW)lp);
            }
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
        POINT pt = {mx, my};
        for (int i = 0; i < 3; ++i) {
            if (PtInRect(&g_tabRects[i], pt)) {
                SwitchTab(hwnd, i);
                return 0;
            }
        }
        return 0;
    }

    case WM_HSCROLL: {
        if (g_suppressSync) return 0;
        HWND hSlider = (HWND)lp;
        int val = (int)SendMessageW(hSlider, TBM_GETPOS, 0, 0);
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            if (hSlider == hBrightSlider) {
                g_settings.brightness = val;
            } else if (hSlider == hSpeedSlider) {
                g_settings.effect_speed = val;
            } else if (hSlider == hGapSlider) {
                g_settings.bottom_gap_pct = val;
                MarkLayoutChanged();
            } else if (hSlider == hDepthSlider) {
                g_settings.edge_depth_pct = val;
                MarkLayoutChanged();
            } else if (hSlider == hSatSlider) {
                g_settings.saturation = val / 10.0f;
            } else if (hSlider == hSmoothSlider) {
                g_settings.smoothing = val / 100.0f;
            }
            UpdateLabelsFromSettings(g_settings);
        }
        MarkDirty();
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        int notif = HIWORD(wp);

        // Tray menu actions
        if (id == ID_TRAY_SHOW) {
            ShowFromTray(hwnd);
            return 0;
        }
        if (id == ID_TRAY_EXIT) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (id == ID_TRAY_TOGGLE_LEDS) {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            g_settings.leds_enabled = !g_settings.leds_enabled;
            SetToggleState(hToggleLeds, g_settings.leds_enabled);
            MarkDirty();
            WakeCapturer();
            return 0;
        }
        if (id >= ID_TRAY_EFFECT_BASE && id < ID_TRAY_EFFECT_BASE + EFFECT_COUNT) {
            int eff = id - ID_TRAY_EFFECT_BASE;
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                if (eff != 11) g_prevNonCalibEffect = eff;
                g_settings.effect = eff;
                SendMessageW(hEffectCB, CB_SETCURSEL, eff, 0);
                SetToggleState(hToggleCalib, eff == 11);
                MarkDirty();
                WakeCapturer();
            }
            return 0;
        }

        if (g_suppressSync) return 0;

        if (id == ID_TOGGLE_LEDS) {
            bool on = !GetToggleState(hToggleLeds);
            SetToggleState(hToggleLeds, on);
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.leds_enabled = on;
            }
            MarkDirty();
            WakeCapturer();
        } else if (id == ID_BTN_REFRESH) {
            PopulatePortCombo();
        } else if (id == ID_BTN_CONNECT) {
            if (g_connected.load()) {
                g_serial.SendBlack(g_settings.led_count);
                g_serial.Disconnect();
                g_connected.store(false);
            } else {
                std::wstring port;
                DWORD baud = BAUD_DEFAULT;
                {
                    std::lock_guard<std::mutex> lk(g_settings_mtx);
                    int portSel = (int)SendMessageW(hPortCB, CB_GETCURSEL, 0, 0);
                    if (portSel == 0) wcscpy_s(g_settings.port, L"AUTO");
                    else if (portSel > 0 && portSel - 1 < (int)g_ports.size())
                        wcsncpy_s(g_settings.port, g_ports[portSel - 1].name.c_str(), _TRUNCATE);

                    int baudSel = (int)SendMessageW(hBaudCB, CB_GETCURSEL, 0, 0);
                    DWORD bauds[] = {9600, 57600, 115200, 500000, 1000000, 2000000};
                    if (baudSel >= 0 && baudSel < 6) g_settings.baud = bauds[baudSel];

                    port = g_settings.port;
                    baud = g_settings.baud;
                }
                if (port == L"AUTO") {
                    auto ports = ScanPorts();
                    port = AutoDetectArduino(ports);
                }
                if (!port.empty() && g_serial.Connect(port, baud)) {
                    g_connected.store(true);
                } else {
                    wchar_t emsg[256];
                    swprintf_s(emsg,
                               LStr(L"Port açılamadı:\n%s\n\nCH340 sürücüsü yüklü mü?",
                                    L"Cannot open port:\n%s\n\nIs the CH340 driver installed?"),
                               g_serial.lastError.c_str());
                    MessageBoxW(hwnd, emsg, LStr(L"Bağlantı Hatası", L"Connection Error"), MB_OK | MB_ICONERROR);
                }
                MarkDirty();
            }
            UpdateConnectButton();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == ID_COMBO_FPS && notif == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(hFpsCB, CB_GETCURSEL, 0, 0);
            int fpsOpts[] = {30, 60, 90, 120, 144, 180};
            if (sel >= 0 && sel < 6) {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.target_fps = fpsOpts[sel];
                MarkDirty();
            }
        } else if (id == ID_COMBO_GAMMA && notif == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(hGammaCB, CB_GETCURSEL, 0, 0);
            float gammas[] = {1.0f, 1.8f, 2.0f, 2.2f, 2.4f};
            if (sel >= 0 && sel < 5) {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.gamma = gammas[sel];
                MarkDirty();
            }
        } else if (id == ID_COMBO_EFFECT && notif == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(hEffectCB, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < EFFECT_COUNT) {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                if (sel != 11) g_prevNonCalibEffect = sel;
                g_settings.effect = sel;
                SetToggleState(hToggleCalib, sel == 11);
                MarkDirty();
                WakeCapturer();
            }
        } else if (id == ID_BTN_COLOR1 || id == ID_BTN_COLOR2) {
            static COLORREF customs[16] = {};
            CHOOSECOLORW cc = {sizeof(cc)};
            cc.hwndOwner = hwnd;
            cc.lpCustColors = customs;
            cc.rgbResult = (id == ID_BTN_COLOR1) ? g_settings.color1 : g_settings.color2;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                if (id == ID_BTN_COLOR1) {
                    g_settings.color1 = cc.rgbResult;
                    SetBtnColorSwatch(hColor1Btn, cc.rgbResult);
                    InvalidateRect(hColor1Btn, NULL, FALSE);
                } else {
                    g_settings.color2 = cc.rgbResult;
                    SetBtnColorSwatch(hColor2Btn, cc.rgbResult);
                    InvalidateRect(hColor2Btn, NULL, FALSE);
                }
                MarkDirty();
            }
        } else if ((id == ID_EDIT_AMBI_L || id == ID_EDIT_AMBI_T ||
                    id == ID_EDIT_AMBI_R || id == ID_EDIT_AMBI_BL || id == ID_EDIT_AMBI_BR) && notif == EN_CHANGE) {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            g_settings.ambi_left         = std::clamp((int)GetDlgItemInt(hwnd, ID_EDIT_AMBI_L,  NULL, FALSE), 0, 300);
            g_settings.ambi_top          = std::clamp((int)GetDlgItemInt(hwnd, ID_EDIT_AMBI_T,  NULL, FALSE), 0, 300);
            g_settings.ambi_right        = std::clamp((int)GetDlgItemInt(hwnd, ID_EDIT_AMBI_R,  NULL, FALSE), 0, 300);
            g_settings.ambi_bottom_left  = std::clamp((int)GetDlgItemInt(hwnd, ID_EDIT_AMBI_BL, NULL, FALSE), 0, 300);
            g_settings.ambi_bottom_right = std::clamp((int)GetDlgItemInt(hwnd, ID_EDIT_AMBI_BR, NULL, FALSE), 0, 300);
            int total = g_settings.TotalEdgeLeds();
            if (total > 0) g_settings.led_count = std::min(total, MAX_LEDS);
            UpdateLabelsFromSettings(g_settings);
            MarkLayoutChanged();
        } else if (id == ID_EDIT_OFFSET && notif == EN_CHANGE) {
            BOOL ok = FALSE;
            int val = (int)GetDlgItemInt(hwnd, ID_EDIT_OFFSET, &ok, TRUE);
            if (ok) {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.led_offset = std::clamp(val, -500, 500);
                MarkLayoutChanged();
            }
        } else if (id == ID_COMBO_CORNER && notif == CBN_SELCHANGE) {
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.start_corner = std::clamp((int)SendMessageW(hCornerCB, CB_GETCURSEL, 0, 0), 0, 4);
                g_settings.led_offset = 0;
                MarkLayoutChanged();
            }
            g_suppressSync = true;
            SendMessageW(hOffsetSpin, UDM_SETPOS32, 0, 0);
            g_suppressSync = false;
        } else if (id == ID_BTN_DIR_CW || id == ID_BTN_DIR_CCW) {
            int newDir = (id == ID_BTN_DIR_CW) ? 0 : 1;
            int newOff = 0;
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                int oldDir = g_settings.direction;
                if (newDir != oldDir) {
                    if (!(g_settings.start_corner == 4 && g_settings.led_offset == 0)) {
                        int L  = g_settings.ambi_left, T = g_settings.ambi_top, R = g_settings.ambi_right;
                        int BL = g_settings.ambi_bottom_left, BR = g_settings.ambi_bottom_right;
                        if (L + T + R + BL + BR <= 0) R = std::max(1, g_settings.led_count);
                        newOff = ComputeOffsetToKeepFirstLed(L, T, R, BL, BR, g_settings.start_corner, oldDir, g_settings.led_offset, newDir);
                        g_settings.led_offset = newOff;
                    }
                    g_settings.direction = newDir;
                    MarkLayoutChanged();
                } else {
                    newOff = g_settings.led_offset;
                }
            }
            g_suppressSync = true;
            SendMessageW(hOffsetSpin, UDM_SETPOS32, 0, newOff);
            UpdateDirectionButtons(newDir);
            g_suppressSync = false;
            if (hPreviewBig)   InvalidateRect(hPreviewBig, NULL, FALSE);
            if (hPreviewSmall) InvalidateRect(hPreviewSmall, NULL, FALSE);
        } else if (id == ID_COMBO_MON && notif == CBN_SELCHANGE) {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            g_settings.monitor_idx = std::max(0, (int)SendMessageW(hMonCB, CB_GETCURSEL, 0, 0));
            MarkLayoutChanged();
        } else if (id == ID_TOGGLE_CALIB) {
            bool on = !GetToggleState(hToggleCalib);
            SetToggleState(hToggleCalib, on);
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                if (on) {
                    if (g_settings.effect != 11) g_prevNonCalibEffect = g_settings.effect;
                    g_settings.effect = 11;
                } else {
                    g_settings.effect = (g_prevNonCalibEffect != 11) ? g_prevNonCalibEffect : 0;
                }
                SendMessageW(hEffectCB, CB_SETCURSEL, g_settings.effect, 0);
                MarkDirty();
                WakeCapturer();
            }
        } else if (id == ID_TOGGLE_AUTOSTART) {
            bool on = !GetToggleState(hToggleAutostart);
            SetToggleState(hToggleAutostart, on);
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.autostart = on;
            }
            SetAutostart(on);
            MarkDirty();
        } else if (id == ID_TOGGLE_STARTHIDDEN) {
            bool on = !GetToggleState(hToggleStartHidden);
            SetToggleState(hToggleStartHidden, on);
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.start_hidden = on;
            }
            MarkDirty();
        } else if (id == ID_TOGGLE_CLOSETRAY) {
            bool on = !GetToggleState(hToggleCloseTray);
            SetToggleState(hToggleCloseTray, on);
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.close_to_tray = on;
            }
            MarkDirty();
        } else if (id == ID_COMBO_CAPTURE && notif == CBN_SELCHANGE) {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            g_settings.capture_method = std::clamp((int)SendMessageW(hCaptureCB, CB_GETCURSEL, 0, 0), 0, 1);
            MarkLayoutChanged();
        } else if (id == ID_COMBO_LANG && notif == CBN_SELCHANGE) {
            int newLang = std::clamp((int)SendMessageW(hLangCB, CB_GETCURSEL, 0, 0), 0, 1);
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                g_settings.lang = newLang;
            }
            g_lang.store(newLang, std::memory_order_relaxed);
            MarkDirty();
            ApplyLanguageToUI(hwnd);
        } else if (id == ID_BTN_SAVE) {
            Settings sCopy;
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                sCopy = g_settings;
            }
            SaveSettings(sCopy, true);
            MessageBoxW(hwnd,
                        LStr(L"Ayarlar hem Registry'ye hem doğrudan EXE dosyasının içine kaydedildi.",
                             L"Settings saved to both Windows Registry and directly inside the EXE."),
                        APP_NAME, MB_OK | MB_ICONINFORMATION);
        } else if (id == ID_BTN_TEXTEDIT) {
            OpenModalDialog(hwnd, DLG_TEXT_EDITOR);
        } else if (id == ID_BTN_ARDUINO) {
            OpenModalDialog(hwnd, DLG_ARDUINO_CODE);
        } else if (id == ID_BTN_CH340) {
            ShowCH340Helper(hwnd);
        } else if (id == ID_BTN_UPDATE) {
            if (!g_updateCheckBusy) {
                g_updateCheckBusy = true;
                SetWindowTextW(hBtnUpdate, LStr(L"Denetleniyor...", L"Checking..."));
                InvalidateRect(hBtnUpdate, NULL, FALSE);
                StartUpdateCheck(hwnd);
            }
        } else if (id == ID_BTN_COLORWIZ) {
            OpenColorWizard(hwnd);
        }
        return 0;
    }

    case WM_UPDATE_CHECK_DONE: {
        g_updateCheckBusy = false;
        SetWindowTextW(hBtnUpdate, LStr(L"Güncelleme Denetle", L"Check Updates"));
        InvalidateRect(hBtnUpdate, NULL, FALSE);
        auto* res = (UpdateCheckResult*)lp;
        if (res) {
            if (!res->ok) {
                MessageBoxW(hwnd,
                            LStr(L"Güncelleme bilgisi alınamadı. İnternet bağlantınızı kontrol edin veya GitHub Releases sayfasını ziyaret edin.",
                                 L"Could not check for updates. Please check your internet connection or visit GitHub Releases."),
                            APP_NAME, MB_OK | MB_ICONWARNING);
            } else if (!res->hasUpdate) {
                wchar_t msg[256];
                swprintf_s(msg,
                           LStr(L"ETDAmbilight güncel (v%s). En son sürümü kullanıyorsunuz.",
                                L"ETDAmbilight is up to date (v%s). You are running the latest version."),
                           APP_VER);
                MessageBoxW(hwnd, msg, APP_NAME, MB_OK | MB_ICONINFORMATION);
            } else {
                wchar_t msg[512];
                swprintf_s(msg,
                           LStr(L"Yeni sürüm bulundu: %s (mevcut: v%s)\n\n"
                                L"Kurulum sihirbazı indirilip çalıştırılsın mı? Tüm ayarlarınız silinmeden korunacaktır.",
                                L"New version available: %s (current: v%s)\n\n"
                                L"Download and launch the setup wizard? All your settings will be preserved."),
                           res->latestTag.c_str(), APP_VER);
                if (MessageBoxW(hwnd, msg, APP_NAME, MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    if (!res->setupUrl.empty() && DownloadAndLaunchSetup(res->setupUrl)) {
                        DestroyWindow(hwnd);
                    } else {
                        ShellExecuteW(NULL, L"open", res->releaseUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }
                }
            }
            delete res;
        }
        return 0;
    }

    case WM_TIMER: {
        if (wp == 1 && g_uiVisible.load(std::memory_order_relaxed)) {
            if (g_activeTab == 0 && hPreviewBig)   InvalidateRect(hPreviewBig, NULL, FALSE);
            if (g_activeTab == 1 && hPreviewSmall) InvalidateRect(hPreviewSmall, NULL, FALSE);

            ++g_uiTickCounter;
            if ((g_uiTickCounter % 8) == 0) {
                RECT topRc = {S(180), 0, 10000, S(52)};
                InvalidateRect(hwnd, &topRc, FALSE);

                if (g_activeTab == 2) {
                    Settings s;
                    {
                        std::lock_guard<std::mutex> lk(g_settings_mtx);
                        s = g_settings;
                    }
                    wchar_t buf[64];
                    swprintf_s(buf, L"%.1f", g_fps_actual.load());
                    SetWindowTextW(hInfoVal[0], buf);
                    swprintf_s(buf, L"%s @ %lu", s.port, s.baud);
                    SetWindowTextW(hInfoVal[1], buf);
                    int capM = g_activeCaptureMode.load();
                    const wchar_t* capStr = (capM == 1) ? L"DXGI Desktop Duplication"
                                          : (capM == 2 ? L"GDI" : LStr(L"Beklemede", L"Idle"));
                    SetWindowTextW(hInfoVal[2], capStr);
                    swprintf_s(buf, L"%d", s.led_count);
                    SetWindowTextW(hInfoVal[3], buf);
                    SetWindowTextW(hInfoVal[4], EffectName(s.effect));
                }
            }
        }
        return 0;
    }

    case WM_STATUS_UPDATE:
        UpdateConnectButton();
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_TRAYICON:
        if (lp == WM_LBUTTONUP || lp == WM_LBUTTONDBLCLK) {
            ShowFromTray(hwnd);
        } else if (lp == WM_RBUTTONUP || lp == WM_CONTEXTMENU) {
            ShowTrayMenu(hwnd);
        }
        return 0;

    case WM_DEVICECHANGE:
        if (wp == DBT_DEVICEARRIVAL || wp == DBT_DEVNODES_CHANGED) {
            g_rescan.store(true, std::memory_order_relaxed);
            if (g_uiVisible.load(std::memory_order_relaxed)) {
                PopulatePortCombo();
            }
        }
        return TRUE;

    case WM_POWERBROADCAST:
        if (wp == PBT_APMSUSPEND) {
            g_serial.SendBlack(g_settings.led_count);
            g_serial.Disconnect();
            g_connected.store(false);
        } else if (wp == PBT_APMRESUMEAUTOMATIC) {
            g_rescan.store(true, std::memory_order_relaxed);
        }
        return TRUE;

    case WM_ENDSESSION:
        if (wp != 0) {
            g_serial.SendBlack(g_settings.led_count);
            g_serial.Disconnect();
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            SaveSettingsToRegistry(g_settings);
        }
        return 0;

    case WM_CLOSE: {
        bool closeToTray = true;
        {
            std::lock_guard<std::mutex> lk(g_settings_mtx);
            closeToTray = g_settings.close_to_tray;
        }
        if (closeToTray) {
            HideToTray(hwnd);
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        g_running.store(false);
        RemoveTrayIcon();
        g_mainBackBuf.Release();
        DestroyThemeResources();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}
