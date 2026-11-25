#pragma once

#include "common.h"
#include "layout.h"

// Rich colorful Ambilight calibration scene in normalized [0..1]x[0..1] space
static inline RGB3 SampleCalibrationScene(float u, float v) {
    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);

    // 4 vivid corners: TL=Cyan/Blue, TR=Magenta, BL=Sunset Orange, BR=Emerald Green
    const float cTL[3] = {  15.0f, 165.0f, 255.0f };
    const float cTR[3] = { 255.0f,  40.0f, 175.0f };
    const float cBL[3] = { 255.0f,  90.0f,  15.0f };
    const float cBR[3] = {  30.0f, 235.0f, 110.0f };

    float rgb[3];
    for (int i = 0; i < 3; ++i) {
        float top = cTL[i] * (1.0f - u) + cTR[i] * u;
        float bot = cBL[i] * (1.0f - u) + cBR[i] * u;
        rgb[i] = top * (1.0f - v) + bot * v;
    }

    // Soft warm center glow
    float dx = u - 0.5f;
    float dy = v - 0.42f;
    float dist = sqrtf(dx * dx + dy * dy);
    float glow = std::max(0.0f, 1.0f - dist * 2.6f);
    glow = glow * glow * 0.45f;
    rgb[0] = rgb[0] * (1.0f - glow) + 255.0f * glow;
    rgb[1] = rgb[1] * (1.0f - glow) + 225.0f * glow;
    rgb[2] = rgb[2] * (1.0f - glow) + 170.0f * glow;

    return {
        (uint8_t)std::clamp((int)rgb[0], 0, 255),
        (uint8_t)std::clamp((int)rgb[1], 0, 255),
        (uint8_t)std::clamp((int)rgb[2], 0, 255)
    };
}

static const wchar_t* EFFECT_NAMES_TR[] = {
    L"Ambilight",
    L"Rainbow",
    L"Rainbow Wave",
    L"Ateş (Fire)",
    L"Nefes (Breathing)",
    L"Renk Silme",
    L"Strobe",
    L"Gradient",
    L"Kıvılcım",
    L"Polis",
    L"Sabit Renk",
    L"Kalibrasyon (Yön Testi)",
};
static const wchar_t* EFFECT_NAMES_EN[] = {
    L"Ambilight",
    L"Rainbow",
    L"Rainbow Wave",
    L"Fire",
    L"Breathing",
    L"Color Wipe",
    L"Strobe",
    L"Gradient",
    L"Sparkle",
    L"Police",
    L"Solid Color",
    L"Calibration (Direction Test)",
};
static const int EFFECT_COUNT = 12;

static inline const wchar_t* EffectName(int idx) {
    int i = std::clamp(idx, 0, EFFECT_COUNT - 1);
    return (g_lang.load(std::memory_order_relaxed) == 1) ? EFFECT_NAMES_EN[i] : EFFECT_NAMES_TR[i];
}

class EffectsEngine {
public:
    struct RGBf { float r, g, b; };

    float              t_ = 0.0f;
    float              lastDt_ = 1.0f / 60.0f;
    std::vector<int>   fire_heat_;
    std::vector<RGB3>  sparkle_buf_;
    std::vector<RGBf>  prev_ambi_;
    std::mt19937       rng_{42};

    // Cached gamma LUT to avoid calling powf() per LED per frame
    float              lutGamma_ = -1.0f;
    uint8_t            gammaLut_[256] = {};

    // Cached corner mask for Calibration effect
    uint32_t           cachedLayoutVer_ = 0;
    std::vector<bool>  cornerMask_;

    void Tick(float dt) {
        t_ += dt;
        lastDt_ = std::clamp(dt, 0.001f, 0.25f);
    }

    void EnsureGammaLUT(float gamma) {
        if (fabsf(gamma - lutGamma_) < 0.001f) return;
        lutGamma_ = gamma;
        for (int i = 0; i < 256; ++i) {
            gammaLut_[i] = GammaCorrect((uint8_t)i, gamma);
        }
    }

    void EnsureCornerMask(const Settings& s) {
        uint32_t ver = g_layoutVersion.load(std::memory_order_acquire);
        if (ver == cachedLayoutVer_ && !cornerMask_.empty()) return;
        cachedLayoutVer_ = ver;

        int L  = s.ambi_left, T = s.ambi_top, R = s.ambi_right;
        int BL = s.ambi_bottom_left, BR = s.ambi_bottom_right;
        if (L + T + R + BL + BR <= 0) R = std::max(1, s.led_count);
        auto meta = BuildOrderedLayoutMeta(
            L, T, R, BL, BR, s.start_corner, s.direction, s.led_offset,
            std::clamp(s.edge_depth_pct, 2, 30) / 100.0f,
            std::clamp(s.bottom_gap_pct, 0, 65) / 100.0f);
        cornerMask_.resize(meta.size());
        for (size_t i = 0; i < meta.size(); ++i) {
            cornerMask_[i] = meta[i].edgeStart;
        }
    }

    void Render(const Settings& s, const std::vector<RGB3>* screenColors, std::vector<RGB3>& out) {
        int n = std::max(1, s.led_count);
        float speed = s.effect_speed / 50.0f;

        if (g_wizardActive.load(std::memory_order_acquire)) {
            int step = g_wizardStep.load(std::memory_order_relaxed);
            if (step == 0) {
                // Step 0: Pure primary color test (Red / Green / Blue) for channel order
                int prim = g_wizardPrimColor.load(std::memory_order_relaxed);
                RGB3 pcol = (prim == 1) ? RGB3{0, 255, 0} : ((prim == 2) ? RGB3{0, 0, 255} : RGB3{255, 0, 0});
                out.assign(n, pcol);
                return;
            }
            if (step == 1) {
                // Step 1: Pure White balance calibration
                out.assign(n, {255, 255, 255});
            } else if (step == 2) {
                // Step 2: Gamma calibration (first half mid-gray 128, second half smooth ramp 24..255)
                out.resize(n);
                int half = std::max(1, n / 2);
                for (int i = 0; i < n; ++i) {
                    uint8_t v = (i < half)
                        ? 128
                        : (uint8_t)(24 + (231 * (i - half)) / std::max(1, n - half - 1));
                    out[i] = {v, v, v};
                }
            } else {
                // Step 3: Live Sample Image Ambilight sync
                int L  = s.ambi_left, T = s.ambi_top, R = s.ambi_right;
                int BL = s.ambi_bottom_left, BR = s.ambi_bottom_right;
                if (L + T + R + BL + BR <= 0) R = n;
                auto meta = BuildOrderedLayoutMeta(
                    L, T, R, BL, BR, s.start_corner, s.direction, s.led_offset,
                    std::clamp(s.edge_depth_pct, 2, 30) / 100.0f,
                    std::clamp(s.bottom_gap_pct, 0, 65) / 100.0f);
                out.resize(meta.size());
                for (size_t i = 0; i < meta.size(); ++i) {
                    float cu = (meta[i].zone.x0 + meta[i].zone.x1) * 0.5f;
                    float cv = (meta[i].zone.y0 + meta[i].zone.y1) * 0.5f;
                    out[i] = BoostSaturation(SampleCalibrationScene(cu, cv), s.saturation);
                }
                n = (int)out.size();
            }

            EnsureGammaLUT(s.gamma);
            float fb = (step == 3) ? (std::clamp(s.brightness, 0, 200) / 100.0f) : 1.0f;
            float gr = fb * (std::clamp(s.gain_r, 20, 100) / 100.0f);
            float gg = fb * (std::clamp(s.gain_g, 20, 100) / 100.0f);
            float gb = fb * (std::clamp(s.gain_b, 20, 100) / 100.0f);
            for (auto& c : out) {
                c.r = gammaLut_[std::clamp((int)(c.r * gr), 0, 255)];
                c.g = gammaLut_[std::clamp((int)(c.g * gg), 0, 255)];
                c.b = gammaLut_[std::clamp((int)(c.b * gb), 0, 255)];
            }
            return;
        }

        switch (s.effect) {
            case 0:  EffAmbilight(s, n, screenColors, out); break;
            case 1:  EffRainbow(n, speed, out); break;
            case 2:  EffRainbowWave(n, speed, out); break;
            case 3:  EffFire(n, out); break;
            case 4:  EffBreathing(s, n, speed, out); break;
            case 5:  EffColorWipe(s, n, speed, out); break;
            case 6:  EffStrobe(s, n, speed, out); break;
            case 7:  EffGradient(s, n, speed, out); break;
            case 8:  EffSparkle(s, n, speed, out); break;
            case 9:  EffPolice(n, speed, out); break;
            case 11: EffCalibration(s, n, out); break;
            default: EffStatic(s, n, out); break;
        }

        EnsureGammaLUT(s.gamma);
        float fb = std::clamp(s.brightness, 0, 200) / 100.0f;
        float fr = fb * (std::clamp(s.gain_r, 20, 100) / 100.0f);
        float fg = fb * (std::clamp(s.gain_g, 20, 100) / 100.0f);
        float fbl = fb * (std::clamp(s.gain_b, 20, 100) / 100.0f);
        for (auto& c : out) {
            int r = std::clamp((int)(c.r * fr), 0, 255);
            int g = std::clamp((int)(c.g * fg), 0, 255);
            int b = std::clamp((int)(c.b * fbl), 0, 255);
            c.r = gammaLut_[r];
            c.g = gammaLut_[g];
            c.b = gammaLut_[b];
        }
    }

private:
    void EffStatic(const Settings& s, int n, std::vector<RGB3>& out) {
        out.assign(n, WinColorToRGB3(s.color1));
    }

    void EffRainbow(int n, float speed, std::vector<RGB3>& out) {
        out.resize(n);
        for (int i = 0; i < n; ++i) {
            float h = fmodf(t_ * speed * 0.3f + (float)i / n, 1.0f);
            out[i] = HsvToRgb(h, 1.0f, 1.0f);
        }
    }

    void EffRainbowWave(int n, float speed, std::vector<RGB3>& out) {
        out.resize(n);
        for (int i = 0; i < n; ++i) {
            float h = fmodf(t_ * speed * 0.2f + (float)i / n * 2.0f, 1.0f);
            out[i] = HsvToRgb(h, 1.0f, 1.0f);
        }
    }

    void EffFire(int n, std::vector<RGB3>& out) {
        if ((int)fire_heat_.size() != n) fire_heat_.assign(n, 0);
        const int COOLING = 60, SPARKING = 120;
        std::uniform_int_distribution<int> rnd(0, 255);
        for (int i = 0; i < n; ++i) {
            int cool = std::max(1, COOLING * 10 / n + 1);
            fire_heat_[i] = std::max(0, fire_heat_[i] - rnd(rng_) % cool);
        }
        for (int i = n - 1; i >= 2; --i) {
            fire_heat_[i] = (fire_heat_[i - 1] + fire_heat_[i - 2] + fire_heat_[i - 2]) / 3;
        }
        if (rnd(rng_) < SPARKING) {
            int y = rnd(rng_) % std::min(7, n);
            fire_heat_[y] = std::min(255, fire_heat_[y] + 160 + rnd(rng_) % 96);
        }
        out.resize(n);
        for (int i = 0; i < n; ++i) {
            int t192 = (int)((float)fire_heat_[i] / 255.0f * 191.0f);
            int ramp = (t192 & 0x3F) << 2;
            if (t192 > 0x80)      out[i] = {255, 255, (uint8_t)ramp};
            else if (t192 > 0x40) out[i] = {255, (uint8_t)ramp, 0};
            else                  out[i] = {(uint8_t)ramp, 0, 0};
        }
    }

    void EffBreathing(const Settings& s, int n, float speed, std::vector<RGB3>& out) {
        float val = (sinf(t_ * speed * (float)M_PI) + 1.0f) / 2.0f;
        RGB3 base = WinColorToRGB3(s.color1);
        RGB3 c = {(uint8_t)(base.r * val), (uint8_t)(base.g * val), (uint8_t)(base.b * val)};
        out.assign(n, c);
    }

    void EffColorWipe(const Settings& s, int n, float speed, std::vector<RGB3>& out) {
        RGB3 c1 = WinColorToRGB3(s.color1);
        RGB3 c2 = WinColorToRGB3(s.color2);
        int pos = (int)(t_ * speed * n * 0.5f) % (n * 2);
        out.resize(n);
        for (int i = 0; i < n; ++i) {
            if (pos < n) out[i] = (i < pos) ? c1 : c2;
            else         out[i] = (i < (pos - n)) ? c2 : c1;
        }
    }

    void EffStrobe(const Settings& s, int n, float speed, std::vector<RGB3>& out) {
        bool on = ((int)(t_ * speed * 10.0f)) % 2 == 0;
        out.assign(n, on ? WinColorToRGB3(s.color1) : RGB3{0, 0, 0});
    }

    void EffGradient(const Settings& s, int n, float speed, std::vector<RGB3>& out) {
        RGB3 c1 = WinColorToRGB3(s.color1);
        RGB3 c2 = WinColorToRGB3(s.color2);
        float offset = (sinf(t_ * speed * 0.5f) + 1.0f) / 2.0f;
        out.resize(n);
        for (int i = 0; i < n; ++i) {
            float t = fmodf((float)i / (n > 1 ? n - 1 : 1) + offset, 1.0f);
            out[i] = {
                (uint8_t)(c1.r * (1.0f - t) + c2.r * t),
                (uint8_t)(c1.g * (1.0f - t) + c2.g * t),
                (uint8_t)(c1.b * (1.0f - t) + c2.b * t)
            };
        }
    }

    void EffSparkle(const Settings& s, int n, float speed, std::vector<RGB3>& out) {
        if ((int)sparkle_buf_.size() != n) sparkle_buf_.assign(n, {0, 0, 0});
        for (auto& c : sparkle_buf_) {
            c.r = (uint8_t)std::max(0, (int)c.r - 25);
            c.g = (uint8_t)std::max(0, (int)c.g - 25);
            c.b = (uint8_t)std::max(0, (int)c.b - 25);
        }
        int count = std::max(1, (int)(n * 0.08f));
        std::uniform_int_distribution<int> rnd(0, n - 1);
        RGB3 base = WinColorToRGB3(s.color1);
        for (int i = 0; i < count; ++i) {
            if (rng_() % 100 < (size_t)(speed * 40.0f)) {
                sparkle_buf_[rnd(rng_)] = base;
            }
        }
        out = sparkle_buf_;
    }

    void EffPolice(int n, float speed, std::vector<RGB3>& out) {
        int half = n / 2;
        int phase = (int)(t_ * speed * 4.0f) % 4;
        out.resize(n);
        for (int i = 0; i < n; ++i) {
            if (i < half) out[i] = (phase < 2) ? RGB3{255, 0, 0} : RGB3{0, 0, 0};
            else          out[i] = (phase < 2) ? RGB3{0, 0, 0}   : RGB3{0, 0, 255};
        }
    }

    void EffCalibration(const Settings& s, int n, std::vector<RGB3>& out) {
        EnsureCornerMask(s);
        out.assign(n, {0, 0, 0});

        // Corner LEDs (first LED of each edge) in yellow 80
        for (int i = 0; i < n && i < (int)cornerMask_.size(); ++i) {
            if (cornerMask_[i]) out[i] = {80, 80, 0};
        }

        // LED #1 (index 0) full red
        if (n >= 1) out[0] = {255, 0, 0};

        // Indices 1..4 green with decreasing brightness 200 / 150 / 100 / 60
        const uint8_t greenVals[4] = {200, 150, 100, 60};
        for (int i = 1; i <= 4 && i < n; ++i) {
            out[i] = {0, greenVals[i - 1], 0};
        }

        // Last LED blue
        if (n >= 2) out[n - 1] = {0, 0, 255};

        // Travelling white dot doing 1 full revolution per second
        int dotIdx = (int)(fmodf(t_, 1.0f) * n) % n;
        if (dotIdx < 0) dotIdx += n;
        out[dotIdx] = {255, 255, 255};
    }

    void EffAmbilight(const Settings& s, int n, const std::vector<RGB3>* sc, std::vector<RGB3>& out) {
        if (sc && !sc->empty()) {
            int m = (int)sc->size();
            std::vector<RGB3> mapped(n);
            if (m == n) {
                mapped = *sc;
            } else {
                for (int i = 0; i < n; ++i) {
                    float srcPos = (float)i * (m - 1) / std::max(1, n - 1);
                    int idx0 = (int)srcPos;
                    int idx1 = std::min(m - 1, idx0 + 1);
                    float f = srcPos - idx0;
                    mapped[i] = {
                        (uint8_t)((*sc)[idx0].r * (1.0f - f) + (*sc)[idx1].r * f),
                        (uint8_t)((*sc)[idx0].g * (1.0f - f) + (*sc)[idx1].g * f),
                        (uint8_t)((*sc)[idx0].b * (1.0f - f) + (*sc)[idx1].b * f)
                    };
                }
            }

            float sm = std::clamp(s.smoothing, 0.0f, 0.98f);
            float alpha = (sm <= 0.001f) ? 1.0f : (1.0f - powf(sm, std::clamp(lastDt_ * 60.0f, 0.05f, 4.0f)));
            if ((int)prev_ambi_.size() != n) {
                prev_ambi_.resize(n);
                for (int i = 0; i < n; ++i) {
                    prev_ambi_[i] = {(float)mapped[i].r, (float)mapped[i].g, (float)mapped[i].b};
                }
            }
            out.resize(n);
            for (int i = 0; i < n; ++i) {
                prev_ambi_[i].r += ((float)mapped[i].r - prev_ambi_[i].r) * alpha;
                prev_ambi_[i].g += ((float)mapped[i].g - prev_ambi_[i].g) * alpha;
                prev_ambi_[i].b += ((float)mapped[i].b - prev_ambi_[i].b) * alpha;
                out[i] = {
                    (uint8_t)std::clamp((int)(prev_ambi_[i].r + 0.5f), 0, 255),
                    (uint8_t)std::clamp((int)(prev_ambi_[i].g + 0.5f), 0, 255),
                    (uint8_t)std::clamp((int)(prev_ambi_[i].b + 0.5f), 0, 255)
                };
            }
            return;
        }

        if ((int)prev_ambi_.size() == n) {
            out.resize(n);
            for (int i = 0; i < n; ++i) {
                out[i] = {
                    (uint8_t)std::clamp((int)(prev_ambi_[i].r + 0.5f), 0, 255),
                    (uint8_t)std::clamp((int)(prev_ambi_[i].g + 0.5f), 0, 255),
                    (uint8_t)std::clamp((int)(prev_ambi_[i].b + 0.5f), 0, 255)
                };
            }
        } else {
            out.assign(n, {0, 0, 0});
        }
    }
};
