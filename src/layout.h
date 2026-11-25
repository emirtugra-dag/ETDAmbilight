#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

struct Zone {
    float x0, y0, x1, y1;
};

struct ZoneMeta {
    Zone  zone;
    int   edge;       // 0=Left, 1=Top, 2=Right, 3=Bottom
    int   segment;    // 0=L, 1=T, 2=R, 3=Bottom-Right, 4=Bottom-Left (distinguishes stand gap sides)
    bool  edgeStart;  // True if this LED is the first LED of a segment in strip order
};

struct BottomGapSpan {
    bool  hasGap = false;
    float x0     = 0.5f;
    float x1     = 0.5f;
};

inline int LayoutPosMod(int a, int n) {
    int r = a % n;
    return (r < 0) ? (r + n) : r;
}

inline BottomGapSpan ComputeBottomGapSpan(int BL, int BR, float gapFrac) {
    BL = std::max(0, BL);
    BR = std::max(0, BR);
    gapFrac = std::clamp(gapFrac, 0.0f, 0.70f);
    if (gapFrac <= 0.001f) return {false, 0.5f, 0.5f};

    float avail = 1.0f - gapFrac;
    if (BL > 0 && BR > 0) {
        float wL = avail * ((float)BL / (float)(BL + BR));
        return {true, wL, wL + gapFrac};
    }
    if (BL > 0 && BR == 0) {
        return {true, avail, 1.0f};
    }
    if (BL == 0 && BR > 0) {
        return {true, 0.0f, gapFrac};
    }
    // No bottom LEDs at all, center visual stand
    return {true, 0.5f - gapFrac * 0.5f, 0.5f + gapFrac * 0.5f};
}

// Compute canonical start index for corner preset:
// 0=Sol-Alt, 1=Sol-Ust, 2=Sag-Ust, 3=Sag-Alt, 4=Alt-Ayak (Orta)
inline int CornerStartIndex(int L, int T, int R, int BR, int corner) {
    switch (corner) {
        case 0: return 0;              // Sol-Alt
        case 1: return L;              // Sol-Üst
        case 2: return L + T;          // Sağ-Üst
        case 3: return L + T + R;      // Sağ-Alt
        case 4: return L + T + R + BR; // Alt-Ayak (Orta boşluk)
        default: return L + T + R;
    }
}

// Canonical ring C (clockwise, viewed from front):
// 0: Left (bottom -> top)            [L]
// 1: Top (left -> right)             [T]
// 2: Right (top -> bottom)           [R]
// 3: Bottom-Right (right -> stand)   [BR]
// 4: Bottom-Left (stand -> left)     [BL]
inline std::vector<ZoneMeta> BuildOrderedLayoutMeta(
    int L, int T, int R, int BL, int BR,
    int corner, int dir, int offset, float depth, float gapFrac = 0.0f)
{
    L  = std::max(0, L);
    T  = std::max(0, T);
    R  = std::max(0, R);
    BL = std::max(0, BL);
    BR = std::max(0, BR);
    int N = L + T + R + BL + BR;
    if (N <= 0) return {};

    depth   = std::clamp(depth, 0.01f, 0.45f);
    gapFrac = std::clamp(gapFrac, 0.0f, 0.70f);
    float avail = 1.0f - gapFrac;

    std::vector<ZoneMeta> C;
    C.reserve(N);

    // Left: bottom -> top
    for (int i = 0; i < L; ++i) {
        float y1 = 1.0f - (float)i / (float)L;
        float y0 = 1.0f - (float)(i + 1) / (float)L;
        C.push_back({{0.0f, y0, depth, y1}, 0, 0, false});
    }
    // Top: left -> right
    for (int i = 0; i < T; ++i) {
        float x0 = (float)i / (float)T;
        float x1 = (float)(i + 1) / (float)T;
        C.push_back({{x0, 0.0f, x1, depth}, 1, 1, false});
    }
    // Right: top -> bottom
    for (int i = 0; i < R; ++i) {
        float y0 = (float)i / (float)R;
        float y1 = (float)(i + 1) / (float)R;
        C.push_back({{1.0f - depth, y0, 1.0f, y1}, 2, 2, false});
    }

    // Bottom split into Bottom-Right and Bottom-Left with optional stand gap
    float wL = 0.0f, wR = 0.0f;
    if (BL > 0 && BR > 0) {
        wL = avail * ((float)BL / (float)(BL + BR));
        wR = avail - wL;
    } else if (BL > 0) {
        wL = avail;
    } else if (BR > 0) {
        wR = avail;
    }

    // Bottom-Right: right corner (1.0) -> right side of stand (1.0 - wR)
    for (int i = 0; i < BR; ++i) {
        float x1 = 1.0f - ((float)i / (float)BR) * wR;
        float x0 = 1.0f - ((float)(i + 1) / (float)BR) * wR;
        C.push_back({{x0, 1.0f - depth, x1, 1.0f}, 3, 3, false});
    }
    // Bottom-Left: left side of stand (wL) -> left corner (0.0)
    int segBL = (gapFrac > 0.001f) ? 4 : 3;
    for (int i = 0; i < BL; ++i) {
        float x1 = wL - ((float)i / (float)BL) * wL;
        float x0 = wL - ((float)(i + 1) / (float)BL) * wL;
        C.push_back({{x0, 1.0f - depth, x1, 1.0f}, 3, segBL, false});
    }

    int start = CornerStartIndex(L, T, R, BR, corner);

    std::vector<ZoneMeta> S(N);
    for (int k = 0; k < N; ++k) {
        int idx = (dir == 0)
            ? LayoutPosMod(start + k, N)
            : LayoutPosMod(start - 1 - k, N);
        S[k] = C[idx];
    }

    std::vector<ZoneMeta> out(N);
    for (int k = 0; k < N; ++k) {
        out[k] = S[LayoutPosMod(k + offset, N)];
    }

    for (int k = 0; k < N; ++k) {
        int prev = LayoutPosMod(k - 1, N);
        out[k].edgeStart = (k == 0) || (out[k].segment != out[prev].segment);
    }

    return out;
}

// Compute new offset when flipping direction so LED #1 stays on the exact same physical LED
inline int ComputeOffsetToKeepFirstLed(
    int L, int T, int R, int BL, int BR,
    int corner, int oldDir, int oldOffset, int newDir)
{
    int N = std::max(0, L) + std::max(0, T) + std::max(0, R) + std::max(0, BL) + std::max(0, BR);
    if (N <= 0) return 0;
    int start = CornerStartIndex(std::max(0, L), std::max(0, T), std::max(0, R), std::max(0, BR), corner);
    int c0 = (oldDir == 0)
        ? LayoutPosMod(start + oldOffset, N)
        : LayoutPosMod(start - 1 - oldOffset, N);
    return (newDir == 0)
        ? LayoutPosMod(c0 - start, N)
        : LayoutPosMod(start - 1 - c0, N);
}

inline std::vector<Zone> BuildOrderedZones(
    int L, int T, int R, int BL, int BR,
    int corner, int dir, int offset, float depth, float gapFrac = 0.0f)
{
    auto meta = BuildOrderedLayoutMeta(L, T, R, BL, BR, corner, dir, offset, depth, gapFrac);
    std::vector<Zone> zones;
    zones.reserve(meta.size());
    for (const auto& m : meta) zones.push_back(m.zone);
    return zones;
}

// Overload for uniform bottom without gap (keeps backward API compatibility)
inline std::vector<Zone> BuildOrderedZones(
    int L, int T, int R, int B,
    int corner, int dir, int offset, float depth)
{
    int BL = B / 2;
    int BR = B - BL;
    return BuildOrderedZones(L, T, R, BL, BR, corner, dir, offset, depth, 0.0f);
}
