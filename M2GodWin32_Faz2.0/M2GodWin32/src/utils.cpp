// utils.cpp — app.h yardımcılarının gerçeklemeleri + genel durum
#include "app.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

HINSTANCE g_hInst = nullptr;
HWND      g_hMain = nullptr;
HFONT     g_fontUI = nullptr;
HFONT     g_fontUIBold = nullptr;
HFONT     g_fontMono = nullptr;
std::wstring g_exeDir;
bool      g_modelDirty = false;

static HWND g_status = nullptr;
void SetStatusBarHwnd(HWND h) { g_status = h; }

// ---------------------------------------------------------------------------
std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}
std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}
std::wstring Lower(std::wstring s) { _wcslwr_s(&s[0], s.size() + 1); return s; }
std::wstring Trim(const std::wstring& s) {
    size_t a = s.find_first_not_of(L" \t\r\n");
    if (a == std::wstring::npos) return L"";
    size_t b = s.find_last_not_of(L" \t\r\n");
    return s.substr(a, b - a + 1);
}
std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep) {
    std::vector<std::wstring> out;
    std::wstring cur;
    for (wchar_t c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}
std::wstring Join(const std::vector<std::wstring>& v, const std::wstring& sep) {
    std::wstring out;
    for (size_t i = 0; i < v.size(); i++) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}
std::wstring ReplaceAll(std::wstring s, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::wstring::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

// ---------------------------------------------------------------------------
bool ReadFileText(const std::wstring& path, std::string& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz; GetFileSizeEx(h, &sz);
    out.resize((size_t)sz.QuadPart);
    DWORD rd = 0;
    BOOL ok = ReadFile(h, &out[0], (DWORD)sz.QuadPart, &rd, nullptr);
    CloseHandle(h);
    if (!ok) return false;
    out.resize(rd);
    // UTF-8 BOM kaldır
    if (out.size() >= 3 && (unsigned char)out[0] == 0xEF && (unsigned char)out[1] == 0xBB && (unsigned char)out[2] == 0xBF)
        out.erase(0, 3);
    return true;
}
bool WriteFileText(const std::wstring& path, const std::string& data) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD wr = 0;
    BOOL ok = WriteFile(h, data.data(), (DWORD)data.size(), &wr, nullptr);
    CloseHandle(h);
    return ok && wr == data.size();
}
bool FileExists(const std::wstring& path) { return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }
std::wstring GetFileDir(const std::wstring& path) {
    size_t p = path.find_last_of(L"\\/");
    if (p == std::wstring::npos) return L".";
    return path.substr(0, p);
}
std::wstring GetFileName(const std::wstring& path) {
    size_t p = path.find_last_of(L"\\/");
    return p == std::wstring::npos ? path : path.substr(p + 1);
}
std::wstring ExePath() {
    wchar_t buf[MAX_PATH]; GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return buf;
}

// ---------------------------------------------------------------------------
struct NamedColor { const wchar_t* name; COLORREF c; };
static const NamedColor kColors[] = {
    { L"Black", RGB(0,0,0) }, { L"White", RGB(255,255,255) }, { L"Red", RGB(255,0,0) },
    { L"Lime", RGB(0,255,0) }, { L"Green", RGB(0,128,0) }, { L"Blue", RGB(0,0,255) },
    { L"Yellow", RGB(255,255,0) }, { L"Cyan", RGB(0,255,255) }, { L"Magenta", RGB(255,0,255) },
    { L"Gray", RGB(128,128,128) }, { L"Silver", RGB(192,192,192) },
    { L"Orange", RGB(255,165,0) }, { L"Purple", RGB(128,0,128) }, { L"Maroon", RGB(128,0,0) },
    { L"Navy", RGB(0,0,128) }, { L"Teal", RGB(0,128,128) }, { L"Olive", RGB(128,128,0) },
    { L"Control", RGB(240,240,240) }, { L"Window", RGB(255,255,255) },
    { L"ButtonText", RGB(0,0,0) }, { L"Highlight", RGB(0,120,215) },
    { L"ControlText", RGB(0,0,0) }, { L"WindowText", RGB(0,0,0) },
    { L"ControlLight", RGB(227,227,227) }, { L"ControlLightLight", RGB(255,255,255) },
    { L"ControlDark", RGB(160,160,160) }, { L"ControlDarkDark", RGB(105,105,105) },
    { L"GrayText", RGB(109,109,109) }, { L"HotTrack", RGB(0,102,204) },
    { L"InactiveCaption", RGB(191,205,219) }, { L"ActiveCaption", RGB(153,180,209) },
    { L"Desktop", RGB(0,0,0) }, { L"AppWorkspace", RGB(171,171,171) },
    { L"Info", RGB(255,255,225) }, { L"Menu", RGB(240,240,240) },
    { L"ScrollBar", RGB(200,200,200) }, { L"Transparent", RGB(255,255,255) },
};
bool ParseColor(const std::wstring& s, COLORREF& out) {
    std::wstring t = Trim(s);
    if (t.empty()) return false;
    if (t[0] == L'#' && t.size() >= 7) {
        out = RGB((unsigned)wcstoul(t.substr(1, 2).c_str(), 0, 16),
                  (unsigned)wcstoul(t.substr(3, 2).c_str(), 0, 16),
                  (unsigned)wcstoul(t.substr(5, 2).c_str(), 0, 16));
        return true;
    }
    for (auto& nc : kColors) if (_wcsicmp(nc.name, t.c_str()) == 0) { out = nc.c; return true; }
    return false;
}
std::wstring ColorToHex(COLORREF c) {
    wchar_t buf[16];
    swprintf(buf, 16, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
    return buf;
}

HFONT MakeFont(const std::wstring& name, int pt, bool bold, bool italic, bool underline) {
    HDC dc = GetDC(nullptr);
    int h = -MulDiv(pt, GetDeviceCaps(dc, LOGPIXELSY), 72);
    ReleaseDC(nullptr, dc);
    return CreateFontW(h, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, italic, underline, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name.c_str());
}
bool ParseFontValue(const std::wstring& v, std::wstring& name, int& pt, bool& bold, bool& italic, bool& underline) {
    // Beklenen biçim: "Arial, 9pt, style=Bold, Italic" — esnek
    name = L"Segoe UI"; pt = 9; bold = italic = underline = false;
    auto parts = Split(v, L',');
    if (!parts.empty() && !Trim(parts[0]).empty()) name = Trim(parts[0]);
    for (size_t i = 1; i < parts.size(); i++) {
        std::wstring p = Trim(parts[i]);
        if (p.size() >= 3 && Lower(p).find(L"pt") != std::wstring::npos) {
            wchar_t* e = nullptr; double d = wcstod(p.c_str(), &e);
            if (e != p.c_str() && d > 0) pt = (int)d;
        }
        std::wstring lp = Lower(p);
        if (lp.find(L"bold") != std::wstring::npos) bold = true;
        if (lp.find(L"italic") != std::wstring::npos) italic = true;
        if (lp.find(L"underline") != std::wstring::npos) underline = true;
    }
    return true;
}

void Status(const std::wstring& text) {
    if (g_status) SendMessageW(g_status, SB_SETTEXTW, 0 | SBT_NOBORDERS, (LPARAM)text.c_str());
}
