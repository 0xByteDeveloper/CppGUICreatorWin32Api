// ============================================================================
//  Util.cpp — implementation of helpers declared in stdafx.h
// ============================================================================
#include "stdafx.h"
#include <cstdarg>
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------
std::wstring Utf8ToW(const std::string& s)
{
    if (s.empty()) return std::wstring();
#ifdef _WIN32
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w((size_t)n, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
#else
    std::wstring w; w.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { w.push_back((wchar_t)c); i += 1; }
        else if ((c >> 5) == 0x6 && i + 1 < s.size()) {
            w.push_back((wchar_t)(((c & 0x1F) << 6) | (s[i+1] & 0x3F))); i += 2;
        } else if ((c >> 4) == 0xE && i + 2 < s.size()) {
            w.push_back((wchar_t)(((c & 0x0F) << 12) | ((s[i+1] & 0x3F) << 6) | (s[i+2] & 0x3F))); i += 3;
        } else { w.push_back(L'?'); i += 1; }
    }
    return w;
#endif
}

std::string WToUtf8(const std::wstring& w)
{
    if (w.empty()) return std::string();
#ifdef _WIN32
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s((size_t)n, '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
#else
    std::string s; s.reserve(w.size());
    for (wchar_t c : w) {
        unsigned v = (unsigned)c;
        if (v < 0x80) s.push_back((char)v);
        else if (v < 0x800) { s.push_back((char)(0xC0 | (v >> 6))); s.push_back((char)(0x80 | (v & 0x3F))); }
        else { s.push_back((char)(0xE0 | (v >> 12))); s.push_back((char)(0x80 | ((v >> 6) & 0x3F))); s.push_back((char)(0x80 | (v & 0x3F))); }
    }
    return s;
#endif
}

// ---------------------------------------------------------------------------
// Strings
// ---------------------------------------------------------------------------
static std::wstring TrimRef(const std::wstring& s)
{
    size_t a = 0, b = s.size();
    while (a < b && iswspace((wint_t)s[a])) a++;
    while (b > a && iswspace((wint_t)s[b - 1])) b--;
    return s.substr(a, b - a);
}
std::wstring TrimW(const std::wstring& s) { return TrimRef(s); }

std::wstring LowerW(const std::wstring& s)
{
    std::wstring r(s);
    for (auto& c : r) c = (wchar_t)towlower((wint_t)c);
    return r;
}
std::wstring UpperW(const std::wstring& s)
{
    std::wstring r(s);
    for (auto& c : r) c = (wchar_t)towupper((wint_t)c);
    return r;
}
bool StartsWithW(const std::wstring& s, const std::wstring& p)
{
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}
bool EndsWithW(const std::wstring& s, const std::wstring& p)
{
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}
bool ContainsI(const std::wstring& s, const std::wstring& sub)
{
    if (sub.empty()) return true;
    return LowerW(s).find(LowerW(sub)) != std::wstring::npos;
}
std::vector<std::wstring> SplitW(const std::wstring& s, wchar_t sep)
{
    std::vector<std::wstring> out;
    std::wstring cur;
    for (wchar_t c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}
std::wstring JoinW(const std::vector<std::wstring>& v, const std::wstring& sep)
{
    std::wstring r;
    for (size_t i = 0; i < v.size(); i++) {
        if (i) r += sep;
        r += v[i];
    }
    return r;
}
std::wstring ReplaceW(std::wstring s, const std::wstring& from, const std::wstring& to)
{
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::wstring::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}
std::wstring Fmt(const wchar_t* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    wchar_t stackbuf[1024];
    int n = vswprintf(stackbuf, 1024, fmt, ap);
    va_end(ap);
    if (n < 0) return std::wstring();
    if (n < 1024) return std::wstring(stackbuf, (size_t)n);
    // grew beyond the stack buffer: retry with heap storage
    std::vector<wchar_t> heap((size_t)n + 64, L'\0');
    va_start(ap, fmt);
    n = vswprintf(&heap[0], heap.size(), fmt, ap);
    va_end(ap);
    if (n < 0) return std::wstring();
    return std::wstring(&heap[0], (size_t)n);
}

// Turkish characters -> ASCII, keep [A-Za-z0-9_]
std::wstring NormalizeIdentW(const std::wstring& s)
{
    static const std::wstring from = L"çğıöşüÇĞİÖŞÜ";
    static const std::wstring to   = L"cgiosucgiosu";
    std::wstring r;
    for (wchar_t c : s) {
        size_t p = from.find(c);
        if (p != std::wstring::npos) { r.push_back(to[p]); continue; }
        if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9') || c == L'_') {
            r.push_back(c); continue;
        }
        if (c >= 0x80) { r.push_back(L'_'); continue; }  // other non-ascii
        if (iswalnum((wint_t)c)) r.push_back(c);
        else r.push_back(L'_');
    }
    if (!r.empty() && r[0] >= L'0' && r[0] <= L'9') r.insert(r.begin(), L'_');
    return r;
}

bool IsAsciiW(const std::wstring& s)
{
    for (wchar_t c : s) if ((unsigned)c >= 0x80) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Files  (wide-path safe: _wfopen on Windows, narrow fopen elsewhere)
// ---------------------------------------------------------------------------
#ifdef _WIN32
static FILE* OpenFileC(const std::wstring& path, const wchar_t* mode)
{
    FILE* f = nullptr;
    _wfopen_s(&f, path.c_str(), mode);
    return f;
}
static int StatPath(const std::wstring& path, bool* isDir)
{
    struct _stat64 st;
    if (_wstat64(path.c_str(), &st) != 0) return 0;
    if (isDir) *isDir = ((st.st_mode & 0170000) == 0040000);
    return 1;
}
#define RMODE_RB L"rb"
#define RMODE_AB L"ab"
#define RMODE_WB L"wb"
#else
static FILE* OpenFileC(const std::wstring& path, const char* mode)
{
    return fopen(WToUtf8(path).c_str(), mode);
}
static int StatPath(const std::wstring& path, bool* isDir)
{
    struct stat st;
    if (stat(WToUtf8(path).c_str(), &st) != 0) return 0;
    if (isDir) *isDir = S_ISDIR(st.st_mode);
    return 1;
}
#define RMODE_RB "rb"
#define RMODE_AB "ab"
#define RMODE_WB "wb"
#endif

bool ReadBinFile(const std::wstring& path, std::string& out)
{
    out.clear();
    FILE* f = OpenFileC(path, RMODE_RB);
    if (!f) return false;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
    bool ok = ferror(f) == 0;
    fclose(f);
    return ok;
}
bool WriteBinFile(const std::wstring& path, const std::string& data, bool append)
{
    FILE* f = OpenFileC(path, append ? RMODE_AB : RMODE_WB);
    if (!f) return false;
    bool ok = true;
    if (!data.empty()) ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    fclose(f);
    return ok;
}
bool FileExistsW(const std::wstring& path)
{
    bool isDir = false;
    return StatPath(path, &isDir) && !isDir;
}
bool DirExistsW(const std::wstring& path)
{
    bool isDir = false;
    return StatPath(path, &isDir) && isDir;
}
bool CreateDirDeepW(const std::wstring& path)
{
    if (path.empty()) return false;
    if (DirExistsW(path)) return true;
    std::wstring cur;
    std::vector<std::wstring> parts = SplitW(path, L'\\');
    if (parts.size() == 1) parts = SplitW(path, L'/');
    for (size_t i = 0; i < parts.size(); i++) {
        if (parts[i].empty()) { if (i == 0) cur = parts[i]; continue; }
        if (cur.empty()) { cur = parts[i]; continue; }
#ifdef _WIN32
        cur += L"\\" + parts[i];
#else
        cur += L"/" + parts[i];
#endif
        if (!DirExistsW(cur)) {
#ifdef _WIN32
            CreateDirectoryW(cur.c_str(), nullptr);
#else
            mkdir(WToUtf8(cur).c_str(), 0755);
#endif
        }
    }
    return DirExistsW(path);
}
std::wstring ExeDirW()
{
#ifdef _WIN32
    wchar_t buf[MAX_PATH + 2] = { 0 };
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return DirFromPathW(buf);
#else
    return L".";
#endif
}
std::wstring NameFromPathW(const std::wstring& p)
{
    size_t a = p.find_last_of(L"\\/");
    return (a == std::wstring::npos) ? p : p.substr(a + 1);
}
std::wstring DirFromPathW(const std::wstring& p)
{
    size_t a = p.find_last_of(L"\\/");
    return (a == std::wstring::npos) ? std::wstring(L".") : p.substr(0, a);
}
long long FileMtimeUnix(const std::wstring& path)
{
#ifdef _WIN32
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return 0;
    FILETIME ft;
    BOOL ok = GetFileTime(h, nullptr, nullptr, &ft);
    CloseHandle(h);
    if (!ok) return 0;
    ULARGE_INTEGER u;
    u.HighPart = ft.dwHighDateTime;
    u.LowPart  = ft.dwLowDateTime;
    return (long long)((u.QuadPart - 116444736000000000ULL) / 10000000ULL);
#else
    (void)path; return 0;
#endif
}

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------
long long NowUnix()
{
    return (long long)time(nullptr);
}
static void LocalTm(long long unixSec, struct tm& t)
{
    time_t tt = (time_t)unixSec;
#ifdef _WIN32
    localtime_s(&t, &tt);
#else
    localtime_r(&tt, &t);
#endif
}
std::wstring TimeStr(long long unixSec)
{
    if (unixSec <= 0) return L"—";
    struct tm t; LocalTm(unixSec, t);
    return Fmt(L"%04d-%02d-%02d %02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min);
}
std::wstring RelTime(long long unixSec)
{
    if (unixSec <= 0) return L"—";
    long long d = NowUnix() - unixSec;
    if (d < 0) d = 0;
    if (d < 60)       return L"az önce";
    if (d < 3600)     return Fmt(L"%lld dk önce", d / 60);
    if (d < 86400)    return Fmt(L"%lld sa önce", d / 3600);
    if (d < 86400*30) return Fmt(L"%lld gün önce", d / 86400);
    return TimeStr(unixSec);
}

// ---------------------------------------------------------------------------
// DPI
// ---------------------------------------------------------------------------
UINT DpiForWnd(HWND h)
{
#ifdef _WIN32
    typedef UINT (WINAPI *Fn)(HWND);
    static Fn f = (Fn)(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (f && h && IsWindow(h)) {
        UINT d = f(h);
        if (d) return d;
    }
    HDC dc = GetDC(h);
    UINT dpi = (UINT)GetDeviceCaps(dc, LOGPIXELSX);
    if (h) ReleaseDC(h, dc);
    return dpi ? dpi : 96;
#else
    (void)h; return 96;
#endif
}
