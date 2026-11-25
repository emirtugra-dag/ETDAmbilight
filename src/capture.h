#pragma once

#include "common.h"
#include "layout.h"

static inline std::vector<MonitorEntry> EnumerateMonitors() {
    std::vector<MonitorEntry> list;
    EnumDisplayMonitors(NULL, NULL, [](HMONITOR hm, HDC, LPRECT, LPARAM lp) -> BOOL {
        auto* out = (std::vector<MonitorEntry>*)lp;
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(hm, &mi)) {
            MonitorEntry e;
            e.hMon = hm;
            e.rect = mi.rcMonitor;
            e.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
            out->push_back(e);
        }
        return TRUE;
    }, (LPARAM)&list);

    if (list.empty()) {
        MonitorEntry e;
        e.rect = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        e.primary = true;
        list.push_back(e);
    }
    return list;
}

class Capturer {
public:
    Capturer() {
        g_captureWakeEvent = CreateEventW(NULL, FALSE, TRUE, NULL);
    }

    ~Capturer() {
        Stop();
        if (g_captureWakeEvent) {
            CloseHandle(g_captureWakeEvent);
            g_captureWakeEvent = NULL;
        }
    }

    void Start() {
        if (running_.load()) return;
        running_.store(true);
        thread_ = std::thread([this] { Loop(); });
    }

    void Stop() {
        running_.store(false);
        WakeCapturer();
        if (thread_.joinable()) thread_.join();
    }

    bool GetColors(std::vector<RGB3>& out) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!valid_ || colors_.empty()) return false;
        out = colors_;
        return true;
    }

private:
    struct PixelZone {
        int x0, y0, x1, y1;
        int edge; // 0=L, 1=T, 2=R, 3=B
    };

    std::atomic<bool>  running_{false};
    std::thread        thread_;
    std::mutex         mtx_;
    std::vector<RGB3>  colors_;
    bool               valid_ = false;

    uint32_t           cachedVersion_ = 0;
    int                cachedMethod_ = -1;
    int                cachedMonIdx_ = -1;
    MonitorEntry       mon_;
    std::vector<PixelZone> pzones_;
    int                edgeCount_[4] = {0, 0, 0, 0};
    D3D11_BOX          edgeBox_[4] = {};
    int                edgeW_ = 0, edgeH_ = 0;

    // DXGI resources
    ID3D11Device*           dev_ = nullptr;
    ID3D11DeviceContext*    ctx_ = nullptr;
    IDXGIOutputDuplication* dupl_ = nullptr;
    ID3D11Texture2D*        staging_ = nullptr;
    UINT                    texW_ = 0, texH_ = 0;
    bool                    dxgiReady_ = false;
    bool                    dxgiHasFrame_ = false;
    int                     dxgiFailCount_ = 0;

    // GDI persistent resources
    HDC     hdcScreen_ = NULL;
    HDC     hdcMem_ = NULL;
    HBITMAP gdiBmp_[4] = {NULL, NULL, NULL, NULL};
    void*   gdiBits_[4] = {nullptr, nullptr, nullptr, nullptr};
    int     gdiBmpW_[4] = {0, 0, 0, 0};
    int     gdiBmpH_[4] = {0, 0, 0, 0};
    bool    gdiDesktopAttached_ = false;

    void ReleaseDXGI() {
        if (staging_) { staging_->Release(); staging_ = nullptr; }
        if (dupl_)    { dupl_->Release();    dupl_ = nullptr; }
        if (ctx_)     { ctx_->Release();     ctx_ = nullptr; }
        if (dev_)     { dev_->Release();     dev_ = nullptr; }
        texW_ = texH_ = 0;
        dxgiReady_ = false;
        dxgiHasFrame_ = false;
    }

    void ReleaseGDI() {
        for (int i = 0; i < 4; ++i) {
            if (gdiBmp_[i]) { DeleteObject(gdiBmp_[i]); gdiBmp_[i] = NULL; }
            gdiBits_[i] = nullptr;
            gdiBmpW_[i] = gdiBmpH_[i] = 0;
        }
        if (hdcMem_)    { DeleteDC(hdcMem_); hdcMem_ = NULL; }
        if (hdcScreen_) { ReleaseDC(NULL, hdcScreen_); hdcScreen_ = NULL; }
    }

    void AttachInteractiveDesktop() {
        if (gdiDesktopAttached_) return;
        HWINSTA hwinsta = OpenWindowStationW(L"WinSta0", FALSE, MAXIMUM_ALLOWED);
        if (hwinsta) SetProcessWindowStation(hwinsta);
        HDESK hdesk = OpenDesktopW(L"Default", 0, FALSE, MAXIMUM_ALLOWED);
        if (hdesk) SetThreadDesktop(hdesk);
        gdiDesktopAttached_ = true;
    }

    void RebuildLayout(const Settings& s) {
        auto mons = EnumerateMonitors();
        int idx = std::clamp(s.monitor_idx, 0, (int)mons.size() - 1);
        mon_ = mons[idx];
        int W = std::max(1, mon_.width());
        int H = std::max(1, mon_.height());

        int L  = s.ambi_left;
        int T  = s.ambi_top;
        int R  = s.ambi_right;
        int BL = s.ambi_bottom_left;
        int BR = s.ambi_bottom_right;
        int B  = BL + BR;
        if (L + T + R + B <= 0) {
            R = std::max(1, s.led_count);
        }

        edgeCount_[0] = L;
        edgeCount_[1] = T;
        edgeCount_[2] = R;
        edgeCount_[3] = B;

        float depth = std::clamp(s.edge_depth_pct, 2, 30) / 100.0f;
        float gapF  = std::clamp(s.bottom_gap_pct, 0, 65) / 100.0f;
        edgeW_ = std::clamp((int)std::ceil(W * depth) + 2, 2, W);
        edgeH_ = std::clamp((int)std::ceil(H * depth) + 2, 2, H);

        // Edge bounding boxes in monitor-local pixel coords
        edgeBox_[0] = {0, 0, 0, (UINT)edgeW_, (UINT)H, 1};
        edgeBox_[1] = {0, 0, 0, (UINT)W, (UINT)edgeH_, 1};
        edgeBox_[2] = {(UINT)(W - edgeW_), 0, 0, (UINT)W, (UINT)H, 1};
        edgeBox_[3] = {0, (UINT)(H - edgeH_), 0, (UINT)W, (UINT)H, 1};

        auto meta = BuildOrderedLayoutMeta(L, T, R, BL, BR, s.start_corner, s.direction, s.led_offset, depth, gapF);
        pzones_.resize(meta.size());
        for (size_t k = 0; k < meta.size(); ++k) {
            const Zone& z = meta[k].zone;
            int px0 = std::clamp((int)std::floor(z.x0 * W), 0, W - 1);
            int py0 = std::clamp((int)std::floor(z.y0 * H), 0, H - 1);
            int px1 = std::clamp((int)std::ceil(z.x1 * W), px0 + 1, W);
            int py1 = std::clamp((int)std::ceil(z.y1 * H), py0 + 1, H);
            pzones_[k] = {px0, py0, px1, py1, meta[k].edge};
        }
    }

    bool InitDXGI() {
        ReleaseDXGI();

        IDXGIFactory1* factory = nullptr;
        HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
        if (FAILED(hr) || !factory) return false;

        IDXGIAdapter1* targetAdapter = nullptr;
        IDXGIOutput*   targetOutput = nullptr;
        IDXGIAdapter1* firstAdapter = nullptr;
        IDXGIOutput*   firstOutput = nullptr;

        for (UINT a = 0; ; ++a) {
            IDXGIAdapter1* adapter = nullptr;
            if (factory->EnumAdapters1(a, &adapter) == DXGI_ERROR_NOT_FOUND) break;

            for (UINT o = 0; ; ++o) {
                IDXGIOutput* output = nullptr;
                if (adapter->EnumOutputs(o, &output) == DXGI_ERROR_NOT_FOUND) break;

                DXGI_OUTPUT_DESC desc = {};
                output->GetDesc(&desc);
                if (!firstOutput) {
                    firstOutput = output;
                    firstOutput->AddRef();
                    firstAdapter = adapter;
                    firstAdapter->AddRef();
                }
                if (mon_.hMon && desc.Monitor == mon_.hMon) {
                    targetOutput = output;
                    targetAdapter = adapter;
                    targetOutput->AddRef();
                    targetAdapter->AddRef();
                    output->Release();
                    break;
                }
                output->Release();
            }
            adapter->Release();
            if (targetOutput) break;
        }
        factory->Release();

        if (!targetOutput) {
            targetOutput = firstOutput;
            targetAdapter = firstAdapter;
        } else {
            if (firstOutput)  firstOutput->Release();
            if (firstAdapter) firstAdapter->Release();
        }

        if (!targetOutput || !targetAdapter) {
            if (targetOutput)  targetOutput->Release();
            if (targetAdapter) targetAdapter->Release();
            return false;
        }

        D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
        hr = D3D11CreateDevice(
            targetAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            NULL, 0, D3D11_SDK_VERSION,
            &dev_, &fl, &ctx_);
        targetAdapter->Release();

        if (FAILED(hr) || !dev_ || !ctx_) {
            targetOutput->Release();
            ReleaseDXGI();
            return false;
        }

        IDXGIOutput1* out1 = nullptr;
        hr = targetOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&out1);
        targetOutput->Release();
        if (FAILED(hr) || !out1) {
            ReleaseDXGI();
            return false;
        }

        hr = out1->DuplicateOutput((IUnknown*)dev_, &dupl_);
        out1->Release();
        if (FAILED(hr) || !dupl_) {
            ReleaseDXGI();
            return false;
        }

        DXGI_OUTDUPL_DESC duplDesc = {};
        dupl_->GetDesc(&duplDesc);
        texW_ = duplDesc.ModeDesc.Width;
        texH_ = duplDesc.ModeDesc.Height;
        if (texW_ == 0 || texH_ == 0) {
            texW_ = (UINT)std::max(1, mon_.width());
            texH_ = (UINT)std::max(1, mon_.height());
        }

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = texW_;
        td.Height = texH_;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = duplDesc.ModeDesc.Format != DXGI_FORMAT_UNKNOWN
                    ? duplDesc.ModeDesc.Format : DXGI_FORMAT_B8G8R8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_STAGING;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        hr = dev_->CreateTexture2D(&td, nullptr, &staging_);
        if (FAILED(hr) || !staging_) {
            ReleaseDXGI();
            return false;
        }

        dxgiReady_ = true;
        dxgiHasFrame_ = false;
        return true;
    }

    // Returns: 1 = frame captured, 0 = timeout (unchanged), -1 = error / lost
    int CaptureDXGI(float sat, bool layoutJustChanged) {
        if (!dxgiReady_ && !InitDXGI()) return -1;

        IDXGIResource* deskRes = nullptr;
        DXGI_OUTDUPL_FRAME_INFO fi = {};
        HRESULT hr = dupl_->AcquireNextFrame(100, &fi, &deskRes);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            if (dxgiHasFrame_ && layoutJustChanged) {
                // Re-average from existing staging texture with new zone layout
                SampleStagingTexture(sat);
                return 1;
            }
            return 0;
        }
        if (FAILED(hr)) {
            if (deskRes) deskRes->Release();
            ReleaseDXGI();
            return -1;
        }

        ID3D11Texture2D* tex = nullptr;
        hr = deskRes->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);
        deskRes->Release();

        if (SUCCEEDED(hr) && tex) {
            D3D11_TEXTURE2D_DESC srcDesc = {};
            tex->GetDesc(&srcDesc);
            if (srcDesc.Width == texW_ && srcDesc.Height == texH_) {
                for (int e = 0; e < 4; ++e) {
                    if (edgeCount_[e] <= 0) continue;
                    D3D11_BOX box = edgeBox_[e];
                    box.right  = std::min(box.right, texW_);
                    box.bottom = std::min(box.bottom, texH_);
                    if (box.right > box.left && box.bottom > box.top) {
                        ctx_->CopySubresourceRegion(staging_, 0, box.left, box.top, 0, tex, 0, &box);
                    }
                }
                dxgiHasFrame_ = true;
            }
            tex->Release();
        }
        dupl_->ReleaseFrame();

        if (!dxgiHasFrame_) return 0;
        return SampleStagingTexture(sat) ? 1 : -1;
    }

    bool SampleStagingTexture(float sat) {
        if (!staging_ || pzones_.empty()) return false;
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx_->Map(staging_, 0, D3D11_MAP_READ, 0, &mapped))) return false;

        const uint8_t* base = (const uint8_t*)mapped.pData;
        const UINT pitch = mapped.RowPitch;
        std::vector<RGB3> out(pzones_.size());

        for (size_t k = 0; k < pzones_.size(); ++k) {
            const PixelZone& pz = pzones_[k];
            int x0 = std::clamp(pz.x0, 0, (int)texW_ - 1);
            int y0 = std::clamp(pz.y0, 0, (int)texH_ - 1);
            int x1 = std::clamp(pz.x1, x0 + 1, (int)texW_);
            int y1 = std::clamp(pz.y1, y0 + 1, (int)texH_);

            int stepX = ((x1 - x0) >= 8) ? 4 : 1;
            int stepY = ((y1 - y0) >= 8) ? 4 : 1;

            uint64_t sr = 0, sg = 0, sb = 0, cnt = 0;
            for (int y = y0; y < y1; y += stepY) {
                const uint8_t* row = base + (size_t)y * pitch;
                for (int x = x0; x < x1; x += stepX) {
                    const uint8_t* px = row + (size_t)x * 4;
                    sb += px[0];
                    sg += px[1];
                    sr += px[2];
                    ++cnt;
                }
            }
            if (cnt > 0) {
                RGB3 c = {(uint8_t)(sr / cnt), (uint8_t)(sg / cnt), (uint8_t)(sb / cnt)};
                out[k] = BoostSaturation(c, sat);
            } else {
                out[k] = {0, 0, 0};
            }
        }
        ctx_->Unmap(staging_, 0);

        std::lock_guard<std::mutex> lk(mtx_);
        colors_.swap(out);
        valid_ = true;
        return true;
    }

    bool EnsureGDIEdgeDIB(int e, int w, int h) {
        if (!hdcScreen_) hdcScreen_ = GetDC(NULL);
        if (!hdcScreen_) return false;
        if (!hdcMem_) hdcMem_ = CreateCompatibleDC(hdcScreen_);
        if (!hdcMem_) return false;

        if (gdiBmp_[e] && gdiBmpW_[e] == w && gdiBmpH_[e] == h && gdiBits_[e])
            return true;

        if (gdiBmp_[e]) { DeleteObject(gdiBmp_[e]); gdiBmp_[e] = NULL; }
        gdiBits_[e] = nullptr;

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        gdiBmp_[e] = CreateDIBSection(hdcMem_, &bi, DIB_RGB_COLORS, &gdiBits_[e], NULL, 0);
        if (!gdiBmp_[e] || !gdiBits_[e]) {
            if (gdiBmp_[e]) { DeleteObject(gdiBmp_[e]); gdiBmp_[e] = NULL; }
            gdiBits_[e] = nullptr;
            gdiBmpW_[e] = gdiBmpH_[e] = 0;
            return false;
        }
        gdiBmpW_[e] = w;
        gdiBmpH_[e] = h;
        return true;
    }

    bool CaptureGDI(float sat) {
        AttachInteractiveDesktop();
        if (pzones_.empty()) return false;

        int W = std::max(1, mon_.width());
        int H = std::max(1, mon_.height());

        const int eX[4] = {0, 0, W - edgeW_, 0};
        const int eY[4] = {0, 0, 0, H - edgeH_};
        const int eW[4] = {edgeW_, W, edgeW_, W};
        const int eH[4] = {H, edgeH_, H, edgeH_};

        bool anyOk = false;
        for (int e = 0; e < 4; ++e) {
            if (edgeCount_[e] <= 0) continue;
            if (!EnsureGDIEdgeDIB(e, eW[e], eH[e])) continue;
            HGDIOBJ old = SelectObject(hdcMem_, gdiBmp_[e]);
            BOOL ok = BitBlt(hdcMem_, 0, 0, eW[e], eH[e],
                             hdcScreen_, mon_.rect.left + eX[e], mon_.rect.top + eY[e], SRCCOPY);
            if (!ok) {
                ok = BitBlt(hdcMem_, 0, 0, eW[e], eH[e],
                            hdcScreen_, mon_.rect.left + eX[e], mon_.rect.top + eY[e], SRCCOPY | CAPTUREBLT);
            }
            SelectObject(hdcMem_, old);
            if (ok) anyOk = true;
        }
        if (!anyOk) return false;

        std::vector<RGB3> out(pzones_.size());
        for (size_t k = 0; k < pzones_.size(); ++k) {
            const PixelZone& pz = pzones_[k];
            int e = pz.edge;
            const DWORD* px = (const DWORD*)gdiBits_[e];
            int bw = gdiBmpW_[e], bh = gdiBmpH_[e];
            if (!px || bw <= 0 || bh <= 0) {
                out[k] = {0, 0, 0};
                continue;
            }
            int x0 = std::clamp(pz.x0 - eX[e], 0, bw - 1);
            int y0 = std::clamp(pz.y0 - eY[e], 0, bh - 1);
            int x1 = std::clamp(pz.x1 - eX[e], x0 + 1, bw);
            int y1 = std::clamp(pz.y1 - eY[e], y0 + 1, bh);

            int stepX = ((x1 - x0) >= 8) ? 4 : 1;
            int stepY = ((y1 - y0) >= 8) ? 4 : 1;

            uint64_t sr = 0, sg = 0, sb = 0, cnt = 0;
            for (int y = y0; y < y1; y += stepY) {
                const DWORD* row = px + (size_t)y * bw;
                for (int x = x0; x < x1; x += stepX) {
                    DWORD p = row[x];
                    sb += (p & 0xFF);
                    sg += ((p >> 8) & 0xFF);
                    sr += ((p >> 16) & 0xFF);
                    ++cnt;
                }
            }
            if (cnt > 0) {
                RGB3 c = {(uint8_t)(sr / cnt), (uint8_t)(sg / cnt), (uint8_t)(sb / cnt)};
                out[k] = BoostSaturation(c, sat);
            } else {
                out[k] = {0, 0, 0};
            }
        }

        std::lock_guard<std::mutex> lk(mtx_);
        colors_.swap(out);
        valid_ = true;
        return true;
    }

    void Loop() {
        while (running_.load()) {
            Settings s;
            {
                std::lock_guard<std::mutex> lk(g_settings_mtx);
                s = g_settings;
            }

            // Sleep with zero CPU when capture is not needed
            bool needCapture = s.leds_enabled && (s.effect == 0);
            if (!needCapture) {
                g_activeCaptureMode.store(0, std::memory_order_relaxed);
                ReleaseDXGI();
                WaitForSingleObject(g_captureWakeEvent, 500);
                continue;
            }

            uint32_t ver = g_layoutVersion.load(std::memory_order_acquire);
            bool layoutChanged = (ver != cachedVersion_) || (s.monitor_idx != cachedMonIdx_);
            if (layoutChanged) {
                RebuildLayout(s);
                if (s.monitor_idx != cachedMonIdx_) {
                    ReleaseDXGI();
                }
                cachedVersion_ = ver;
                cachedMonIdx_ = s.monitor_idx;
            }

            if (s.capture_method != cachedMethod_) {
                if (s.capture_method != 0) ReleaseDXGI();
                dxgiFailCount_ = 0;
                cachedMethod_ = s.capture_method;
            }

            bool useDXGI = (s.capture_method == 0 && dxgiFailCount_ < 3);
            if (useDXGI) {
                int res = CaptureDXGI(s.saturation, layoutChanged);
                if (res >= 0) {
                    g_activeCaptureMode.store(1, std::memory_order_relaxed);
                    dxgiFailCount_ = 0;
                    if (!dxgiHasFrame_) {
                        // Seed an initial frame via GDI if screen hasn't changed yet
                        CaptureGDI(s.saturation);
                    }
                    continue;
                }
                ++dxgiFailCount_;
            }

            g_activeCaptureMode.store(2, std::memory_order_relaxed);
            CaptureGDI(s.saturation);
            int interval = std::max(8, 1000 / std::min(std::max(1, s.target_fps), 60));
            Sleep((DWORD)interval);
        }

        ReleaseDXGI();
        ReleaseGDI();
    }
};
