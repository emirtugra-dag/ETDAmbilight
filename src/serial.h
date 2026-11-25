#pragma once

#include "common.h"
#include "i18n.h"

struct PortInfo {
    std::wstring name;
    std::wstring desc;
};

static inline std::vector<PortInfo> ScanPorts() {
    std::vector<PortInfo> result;
    HDEVINFO hDev = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    if (hDev == INVALID_HANDLE_VALUE) return result;

    SP_DEVINFO_DATA devData = {};
    devData.cbSize = sizeof(devData);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDev, i, &devData); ++i) {
        wchar_t desc[256] = {}, portName[32] = {};
        SetupDiGetDeviceRegistryPropertyW(hDev, &devData, SPDRP_FRIENDLYNAME,
                                          NULL, (BYTE*)desc, sizeof(desc), NULL);
        HKEY hk = SetupDiOpenDevRegKey(hDev, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (hk != INVALID_HANDLE_VALUE) {
            DWORD sz = sizeof(portName), type = 0;
            RegQueryValueExW(hk, L"PortName", 0, &type, (BYTE*)portName, &sz);
            RegCloseKey(hk);
        }
        if (portName[0]) {
            result.push_back({portName, desc[0] ? desc : portName});
        }
    }
    SetupDiDestroyDeviceInfoList(hDev);
    std::sort(result.begin(), result.end(), [](const PortInfo& a, const PortInfo& b) {
        if (a.name.size() != b.name.size()) return a.name.size() < b.name.size();
        return a.name < b.name;
    });
    return result;
}

static inline std::wstring AutoDetectArduino(const std::vector<PortInfo>& ports) {
    for (const auto& p : ports) {
        std::wstring dl = p.desc;
        std::transform(dl.begin(), dl.end(), dl.begin(), ::towlower);
        if (dl.find(L"ch340") != std::wstring::npos || dl.find(L"ch341") != std::wstring::npos)
            return p.name;
    }
    for (const auto& p : ports) {
        std::wstring dl = p.desc;
        std::transform(dl.begin(), dl.end(), dl.begin(), ::towlower);
        if (dl.find(L"arduino") != std::wstring::npos ||
            dl.find(L"usb serial") != std::wstring::npos ||
            dl.find(L"cp210") != std::wstring::npos ||
            dl.find(L"ftdi") != std::wstring::npos)
            return p.name;
    }
    if (!ports.empty()) return ports[0].name;
    return L"";
}

static inline RGB3 ApplyColorOrder(RGB3 c, int order) {
    switch (order) {
        case 1:  return {c.g, c.r, c.b}; // GRB
        case 2:  return {c.b, c.g, c.r}; // BGR
        case 3:  return {c.r, c.b, c.g}; // RBG
        case 4:  return {c.g, c.b, c.r}; // GBR
        case 5:  return {c.b, c.r, c.g}; // BRG
        default: return c;               // RGB
    }
}

static inline void BuildAdalightPacket(const std::vector<RGB3>& leds, int colorOrder, std::vector<uint8_t>& pkt) {
    size_t n = leds.size();
    if (n == 0) {
        pkt.clear();
        return;
    }
    pkt.resize(6 + n * 3);
    uint8_t hi = (uint8_t)((n - 1) >> 8);
    uint8_t lo = (uint8_t)((n - 1) & 0xFF);
    uint8_t chk = hi ^ lo ^ 0x55;
    pkt[0] = 'A';
    pkt[1] = 'd';
    pkt[2] = 'a';
    pkt[3] = hi;
    pkt[4] = lo;
    pkt[5] = chk;
    if (colorOrder == 0) {
        memcpy(&pkt[6], leds.data(), n * 3);
    } else {
        for (size_t i = 0; i < n; ++i) {
            RGB3 c = ApplyColorOrder(leds[i], colorOrder);
            pkt[6 + i * 3 + 0] = c.r;
            pkt[6 + i * 3 + 1] = c.g;
            pkt[6 + i * 3 + 2] = c.b;
        }
    }
}

static inline void BuildAdalightPacket(const std::vector<RGB3>& leds, std::vector<uint8_t>& pkt) {
    BuildAdalightPacket(leds, 0, pkt);
}

class SerialPort {
public:
    HANDLE       hCom = INVALID_HANDLE_VALUE;
    HANDLE       hWriteEvent = NULL;
    bool         connected = false;
    std::wstring lastError;
    std::mutex   mtx;

    SerialPort() {
        hWriteEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    }

    ~SerialPort() {
        Disconnect();
        if (hWriteEvent) {
            CloseHandle(hWriteEvent);
            hWriteEvent = NULL;
        }
    }

    bool Connect(const std::wstring& portName, DWORD baud) {
        std::lock_guard<std::mutex> lk(mtx);
        Disconnect_();
        if (portName.empty() || portName == L"AUTO") {
            lastError = LStr(L"Port bulunamadı", L"Port not found");
            return false;
        }
        std::wstring path = L"\\\\.\\" + portName;
        hCom = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL,
                           OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
        if (hCom == INVALID_HANDLE_VALUE) {
            lastError = std::wstring(LStr(L"Port açılamadı (hata: ", L"Cannot open port (error: "))
                        + std::to_wstring(GetLastError()) + L")";
            return false;
        }
        DCB dcb = {};
        dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(hCom, &dcb)) {
            Disconnect_();
            lastError = LStr(L"GetCommState başarısız", L"GetCommState failed");
            return false;
        }
        dcb.BaudRate = baud;
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity = NOPARITY;
        dcb.fBinary = TRUE;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        if (!SetCommState(hCom, &dcb)) {
            Disconnect_();
            lastError = LStr(L"SetCommState başarısız", L"SetCommState failed");
            return false;
        }
        COMMTIMEOUTS to = {};
        to.WriteTotalTimeoutConstant = 50;
        SetCommTimeouts(hCom, &to);
        PurgeComm(hCom, PURGE_RXCLEAR | PURGE_TXCLEAR);
        connected = true;
        lastError.clear();
        return true;
    }

    void Disconnect() {
        std::lock_guard<std::mutex> lk(mtx);
        Disconnect_();
    }

    bool Send(const uint8_t* data, size_t len) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected || hCom == INVALID_HANDLE_VALUE || len == 0) return false;
        if (hWriteEvent) ResetEvent(hWriteEvent);
        OVERLAPPED ov = {};
        ov.hEvent = hWriteEvent;
        DWORD written = 0;
        BOOL ok = WriteFile(hCom, data, (DWORD)len, &written, &ov);
        if (!ok) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                DWORD wait = WaitForSingleObject(hWriteEvent, 50);
                if (wait == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(hCom, &ov, &written, FALSE)) {
                        Disconnect_();
                        lastError = LStr(L"Yazma başarısız", L"Write failed");
                    }
                } else {
                    CancelIo(hCom);
                    Disconnect_();
                    lastError = LStr(L"Yazma zaman aşımı", L"Write timeout");
                }
            } else {
                Disconnect_();
                lastError = std::wstring(LStr(L"Bağlantı koptu (", L"Disconnected ("))
                            + std::to_wstring(err) + L")";
            }
        }
        return connected;
    }

    void SendBlack(int ledCount) {
        if (ledCount <= 0) return;
        std::vector<RGB3> black(ledCount, {0, 0, 0});
        std::vector<uint8_t> pkt;
        BuildAdalightPacket(black, pkt);
        Send(pkt.data(), pkt.size());
    }

private:
    void Disconnect_() {
        if (hCom != INVALID_HANDLE_VALUE) {
            CloseHandle(hCom);
            hCom = INVALID_HANDLE_VALUE;
        }
        connected = false;
    }
};
