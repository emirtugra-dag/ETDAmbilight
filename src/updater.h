#pragma once

#include "common.h"
#include "i18n.h"

struct UpdateCheckResult {
    bool ok        = false;
    bool hasUpdate = false;
    std::wstring latestTag;
    std::wstring setupUrl;
    std::wstring releaseUrl;
};

static inline std::vector<int> ParseVersionParts(const std::wstring& s) {
    std::vector<int> parts;
    int cur = 0;
    bool inNum = false;
    for (wchar_t ch : s) {
        if (ch >= L'0' && ch <= L'9') {
            cur = cur * 10 + (ch - L'0');
            inNum = true;
        } else if (inNum) {
            parts.push_back(cur);
            cur = 0;
            inNum = false;
        }
    }
    if (inNum) parts.push_back(cur);
    return parts;
}

static inline bool IsVersionNewer(const std::wstring& remoteVer, const std::wstring& localVer) {
    auto r = ParseVersionParts(remoteVer);
    auto l = ParseVersionParts(localVer);
    size_t n = std::max(r.size(), l.size());
    r.resize(n, 0);
    l.resize(n, 0);
    for (size_t i = 0; i < n; ++i) {
        if (r[i] > l[i]) return true;
        if (r[i] < l[i]) return false;
    }
    return false;
}

static inline std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), NULL, 0);
    if (n <= 0) return L"";
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), out.data(), n);
    return out;
}

static inline std::string ExtractJsonStringValue(const std::string& json, const std::string& key, size_t startPos = 0) {
    std::string pat = "\"" + key + "\"";
    size_t pos = json.find(pat, startPos);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + pat.size());
    if (pos == std::string::npos) return "";
    size_t q1 = json.find('"', pos + 1);
    if (q1 == std::string::npos) return "";
    size_t q2 = q1 + 1;
    while (q2 < json.size()) {
        if (json[q2] == '"' && json[q2 - 1] != '\\') break;
        ++q2;
    }
    if (q2 >= json.size()) return "";
    return json.substr(q1 + 1, q2 - q1 - 1);
}

static inline bool WinHttpFetchBytes(const std::wstring& url, std::string& outBody) {
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {}, path[1024] = {};
    uc.lpszHostName = host; uc.dwHostNameLength = 256;
    uc.lpszUrlPath  = path; uc.dwUrlPathLength  = 1024;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;

    HINTERNET hSess = WinHttpOpen(L"ETDAmbilight/2.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return false;

    WinHttpSetTimeouts(hSess, 8000, 8000, 8000, 15000);
    HINTERNET hConn = WinHttpConnect(hSess, host, uc.nPort, 0);
    if (!hConn) { WinHttpCloseHandle(hSess); return false; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"GET", path, NULL, WINHTTP_NO_REFERER,
                                        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return false; }

    WinHttpAddRequestHeaders(hReq, L"Accept: application/vnd.github+json\r\n", (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

    bool ok = false;
    if (WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hReq, NULL)) {
        DWORD status = 0, ssz = sizeof(status);
        WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            NULL, &status, &ssz, NULL);
        if (status >= 200 && status < 300) {
            outBody.clear();
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(hReq, &avail) && avail > 0) {
                std::string chunk(avail, '\0');
                DWORD read = 0;
                if (WinHttpReadData(hReq, chunk.data(), avail, &read) && read > 0) {
                    chunk.resize(read);
                    outBody += chunk;
                } else {
                    break;
                }
                if (outBody.size() > 64 * 1024 * 1024) break;
            }
            ok = !outBody.empty();
        }
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSess);
    return ok;
}

static inline void StartUpdateCheck(HWND hwnd) {
    std::thread([hwnd]() {
        auto* res = new UpdateCheckResult();
        std::wstring apiUrl = std::wstring(L"https://api.github.com/repos/") + APP_REPO + L"/releases/latest";
        std::string body;
        if (WinHttpFetchBytes(apiUrl, body)) {
            std::string tag = ExtractJsonStringValue(body, "tag_name");
            std::string htmlUrl = ExtractJsonStringValue(body, "html_url");
            std::string setupUrl;
            size_t pos = 0;
            while (true) {
                size_t found = body.find("\"browser_download_url\"", pos);
                if (found == std::string::npos) break;
                std::string u = ExtractJsonStringValue(body, "browser_download_url", found);
                if (u.find("Setup.exe") != std::string::npos || u.find("setup.exe") != std::string::npos) {
                    setupUrl = u;
                    break;
                }
                if (setupUrl.empty() && u.find(".exe") != std::string::npos) {
                    setupUrl = u;
                }
                pos = found + 20;
            }
            if (!tag.empty()) {
                res->ok = true;
                res->latestTag = Utf8ToWide(tag);
                res->releaseUrl = htmlUrl.empty()
                    ? (std::wstring(L"https://github.com/") + APP_REPO + L"/releases/latest")
                    : Utf8ToWide(htmlUrl);
                res->setupUrl = Utf8ToWide(setupUrl);
                res->hasUpdate = IsVersionNewer(res->latestTag, APP_VER);
            }
        }
        if (IsWindow(hwnd)) {
            PostMessageW(hwnd, WM_UPDATE_CHECK_DONE, 0, (LPARAM)res);
        } else {
            delete res;
        }
    }).detach();
}

static inline bool DownloadAndLaunchSetup(const std::wstring& setupUrl) {
    if (setupUrl.empty()) return false;
    std::string bin;
    if (!WinHttpFetchBytes(setupUrl, bin) || bin.size() < 32768) return false;
    if (bin[0] != 'M' || bin[1] != 'Z') return false;

    wchar_t tmpDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tmpDir);
    std::wstring setupPath = std::wstring(tmpDir) + L"ETDAmbilight-Setup-Update.exe";

    HANDLE hf = CreateFileW(setupPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool wOk = WriteFile(hf, bin.data(), (DWORD)bin.size(), &written, NULL) && written == (DWORD)bin.size();
    CloseHandle(hf);
    if (!wOk) return false;

    HINSTANCE res = ShellExecuteW(NULL, L"open", setupPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)res > 32);
}
