#pragma once

#include "common.h"

// ─────────────────────────── PALETTE ────────────────────────────────────────
static const COLORREF CLR_BG       = RGB(0x1B, 0x1C, 0x20);
static const COLORREF CLR_CARD     = RGB(0x24, 0x26, 0x2B);
static const COLORREF CLR_BORDER   = RGB(0x2F, 0x32, 0x38);
static const COLORREF CLR_CTRL_BG  = RGB(0x2D, 0x30, 0x37);
static const COLORREF CLR_TEXT     = RGB(0xE6, 0xE6, 0xE6);
static const COLORREF CLR_TEXT2    = RGB(0x9A, 0xA0, 0xA6);
static const COLORREF CLR_ACCENT   = RGB(0x4C, 0x8D, 0xFF);
static const COLORREF CLR_SUCCESS  = RGB(0x3D, 0xDC, 0x97);
static const COLORREF CLR_ERROR    = RGB(0xFF, 0x5C, 0x5C);
static const COLORREF CLR_WARN     = RGB(0xFF, 0xC1, 0x45);

// ─────────────────────────── DPI SCALING ────────────────────────────────────
static int g_dpi = 96;

static inline int S(int x) {
    return MulDiv(x, g_dpi, 96);
}

static inline void UpdateDpiForWindow(HWND hwnd) {
    typedef UINT(WINAPI* GetDpiFn)(HWND);
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser && hwnd) {
        auto fn = (GetDpiFn)GetProcAddress(hUser, "GetDpiForWindow");
        if (fn) {
            UINT d = fn(hwnd);
            if (d >= 72 && d <= 480) {
                g_dpi = (int)d;
                return;
            }
        }
    }
    HDC hdc = GetDC(hwnd);
    if (hdc) {
        g_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(hwnd, hdc);
    }
    if (g_dpi < 72) g_dpi = 96;
}

// ─────────────────────────── RESOURCES (FONTS & BRUSHES) ────────────────────
static HBRUSH hBrBg   = NULL;
static HBRUSH hBrCard = NULL;
static HBRUSH hBrCtrl = NULL;

static HFONT hFontUI    = NULL;
static HFONT hFontSemi  = NULL;
static HFONT hFontHead  = NULL;
static HFONT hFontTitle = NULL;
static HFONT hFontMono  = NULL;
static HFONT hFontSmall = NULL;

static inline bool FontExists(const wchar_t* face) {
    struct Ctx { bool found = false; } ctx;
    HDC hdc = GetDC(NULL);
    if (!hdc) return false;
    LOGFONTW lf = {};
    lf.lfCharSet = DEFAULT_CHARSET;
    wcsncpy_s(lf.lfFaceName, face, _TRUNCATE);
    EnumFontFamiliesExW(hdc, &lf, [](const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM lp) -> int {
        ((Ctx*)lp)->found = true;
        return 0;
    }, (LPARAM)&ctx, 0);
    ReleaseDC(NULL, hdc);
    return ctx.found;
}

static inline void DestroyThemeResources() {
    if (hBrBg)   { DeleteObject(hBrBg);   hBrBg = NULL; }
    if (hBrCard) { DeleteObject(hBrCard); hBrCard = NULL; }
    if (hBrCtrl) { DeleteObject(hBrCtrl); hBrCtrl = NULL; }
    if (hFontUI)    { DeleteObject(hFontUI);    hFontUI = NULL; }
    if (hFontSemi)  { DeleteObject(hFontSemi);  hFontSemi = NULL; }
    if (hFontHead)  { DeleteObject(hFontHead);  hFontHead = NULL; }
    if (hFontTitle) { DeleteObject(hFontTitle); hFontTitle = NULL; }
    if (hFontMono)  { DeleteObject(hFontMono);  hFontMono = NULL; }
    if (hFontSmall) { DeleteObject(hFontSmall); hFontSmall = NULL; }
}

static inline void RebuildThemeResources() {
    DestroyThemeResources();

    hBrBg   = CreateSolidBrush(CLR_BG);
    hBrCard = CreateSolidBrush(CLR_CARD);
    hBrCtrl = CreateSolidBrush(CLR_CTRL_BG);

    const wchar_t* uiFace = FontExists(L"Segoe UI Variable Text")
                            ? L"Segoe UI Variable Text" : L"Segoe UI";

    int h9  = -MulDiv(9, g_dpi, 72);
    int h11 = -MulDiv(11, g_dpi, 72);
    int h13 = -MulDiv(13, g_dpi, 72);
    int h8  = -MulDiv(7, g_dpi, 72);

    hFontUI = CreateFontW(h9, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, uiFace);
    hFontSemi = CreateFontW(h9, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, uiFace);
    hFontHead = CreateFontW(h11, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, uiFace);
    hFontTitle = CreateFontW(h13, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, uiFace);
    hFontMono = CreateFontW(h9, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    hFontSmall = CreateFontW(h8, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, uiFace);
}

// ─────────────────────────── DWM & DARK CONTROLS ────────────────────────────
static inline void ApplyWindowChrome(HWND hwnd) {
    BOOL dark = TRUE;
    if (FAILED(DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark)))) {
        DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
    }
    DWORD corner = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));
    DWORD mica = 2;   // DWMSBT_MAINWINDOW
    DwmSetWindowAttribute(hwnd, 38, &mica, sizeof(mica));
}

static inline void ApplyDarkTheme(HWND h, bool explorerStyle = false) {
    if (!h) return;
    SetWindowTheme(h, explorerStyle ? L"DarkMode_Explorer" : L"DarkMode_CFD", NULL);
}

// ─────────────────────────── COLOR MATH ─────────────────────────────────────
static inline COLORREF AdjustColor(COLORREF c, int delta) {
    int r = std::clamp((int)GetRValue(c) + delta, 0, 255);
    int g = std::clamp((int)GetGValue(c) + delta, 0, 255);
    int b = std::clamp((int)GetBValue(c) + delta, 0, 255);
    return RGB(r, g, b);
}

// ─────────────────────────── BUTTON & TOGGLE PROPS ──────────────────────────
static inline void SetBtnBg(HWND h, COLORREF c) {
    SetPropW(h, L"bg", (HANDLE)(UINT_PTR)c);
}

static inline void SetBtnColorSwatch(HWND h, COLORREF c) {
    SetPropW(h, L"is_color", (HANDLE)1);
    SetPropW(h, L"bg", (HANDLE)(UINT_PTR)c);
}

static inline void SetToggleState(HWND h, bool on) {
    SetPropW(h, L"is_toggle", (HANDLE)1);
    SetPropW(h, L"on", (HANDLE)(UINT_PTR)(on ? 1 : 0));
    InvalidateRect(h, NULL, FALSE);
}

static inline bool GetToggleState(HWND h) {
    return GetPropW(h, L"on") != NULL;
}

static inline LRESULT CALLBACK HoverSubclassProc(
    HWND hw, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR)
{
    if (msg == WM_MOUSEMOVE) {
        if (!GetPropW(hw, L"hover")) {
            SetPropW(hw, L"hover", (HANDLE)1);
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hw, 0};
            TrackMouseEvent(&tme);
            InvalidateRect(hw, NULL, FALSE);
        }
    } else if (msg == WM_MOUSELEAVE) {
        RemovePropW(hw, L"hover");
        InvalidateRect(hw, NULL, FALSE);
    } else if (msg == WM_NCDESTROY) {
        RemovePropW(hw, L"hover");
        RemovePropW(hw, L"bg");
        RemovePropW(hw, L"is_color");
        RemovePropW(hw, L"is_toggle");
        RemovePropW(hw, L"on");
    }
    return DefSubclassProc(hw, msg, wp, lp);
}

static inline void AttachHoverSubclass(HWND h) {
    SetWindowSubclass(h, HoverSubclassProc, 10, 0);
}

// ─────────────────────────── OWNER-DRAW BUTTONS ─────────────────────────────
static inline void DrawModernButton(DRAWITEMSTRUCT* di) {
    if (!di || di->CtlType != ODT_BUTTON) return;
    HDC dc = di->hDC;
    RECT rc = di->rcItem;
    HWND hw = di->hwndItem;

    bool hover   = GetPropW(hw, L"hover") != NULL;
    bool pressed = (di->itemState & ODS_SELECTED) != 0;

    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(DC_PEN));

    // Clear cell behind rounded rect with parent card background
    COLORREF parentBg = (GetPropW(hw, L"in_topbar") != NULL) ? CLR_BG : CLR_CARD;
    SetDCBrushColor(dc, parentBg);
    SetDCPenColor(dc, parentBg);
    Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);

    if (GetPropW(hw, L"is_color") != NULL) {
        COLORREF col = (COLORREF)(UINT_PTR)GetPropW(hw, L"bg");
        SetDCBrushColor(dc, col);
        SetDCPenColor(dc, hover ? CLR_TEXT : CLR_BORDER);
        RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, S(6), S(6));
        return;
    }

    if (GetPropW(hw, L"is_toggle") != NULL) {
        bool on = GetPropW(hw, L"on") != NULL;
        wchar_t text[128] = {};
        GetWindowTextW(hw, text, 127);

        if (text[0]) {
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, CLR_TEXT);
            SelectObject(dc, hFontUI);
            RECT tr = rc;
            tr.right -= S(44);
            DrawTextW(dc, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        int pw = S(36), ph = S(20);
        int px = text[0] ? (rc.right - pw) : ((rc.left + rc.right - pw) / 2);
        int py = (rc.top + rc.bottom - ph) / 2;

        COLORREF trackCol = on ? CLR_ACCENT : CLR_CTRL_BG;
        if (hover) trackCol = AdjustColor(trackCol, 14);
        SetDCBrushColor(dc, trackCol);
        SetDCPenColor(dc, on ? trackCol : CLR_BORDER);
        RoundRect(dc, px, py, px + pw, py + ph, ph, ph);

        int knobD = ph - S(6);
        int kx = on ? (px + pw - knobD - S(3)) : (px + S(3));
        int ky = py + S(3);
        SetDCBrushColor(dc, RGB(245, 247, 250));
        SetDCPenColor(dc, RGB(245, 247, 250));
        Ellipse(dc, kx, ky, kx + knobD, ky + knobD);
        return;
    }

    COLORREF bg = (COLORREF)(UINT_PTR)GetPropW(hw, L"bg");
    if (bg == 0) bg = CLR_CTRL_BG;
    if (pressed)     bg = AdjustColor(bg, -18);
    else if (hover)  bg = AdjustColor(bg, 16);

    COLORREF borderCol = (bg == CLR_ACCENT || GetRValue(bg) > 160) ? bg : CLR_BORDER;
    if (hover && borderCol == CLR_BORDER) borderCol = RGB(82, 88, 98);

    SetDCBrushColor(dc, bg);
    SetDCPenColor(dc, borderCol);
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, S(8), S(8));

    wchar_t text[128] = {};
    GetWindowTextW(hw, text, 127);
    if (text[0]) {
        if (pressed) OffsetRect(&rc, 0, 1);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, CLR_TEXT);
        SelectObject(dc, hFontSemi);
        DrawTextW(dc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

// ─────────────────────────── CUSTOM-DRAW TRACKBAR ───────────────────────────
static inline LRESULT HandleTrackbarCustomDraw(LPNMCUSTOMDRAW nm) {
    if (!nm) return CDRF_DODEFAULT;
    if (nm->dwDrawStage == CDDS_PREPAINT) {
        return CDRF_NOTIFYITEMDRAW;
    }
    if (nm->dwDrawStage == CDDS_ITEMPREPAINT) {
        HDC dc = nm->hdc;
        SelectObject(dc, GetStockObject(DC_BRUSH));
        SelectObject(dc, GetStockObject(DC_PEN));

        if (nm->dwItemSpec == TBCD_CHANNEL) {
            RECT rc = nm->rc;
            int cy = (rc.top + rc.bottom) / 2;
            int h = S(4);
            int top = cy - h / 2;
            int bot = top + h;

            SetDCBrushColor(dc, CLR_BORDER);
            SetDCPenColor(dc, CLR_BORDER);
            RoundRect(dc, rc.left, top, rc.right, bot, h, h);

            int minV = (int)SendMessageW(nm->hdr.hwndFrom, TBM_GETRANGEMIN, 0, 0);
            int maxV = (int)SendMessageW(nm->hdr.hwndFrom, TBM_GETRANGEMAX, 0, 0);
            int pos  = (int)SendMessageW(nm->hdr.hwndFrom, TBM_GETPOS, 0, 0);
            if (maxV > minV) {
                float frac = std::clamp((float)(pos - minV) / (float)(maxV - minV), 0.0f, 1.0f);
                int fillRight = rc.left + (int)((rc.right - rc.left) * frac);
                if (fillRight > rc.left + 1) {
                    SetDCBrushColor(dc, CLR_ACCENT);
                    SetDCPenColor(dc, CLR_ACCENT);
                    RoundRect(dc, rc.left, top, fillRight, bot, h, h);
                }
            }
            return CDRF_SKIPDEFAULT;
        }
        if (nm->dwItemSpec == TBCD_THUMB) {
            RECT rc = nm->rc;
            int cx = (rc.left + rc.right) / 2;
            int cy = (rc.top + rc.bottom) / 2;
            int r  = S(7);
            SetDCBrushColor(dc, RGB(242, 245, 250));
            SetDCPenColor(dc, CLR_ACCENT);
            Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
            return CDRF_SKIPDEFAULT;
        }
        if (nm->dwItemSpec == TBCD_TICS) {
            return CDRF_SKIPDEFAULT;
        }
    }
    return CDRF_DODEFAULT;
}
