R"M2GODGLOBAL(// ============================================================================
//  Global.h — M2GodWin32 UI Designer (Faz 2.5.2) TAM KAPSAMLI nesne kutuphanesi
//  M2GOD-GLOBAL v2502
//  !! BU DOSYAYI DUZENLEMEYIN — tasarimci gerektiginde gunceller.
//  Amac: Form1 olay kodlarinda WinForms benzeri sozdizimi (saf Win32 uzerinde):
//      timer1->Enabled = false;                  // Timer durdurur
//      button1->Text = L"Merhaba";
//      textBox1->ReadOnly = true;
//      comboBox1->Items->Add(L"Secenek");
//      checkedListBox1->SetItemChecked(0, true);
//      progressBar1->Value = 50;
//      button1->BackColor = M2God::Color::Red;
//      pictureBox1->Image = M2God::Image::FromFile(L"resim.png");  // PNG/JPG/GIF/BMP
//      comboBox1->DropDownStyle = M2God::ComboBoxStyle::DropDownList;
//      if (openFileDialog1->ShowDialog() == M2God::DialogResult::OK) ...
//  HWND gerekirse: button1->Handle   (ya da dogrudan SendMessage(button1, ...))
//  Derleme: VS2019 VARSAYILAN ayar (C++14) ile sorunsuz derlenir — /std:c++17
//  seciliyken de aynen calisir. Ek kutuphane gerekmez (header-only).
//  Not: proje ayariyla oynamaniza gerek yok — bu dosya C++14 ile uyumludur.
//  Kapsam: Config_info.json'daki tum toolbox kontrolleri, bilesenler ve
//          diyaloglar; her kontrol icin tasarimci panelindeki ozellikler.
// ============================================================================
#pragma once
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <shellapi.h>
#include <shlobj.h>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <initializer_list>
#include <type_traits>

#ifndef LVM_SETVIEW
#define LVM_SETVIEW (LVM_FIRST + 39)
#endif
#ifndef EM_SETBKCOLOR
#define EM_SETBKCOLOR (WM_USER + 11)
#endif

namespace M2God {

// ---------------------------------------------------------------------------
//  Dize birlestirme: L"Secim: " + comboBox1->Items->SelectedItem gibi
//  property nesneleri dogal sozdizimiyle birlestirilebilsin (ADL icin M2God'da)
// ---------------------------------------------------------------------------
template <typename T, typename = typename std::enable_if<std::is_class<T>::value && !std::is_same<T, std::wstring>::value && std::is_convertible<const T&, std::wstring>::value>::type>
inline std::wstring operator+(const wchar_t* a, const T& b) { return std::wstring(a) + (std::wstring)b; }
template <typename T, typename = typename std::enable_if<std::is_class<T>::value && !std::is_same<T, std::wstring>::value && std::is_convertible<const T&, std::wstring>::value>::type>
inline std::wstring operator+(const T& b, const wchar_t* a) { return (std::wstring)b + a; }
template <typename T, typename = typename std::enable_if<std::is_class<T>::value && !std::is_same<T, std::wstring>::value && std::is_convertible<const T&, std::wstring>::value>::type>
inline std::wstring operator+(const std::wstring& a, const T& b) { return a + (std::wstring)b; }

// ---------------------------------------------------------------------------
//  wcsncpy muadili — C4996 (güvensiz CRT) uyarisi ÜRETMEDEN ayni davranis:
//  kaynagi n karaktere kadar kopyala, kisa kaynakta kalan alan null doldur
// ---------------------------------------------------------------------------
inline void M2StrNCpy(wchar_t* dst, const wchar_t* src, size_t n) {
    if (!dst || n == 0) return;
    size_t i = 0;
    if (src) { for (; i < n && src[i]; ++i) dst[i] = src[i]; }
    for (; i < n; ++i) dst[i] = 0;
}

// ---------------------------------------------------------------------------
//  Color — M2God::Color::Red gibi sabitler + FromArgb
// ---------------------------------------------------------------------------
struct Color {
    static const COLORREF Red          = RGB(255, 0, 0);
    static const COLORREF Green        = RGB(0, 128, 0);
    static const COLORREF Blue         = RGB(0, 0, 255);
    static const COLORREF White        = RGB(255, 255, 255);
    static const COLORREF Black        = RGB(0, 0, 0);
    static const COLORREF Yellow       = RGB(255, 255, 0);
    static const COLORREF Orange       = RGB(255, 165, 0);
    static const COLORREF Gold         = RGB(255, 215, 0);
    static const COLORREF Purple       = RGB(128, 0, 128);
    static const COLORREF Magenta      = RGB(255, 0, 255);
    static const COLORREF Pink         = RGB(255, 192, 203);
    static const COLORREF Brown        = RGB(165, 42, 42);
    static const COLORREF Gray         = RGB(128, 128, 128);
    static const COLORREF LightGray    = RGB(211, 211, 211);
    static const COLORREF DarkGray     = RGB(64, 64, 64);
    static const COLORREF Silver       = RGB(192, 192, 192);
    static const COLORREF Cyan         = RGB(0, 255, 255);
    static const COLORREF LightBlue    = RGB(173, 216, 230);
    static const COLORREF LightGreen   = RGB(144, 238, 144);
    static const COLORREF Navy         = RGB(0, 0, 128);
    static const COLORREF Teal         = RGB(0, 128, 128);
    static const COLORREF Lime         = RGB(0, 255, 0);
    static const COLORREF Maroon       = RGB(128, 0, 0);
    static const COLORREF Olive        = RGB(128, 128, 0);
    static const COLORREF SkyBlue      = RGB(135, 206, 235);
    static const COLORREF Tomato       = RGB(255, 99, 71);
    static const COLORREF Khaki        = RGB(240, 230, 140);
    static const COLORREF Beige        = RGB(245, 245, 220);
    static const COLORREF Ivory        = RGB(255, 255, 240);
    static const COLORREF Salmon       = RGB(250, 128, 114);
    static const COLORREF Transparent  = 0xFFFFFFFE;   // ozel: cizimde atlanir
    static const COLORREF Control      = RGB(240, 240, 240);
    static const COLORREF Window       = RGB(255, 255, 255);
    static const COLORREF ControlText  = RGB(0, 0, 0);
    static const COLORREF WindowText   = RGB(0, 0, 0);
    static const COLORREF Highlight    = RGB(0, 120, 215);
    static COLORREF FromArgb(int r, int g, int b) { return RGB(r, g, b); }
    static COLORREF FromArgb(int a, int r, int g, int b) { (void)a; return RGB(r, g, b); }
    static int AlphaOf(COLORREF c) { (void)c; return 255; }
    static int ROf(COLORREF c) { return (int)GetRValue(c); }
    static int GOf(COLORREF c) { return (int)GetGValue(c); }
    static int BOf(COLORREF c) { return (int)GetBValue(c); }
};

// ---------------------------------------------------------------------------
//  Point / Size / Rectangle — Location/Size property'leri icin
// ---------------------------------------------------------------------------
struct Point {
    int X = 0, Y = 0;
    Point() {}
    Point(int x, int y) : X(x), Y(y) {}
};
struct Size {
    int Width = 0, Height = 0;
    Size() {}
    Size(int w, int h) : Width(w), Height(h) {}
};
struct Rectangle {
    int X = 0, Y = 0, Width = 0, Height = 0;
    Rectangle() {}
    Rectangle(int x, int y, int w, int h) : X(x), Y(y), Width(w), Height(h) {}
};
struct M2DateTime {
    int Year = 2024, Month = 1, Day = 1, Hour = 0, Minute = 0, Second = 0;
    M2DateTime() {}
    M2DateTime(int y, int m, int d, int hh = 0, int mm = 0, int ss = 0)
        : Year(y), Month(m), Day(d), Hour(hh), Minute(mm), Second(ss) {}
    static M2DateTime FromSystemTime(const SYSTEMTIME& st) {
        return M2DateTime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    }
    void ToSystemTime(SYSTEMTIME& st) const {
        ZeroMemory(&st, sizeof(st));
        st.wYear = (WORD)Year; st.wMonth = (WORD)Month; st.wDay = (WORD)Day;
        st.wHour = (WORD)Hour; st.wMinute = (WORD)Minute; st.wSecond = (WORD)Second;
    }
    std::wstring ToString() const {
        wchar_t b[64]; swprintf(b, 64, L"%02d.%02d.%04d %02d:%02d:%02d", Day, Month, Year, Hour, Minute, Second);
        return b;
    }
};

// ---------------------------------------------------------------------------
//  Enumlar — WinForms degerleri korunur; uygulama kodu Win32'ye cevirir
// ---------------------------------------------------------------------------
enum class HorizontalAlignment   { Left = 0, Center = 1, Right = 2 };
enum class BorderStyle           { None = 0, FixedSingle = 1, Fixed3D = 2 };
enum class FlatStyle             { Standard = 0, Popup = 1, Flat = 2, System = 3 };
enum class ComboBoxStyle         { Simple = 1, DropDown = 2, DropDownList = 3 };
enum class CheckState            { Unchecked = 0, Checked = 1, Indeterminate = 2 };
enum class View                  { LargeIcon = 0, Details = 1, SmallIcon = 2, List = 3, Tile = 4 };
enum class SortOrder             { None = 0, Ascending = 1, Descending = 2 };
enum class PictureBoxSizeMode    { Normal = 0, StretchImage = 1, AutoSize = 2, CenterImage = 3, Zoom = 4 };
enum class ProgressBarStyle      { Blocks = 0, Continuous = 1, Marquee = 2 };
enum class DateTimePickerFormat  { Long = 1, Short = 2, Time = 3, Custom = 4 };
enum class TickStyle             { None = 0, TopLeft = 1, BottomRight = 2, Both = 3 };
enum class Orientation           { Horizontal = 0, Vertical = 1 };
enum class SelectionMode         { None = 0, One = 1, MultiSimple = 2, MultiExtended = 3 };
enum class CharacterCasing       { Normal = 0, Upper = 1, Lower = 2 };
enum class ScrollBars            { None = 0, Horizontal = 1, Vertical = 2, Both = 3 };
enum class HeaderStyle           { Clickable = 0, NonClickable = 1, None = 2 };
enum class ContentAlignment      { TopLeft = 1, TopCenter = 2, TopRight = 4,
                                   MiddleLeft = 16, MiddleCenter = 32, MiddleRight = 64,
                                   BottomLeft = 256, BottomCenter = 512, BottomRight = 1024 };
enum class MessageBoxButtons     { OK = 0, OKCancel = 1, AbortRetryIgnore = 2, YesNoCancel = 3, YesNo = 4, RetryCancel = 5 };
enum class MessageBoxIcon        { None = 0, Error = 16, Question = 32, Warning = 48, Information = 64, Hand = 16, Stop = 16, Exclamation = 48, Asterisk = 64 };
enum class DialogResult          { None = 0, OK = 1, Cancel = 2, Abort = 3, Retry = 4, Ignore = 5, Yes = 6, No = 7 };
enum class FormBorderStyle       { None = 0, FixedSingle = 1, Fixed3D = 2, FixedDialog = 3, Sizable = 4, FixedToolWindow = 5, SizableToolWindow = 6 };
enum class FormStartPosition     { Manual = 0, CenterScreen = 1, WindowsDefaultLocation = 2, WindowsDefaultBounds = 3, CenterParent = 4 };
enum class FormWindowState       { Normal = 0, Minimized = 1, Maximized = 2 };
enum class RichTextBoxStreamType { RichText = 0, PlainText = 1, UnicodePlainText = 2, LineText = 4 };
enum class Keys                  { None = 0, Back = 8, Tab = 9, Enter = 13, Shift = 16, Control = 17, Menu = 18,
                                   Escape = 27, Space = 32, PageUp = 33, PageDown = 34, End = 35, Home = 36,
                                   Left = 37, Up = 38, Right = 39, Down = 40, Insert = 45, Delete = 46,
                                   D0 = 48, D1 = 49, D2 = 50, D3 = 51, D4 = 52, D5 = 53, D6 = 54, D7 = 55, D8 = 56, D9 = 57,
                                   A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72, I = 73, J = 74,
                                   K = 75, L = 76, M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84,
                                   U = 85, V = 86, W = 87, X = 88, Y = 89, Z = 90,
                                   NumPad0 = 96, NumPad1 = 97, NumPad2 = 98, NumPad3 = 99, NumPad4 = 100,
                                   NumPad5 = 101, NumPad6 = 102, NumPad7 = 103, NumPad8 = 104, NumPad9 = 105,
                                   F1 = 112, F2 = 113, F3 = 114, F4 = 115, F5 = 116, F6 = 117, F7 = 118, F8 = 119,
                                   F9 = 120, F10 = 121, F11 = 122, F12 = 123 };

// ---------------------------------------------------------------------------
//  UI fontu
// ---------------------------------------------------------------------------
inline HFONT GetUIFont() {
    static HFONT f = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    return f;
}
// M2God::MakeFont(L"Tahoma", 10, true)
inline HFONT MakeFont(const wchar_t* name, int pt, bool bold = false,
                      bool italic = false, bool underline = false, bool strikeout = false) {
    HDC dc = GetDC(nullptr);
    int hgt = -MulDiv(pt > 0 ? pt : 9, GetDeviceCaps(dc, LOGPIXELSY), 72);
    ReleaseDC(nullptr, dc);
    return CreateFontW(hgt, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
        italic ? TRUE : FALSE, underline ? TRUE : FALSE, strikeout ? TRUE : FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, name);
}
// M2God::FontName/FontSize/FontBold/FontItalic/FontUnderline — bir fonttan oku
inline std::wstring FontName(HFONT f) {
    if (!f) return L"Segoe UI";
    LOGFONTW lf; ZeroMemory(&lf, sizeof(lf));
    GetObjectW(f, sizeof(lf), &lf);
    return lf.lfFaceName;
}
inline int FontSize(HFONT f) {
    if (!f) return 9;
    LOGFONTW lf; ZeroMemory(&lf, sizeof(lf));
    GetObjectW(f, sizeof(lf), &lf);
    HDC dc = GetDC(nullptr);
    int pt = MulDiv(-lf.lfHeight, 72, GetDeviceCaps(dc, LOGPIXELSY));
    ReleaseDC(nullptr, dc);
    return pt;
}

// ---------------------------------------------------------------------------
//  Renk kayit defteri — BackColor/ForeColor WM_CTLCOLOR* ile uygulanir
// ---------------------------------------------------------------------------
struct ColorInfo { COLORREF back = 0xFFFFFFFF; COLORREF fore = 0xFFFFFFFF; };
inline ColorInfo& CtrlColorInfo(HWND h) {
    static std::map<HWND, ColorInfo> tbl;
    return tbl[h];
}
// Form1_WndProc icindeki WM_CTLCOLOR* case'lerinden cagrilir
inline LRESULT HandleCtlColor(HWND ctrl, HDC dc) {
    ColorInfo ci = CtrlColorInfo(ctrl);
    bool hasBack = (ci.back != 0xFFFFFFFF), hasFore = (ci.fore != 0xFFFFFFFF);
    if (!hasBack && !hasFore) return 0;
    if (hasFore) SetTextColor(dc, ci.fore);
    if (hasBack) {
        SetBkColor(dc, ci.back);
        static std::map<HWND, HBRUSH> brushes;
        auto it = brushes.find(ctrl);
        if (it == brushes.end()) {
            HBRUSH b = CreateSolidBrush(ci.back);
            brushes[ctrl] = b;
            return (LRESULT)(INT_PTR)b;
        }
        return (LRESULT)(INT_PTR)it->second;
    }
    SetBkMode(dc, TRANSPARENT);
    return (LRESULT)(INT_PTR)GetStockObject(HOLLOW_BRUSH);
}

// ---------------------------------------------------------------------------
//  Panel sinifi (M2GOD_PANEL_GEN) — kapsayici icin ozel pencere
// ---------------------------------------------------------------------------
inline LRESULT CALLBACK M2PanelProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_ERASEBKGND) {
        HDC dc = (HDC)w; RECT rc; GetClientRect(h, &rc);
        ColorInfo ci = CtrlColorInfo(h);
        COLORREF c = (ci.back != 0xFFFFFFFF) ? ci.back : RGB(255, 255, 255);
        HBRUSH br = CreateSolidBrush(c); FillRect(dc, &rc, br); DeleteObject(br);
        return 1;
    }
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        ColorInfo ci = CtrlColorInfo(h);
        COLORREF c = (ci.back != 0xFFFFFFFF) ? ci.back : RGB(255, 255, 255);
        HBRUSH br = CreateSolidBrush(c); FillRect(dc, &rc, br); DeleteObject(br);
        wchar_t buf[256]; int n = GetWindowTextW(h, buf, 256);
        if (n > 0) {
            SetBkMode(dc, TRANSPARENT);
            if (ci.fore != 0xFFFFFFFF) SetTextColor(dc, ci.fore);
            SelectObject(dc, GetUIFont());
            RECT tr = rc; tr.left += 4; tr.top += 2;
            DrawTextW(dc, buf, n, &tr, DT_LEFT | DT_TOP);
        }
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
inline void EnsurePanelClass() {
    static bool done = false; if (done) return; done = true;
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = M2PanelProc;
    wc.hInstance   = GetModuleHandleW(nullptr);
    wc.hCursor     = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"M2GOD_PANEL_GEN";
    RegisterClassW(&wc);
}
inline void EnsureRichEdit() {
    static bool done = false; if (done) return; done = true;
    LoadLibraryW(L"Riched20.dll");
}

// ---------------------------------------------------------------------------
//  Yardimci: pencere stili degistir + cizgeyi tazele
// ---------------------------------------------------------------------------
inline void RestyleWindow(HWND h, LONG_PTR addStyle, LONG_PTR removeStyle,
                          LONG_PTR addEx = 0, LONG_PTR removeEx = 0) {
    if (!h) return;
    LONG_PTR st = GetWindowLongPtrW(h, GWL_STYLE);
    st = (st | addStyle) & ~removeStyle;
    SetWindowLongPtrW(h, GWL_STYLE, st);
    LONG_PTR ex = GetWindowLongPtrW(h, GWL_EXSTYLE);
    ex = (ex | addEx) & ~removeEx;
    SetWindowLongPtrW(h, GWL_EXSTYLE, ex);
    SetWindowPos(h, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    InvalidateRect(h, nullptr, TRUE);
}

// ============================================================================
//  Control — tum gorsel kontrollerin temeli (property vekilleri)
// ============================================================================
class Control {
public:
    HWND Handle = NULL;
    int  Id     = 0;
    std::wstring Tag;        // serbest metin (tasarimcidan gelmez; kullanicinin)
    std::wstring Name;       // kontrol adi
    int TabIndexStored = 0;  // Win32'de siralama yaratma sirasidir; bilgi amacli

    Control() {}
    Control(HWND h, int id) : Handle(h), Id(id) {}
    virtual ~Control() {}

    operator HWND() const { return Handle; }   // SendMessage(button1, ...) gibi

    // ---- Text ----
    class TextProp {
        Control* c;
    public:
        explicit TextProp(Control* o) : c(o) {}
        TextProp& operator=(const std::wstring& s) { if (c->Handle) SetWindowTextW(c->Handle, s.c_str()); return *this; }
        TextProp& operator=(const wchar_t* s) { return *this = std::wstring(s ? s : L""); }
        operator std::wstring() const {
            if (!c->Handle) return L"";
            int n = GetWindowTextLengthW(c->Handle);
            if (n <= 0) return L"";
            std::vector<wchar_t> buf((size_t)n + 2, 0);
            GetWindowTextW(c->Handle, buf.data(), n + 1);
            return std::wstring(buf.data());
        }
        bool Empty() const { return ((std::wstring)*this).empty(); }
    } Text{ this };

    // ---- Enabled ----
    class EnabledProp {
        Control* c;
    public:
        explicit EnabledProp(Control* o) : c(o) {}
        EnabledProp& operator=(bool v) { if (c->Handle) EnableWindow(c->Handle, v ? TRUE : FALSE); return *this; }
        operator bool() const { return c->Handle && IsWindowEnabled(c->Handle) != 0; }
    } Enabled{ this };

    // ---- Visible ----
    class VisibleProp {
        Control* c;
    public:
        explicit VisibleProp(Control* o) : c(o) {}
        VisibleProp& operator=(bool v) { if (c->Handle) ShowWindow(c->Handle, v ? SW_SHOW : SW_HIDE); return *this; }
        operator bool() const { return c->Handle && IsWindowVisible(c->Handle) != 0; }
    } Visible{ this };

    // ---- Font (HFONT atanir; M2God::MakeFont ile uretilebilir) ----
    class FontProp {
        Control* c;
    public:
        explicit FontProp(Control* o) : c(o) {}
        FontProp& operator=(HFONT f) { if (c->Handle) SendMessageW(c->Handle, WM_SETFONT, (WPARAM)f, MAKELPARAM(TRUE, 0)); return *this; }
        operator HFONT() const { return c->Handle ? (HFONT)SendMessageW(c->Handle, WM_GETFONT, 0, 0) : nullptr; }
    } Font{ this };

    // ---- BackColor / ForeColor ----
    class BackColorProp {
        Control* c;
    public:
        explicit BackColorProp(Control* o) : c(o) {}
        BackColorProp& operator=(COLORREF v) {
            CtrlColorInfo(c->Handle).back = v;
            if (c->Handle) { c->OnBackColorChanged(); InvalidateRect(c->Handle, nullptr, TRUE); }
            return *this;
        }
        operator COLORREF() const { return CtrlColorInfo(c->Handle).back; }
    } BackColor{ this };
    class ForeColorProp {
        Control* c;
    public:
        explicit ForeColorProp(Control* o) : c(o) {}
        ForeColorProp& operator=(COLORREF v) {
            CtrlColorInfo(c->Handle).fore = v;
            if (c->Handle) { c->OnForeColorChanged(); InvalidateRect(c->Handle, nullptr, TRUE); }
            return *this;
        }
        operator COLORREF() const { return CtrlColorInfo(c->Handle).fore; }
    } ForeColor{ this };

    // ---- Location (Point) ----
    class LocationProp {
        Control* c;
    public:
        explicit LocationProp(Control* o) : c(o) {}
        LocationProp& operator=(const Point& p) {
            if (c->Handle) SetWindowPos(c->Handle, nullptr, p.X, p.Y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            return *this;
        }
        LocationProp& operator=(const Size& s) = delete;   // karisiklik onlemi
        operator Point() const {
            if (!c->Handle) return Point();
            RECT rc; GetWindowRect(c->Handle, &rc);
            HWND p = GetParent(c->Handle);
            POINT pt = { rc.left, rc.top };
            if (p) MapWindowPoints(nullptr, p, &pt, 1);
            return Point(pt.x, pt.y);
        }
        LocationProp& operator=(const wchar_t*) = delete;
    } Location{ this };

    // ---- Size ----
    class SizeProp {
        Control* c;
    public:
        explicit SizeProp(Control* o) : c(o) {}
        SizeProp& operator=(const Size& s) {
            if (c->Handle) SetWindowPos(c->Handle, nullptr, 0, 0, s.Width, s.Height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return *this;
        }
        operator Size() const {
            if (!c->Handle) return M2God::Size();
            RECT rc; GetClientRect(c->Handle, &rc);
            return M2God::Size(rc.right - rc.left, rc.bottom - rc.top);
        }
    } Size{ this };

    // ---- Width / Height / Left / Top / Right / Bottom ----
    class WidthProp {
        Control* c;
    public:
        explicit WidthProp(Control* o) : c(o) {}
        WidthProp& operator=(int v) { if (c->Handle) { RECT r; GetWindowRect(c->Handle, &r); SetWindowPos(c->Handle, nullptr, 0, 0, v, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); } return *this; }
        operator int() const { if (!c->Handle) return 0; RECT r; GetWindowRect(c->Handle, &r); return r.right - r.left; }
    } Width{ this };
    class HeightProp {
        Control* c;
    public:
        explicit HeightProp(Control* o) : c(o) {}
        HeightProp& operator=(int v) { if (c->Handle) { RECT r; GetWindowRect(c->Handle, &r); SetWindowPos(c->Handle, nullptr, 0, 0, r.right - r.left, v, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); } return *this; }
        operator int() const { if (!c->Handle) return 0; RECT r; GetWindowRect(c->Handle, &r); return r.bottom - r.top; }
    } Height{ this };
    class LeftProp {
        Control* c;
    public:
        explicit LeftProp(Control* o) : c(o) {}
        LeftProp& operator=(int v) { Point p = (Point)c->Location; c->Location = Point(v, p.Y); return *this; }
        operator int() const { return ((Point)c->Location).X; }
    } Left{ this };
    class TopProp {
        Control* c;
    public:
        explicit TopProp(Control* o) : c(o) {}
        TopProp& operator=(int v) { Point p = (Point)c->Location; c->Location = Point(p.X, v); return *this; }
        operator int() const { return ((Point)c->Location).Y; }
    } Top{ this };
    class RightProp {
        Control* c;
    public:
        explicit RightProp(Control* o) : c(o) {}
        RightProp& operator=(int v) { Point p = (Point)c->Location; c->Location = Point(v - (int)c->Width, p.Y); return *this; }
        operator int() const { Point p = (Point)c->Location; return p.X + (int)c->Width; }
    } Right{ this };
    class BottomProp {
        Control* c;
    public:
        explicit BottomProp(Control* o) : c(o) {}
        BottomProp& operator=(int v) { Point p = (Point)c->Location; c->Location = Point(p.X, v - (int)c->Height); return *this; }
        operator int() const { Point p = (Point)c->Location; return p.Y + (int)c->Height; }
    } Bottom{ this };

    // ---- TabStop (WS_TABSTOP stili) ----
    class TabStopProp {
        Control* c;
    public:
        explicit TabStopProp(Control* o) : c(o) {}
        TabStopProp& operator=(bool v) {
            if (c->Handle) RestyleWindow(c->Handle, v ? WS_TABSTOP : 0, v ? 0 : WS_TABSTOP);
            return *this;
        }
        operator bool() const {
            return c->Handle && (GetWindowLongPtrW(c->Handle, GWL_STYLE) & WS_TABSTOP) != 0;
        }
    } TabStop{ this };

    // ---- sanal renk olaylari (RichTextBox override eder) ----
    virtual void OnBackColorChanged() {}
    virtual void OnForeColorChanged() {}

    // ---- metotlar ----
    void SetLocation(int x, int y) { Location = Point(x, y); }
    void SetSize(int w, int h)     { Size = M2God::Size(w, h); }
    void SetBounds(int x, int y, int w, int h) {
        if (Handle) SetWindowPos(Handle, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    void Focus()                   { if (Handle) SetFocus(Handle); }
    void Hide() { Visible = false; }
    void Show() { Visible = true; }
    void Refresh()  { if (Handle) { InvalidateRect(Handle, nullptr, TRUE); UpdateWindow(Handle); } }
    void Invalidate() { if (Handle) InvalidateRect(Handle, nullptr, TRUE); }
    void Update()   { if (Handle) UpdateWindow(Handle); }
    void BringToFront() { if (Handle) SetWindowPos(Handle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); }
    void SendToBack()   { if (Handle) SetWindowPos(Handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); }
    void Dispose()      { if (Handle) { DestroyWindow(Handle); Handle = nullptr; } }
    HWND Parent() const { return Handle ? GetParent(Handle) : nullptr; }
    Point PointToScreen(const Point& p) const {
        POINT pt = { p.X, p.Y };
        if (Handle) MapWindowPoints(Handle, nullptr, &pt, 1);
        return Point(pt.x, pt.y);
    }
};

// InitDone(): uretilen kod InitializeControls sonunda true yapar.
// Init bitene kadar WndProc bildirim yonlendirmesi atlanir (zincirleme
// bildirimlerin yarim nesnelere carpmasini onler).
inline bool& InitDone() { static bool d = false; return d; }

// ============================================================================
//  Form — statik form API'si (M2God::Form::Handle uretilen koda atanir)
//  C++14 uyumlu: "inline static" yerine SINIF-SABLONU tekniği kullaniliyor
//  (VS2019 varsayilan /std:c++14 ayarinda derlenir; /std:c++17'de de aynen
//   calisir). Sozdizimi degismedi: M2God::Form::Handle, M2God::Form::Text ...
// ============================================================================
template<typename D>
struct FormBase {
    static HWND Handle;                    // wWinMain/WM_CREATE icinde atanir

    struct TextP {
        void operator=(const std::wstring& s) { if (Handle) SetWindowTextW(Handle, s.c_str()); }
        void operator=(const wchar_t* s) { *this = std::wstring(s ? s : L""); }
        operator std::wstring() const {
            if (!Handle) return L"";
            int n = GetWindowTextLengthW(Handle);
            std::vector<wchar_t> buf((size_t)n + 2, 0);
            GetWindowTextW(Handle, buf.data(), n + 1);
            return std::wstring(buf.data());
        }
    };
    static TextP Text;
    struct ClientSizeP {
        void operator=(const Size& s) {
            if (!Handle) return;
            RECT rc = { 0, 0, s.Width, s.Height };
            LONG_PTR st = GetWindowLongPtrW(Handle, GWL_STYLE);
            AdjustWindowRect(&rc, (DWORD)st, FALSE);
            SetWindowPos(Handle, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        operator Size() const {
            if (!Handle) return Size();
            RECT rc; GetClientRect(Handle, &rc);
            return Size(rc.right - rc.left, rc.bottom - rc.top);
        }
    };
    static ClientSizeP ClientSize;
    struct BackColorP {
        void operator=(COLORREF c) {
            if (!Handle) return;
            HBRUSH br = CreateSolidBrush(c);
            HBRUSH old = (HBRUSH)SetClassLongPtrW(Handle, GCLP_HBRBACKGROUND, (LONG_PTR)br);
            if (old) DeleteObject(old);
            InvalidateRect(Handle, nullptr, TRUE);
        }
        operator COLORREF() const { return CtrlColorInfo(Handle).back; }
    };
    static BackColorP BackColor;
    struct ForeColorP {
        void operator=(COLORREF c) { CtrlColorInfo(Handle).fore = c; if (Handle) InvalidateRect(Handle, nullptr, TRUE); }
        operator COLORREF() const { return CtrlColorInfo(Handle).fore; }
    };
    static ForeColorP ForeColor;
    struct TopMostP {
        void operator=(bool v) {
            if (Handle) SetWindowPos(Handle, v ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        operator bool() const {
            return Handle && (GetWindowLongPtrW(Handle, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
        }
    };
    static TopMostP TopMost;
    struct OpacityP {   // 0-100 (100 = tam opak)
        void operator=(int pct) {
            if (!Handle) return;
            LONG_PTR ex = GetWindowLongPtrW(Handle, GWL_EXSTYLE);
            if (pct >= 100) { SetWindowLongPtrW(Handle, GWL_EXSTYLE, ex & ~WS_EX_LAYERED); return; }
            SetWindowLongPtrW(Handle, GWL_EXSTYLE, ex | WS_EX_LAYERED);
            SetLayeredWindowAttributes(Handle, 0, (BYTE)(pct < 0 ? 0 : pct > 100 ? 255 : pct * 255 / 100), LWA_ALPHA);
        }
    };
    static OpacityP Opacity;
    struct WindowStateP {
        void operator=(FormWindowState ws) {
            if (!Handle) return;
            ShowWindow(Handle, ws == FormWindowState::Maximized ? SW_MAXIMIZE :
                              ws == FormWindowState::Minimized ? SW_MINIMIZE : SW_RESTORE);
        }
        operator FormWindowState() const {
            if (!Handle) return FormWindowState::Normal;
            if (IsIconic(Handle)) return FormWindowState::Minimized;
            if (IsZoomed(Handle)) return FormWindowState::Maximized;
            return FormWindowState::Normal;
        }
    };
    static WindowStateP WindowState;
    struct MaximizeBoxP {
        void operator=(bool v) { if (Handle) RestyleWindow(Handle, v ? WS_MAXIMIZEBOX : 0, v ? 0 : WS_MAXIMIZEBOX); }
        operator bool() const { return Handle && (GetWindowLongPtrW(Handle, GWL_STYLE) & WS_MAXIMIZEBOX) != 0; }
    };
    static MaximizeBoxP MaximizeBox;
    struct MinimizeBoxP {
        void operator=(bool v) { if (Handle) RestyleWindow(Handle, v ? WS_MINIMIZEBOX : 0, v ? 0 : WS_MINIMIZEBOX); }
        operator bool() const { return Handle && (GetWindowLongPtrW(Handle, GWL_STYLE) & WS_MINIMIZEBOX) != 0; }
    };
    static MinimizeBoxP MinimizeBox;
    struct ControlBoxP {
        void operator=(bool v) { if (Handle) RestyleWindow(Handle, v ? WS_SYSMENU : 0, v ? 0 : WS_SYSMENU); }
    };
    static ControlBoxP ControlBox;
    struct FormBorderStyleP {
        void operator=(M2God::FormBorderStyle b) {
            if (!Handle) return;
            LONG_PTR add = 0, rem = 0;
            switch (b) {
            case M2God::FormBorderStyle::None:            rem = WS_OVERLAPPEDWINDOW; add = WS_POPUP; break;
            case M2God::FormBorderStyle::FixedSingle:     rem = WS_THICKFRAME | WS_MAXIMIZEBOX; break;
            case M2God::FormBorderStyle::Fixed3D:
            case M2God::FormBorderStyle::FixedDialog:     rem = WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX; add = WS_CAPTION | WS_SYSMENU; break;
            case M2God::FormBorderStyle::FixedToolWindow: rem = WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU; break;
            case M2God::FormBorderStyle::SizableToolWindow: rem = WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU; break;
            default: break;   // Sizable — zaten WS_OVERLAPPEDWINDOW
            }
            RestyleWindow(Handle, add, rem);
        }
    };
    static FormBorderStyleP FormBorderStyle;

    static void Close()    { if (Handle) PostMessageW(Handle, WM_CLOSE, 0, 0); }
    static void Activate() { if (Handle) { SetForegroundWindow(Handle); SetActiveWindow(Handle); } }
    static void CenterToScreen() {
        if (!Handle) return;
        RECT rw, rd;
        GetWindowRect(Handle, &rw);
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rd, 0);
        SetWindowPos(Handle, nullptr,
            rd.left + ((rd.right - rd.left) - (rw.right - rw.left)) / 2,
            rd.top + ((rd.bottom - rd.top) - (rw.bottom - rw.top)) / 2,
            0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
};

// Statik üye tanımlari — sinif sablonu üyesi olduklari icin header'da
// tanimlanabilirler (C++14 uyumlu; ODR guvenli: tek varlik)
template<typename D> HWND FormBase<D>::Handle = nullptr;
template<typename D> typename FormBase<D>::TextP FormBase<D>::Text{};
template<typename D> typename FormBase<D>::ClientSizeP FormBase<D>::ClientSize{};
template<typename D> typename FormBase<D>::BackColorP FormBase<D>::BackColor{};
template<typename D> typename FormBase<D>::ForeColorP FormBase<D>::ForeColor{};
template<typename D> typename FormBase<D>::TopMostP FormBase<D>::TopMost{};
template<typename D> typename FormBase<D>::OpacityP FormBase<D>::Opacity{};
template<typename D> typename FormBase<D>::WindowStateP FormBase<D>::WindowState{};
template<typename D> typename FormBase<D>::MaximizeBoxP FormBase<D>::MaximizeBox{};
template<typename D> typename FormBase<D>::MinimizeBoxP FormBase<D>::MinimizeBox{};
template<typename D> typename FormBase<D>::ControlBoxP FormBase<D>::ControlBox{};
template<typename D> typename FormBase<D>::FormBorderStyleP FormBase<D>::FormBorderStyle{};

// Form — sablon parametresi sabitlenmis takma ad (sozdizimi: M2God::Form::X)
using Form = FormBase<void>;

// ============================================================================
//  Button
// ============================================================================
class Button : public Control {
public:
    Button(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"BUTTON", text.c_str(),
            WS_TABSTOP | BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    void PerformClick() { if (Handle) SendMessageW(Handle, BM_CLICK, 0, 0); }
    // TextAlign: ContentAlignment -> BS_* stilleri
    class TextAlignProp {
        Control* c;
    public:
        explicit TextAlignProp(Control* o) : c(o) {}
        TextAlignProp& operator=(ContentAlignment a) {
            if (!c->Handle) return *this;
            LONG_PTR add = 0, rem = BS_LEFT | BS_RIGHT | BS_CENTER | BS_TOP | BS_BOTTOM | BS_VCENTER;
            switch (a) {
            case ContentAlignment::TopLeft:      add = BS_LEFT | BS_TOP; break;
            case ContentAlignment::TopCenter:    add = BS_CENTER | BS_TOP; break;
            case ContentAlignment::TopRight:     add = BS_RIGHT | BS_TOP; break;
            case ContentAlignment::MiddleLeft:   add = BS_LEFT | BS_VCENTER; break;
            case ContentAlignment::MiddleCenter: add = BS_CENTER | BS_VCENTER; break;
            case ContentAlignment::MiddleRight:  add = BS_RIGHT | BS_VCENTER; break;
            case ContentAlignment::BottomLeft:   add = BS_LEFT | BS_BOTTOM; break;
            case ContentAlignment::BottomCenter: add = BS_CENTER | BS_BOTTOM; break;
            case ContentAlignment::BottomRight:  add = BS_RIGHT | BS_BOTTOM; break;
            }
            RestyleWindow(c->Handle, add, rem);
            return *this;
        }
    } TextAlign{ this };
    // FlatStyle: Flat -> BS_FLAT
    class FlatStyleProp {
        Control* c;
    public:
        explicit FlatStyleProp(Control* o) : c(o) {}
        FlatStyleProp& operator=(FlatStyle f) {
            if (c->Handle) RestyleWindow(c->Handle, f == FlatStyle::Flat ? BS_FLAT : 0, f == FlatStyle::Flat ? 0 : BS_FLAT);
            return *this;
        }
    } FlatStyle{ this };
};

// ============================================================================
//  CheckBox / RadioButton
// ============================================================================
class CheckBox : public Control {
public:
    CheckBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"BUTTON", text.c_str(),
            WS_TABSTOP | BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    // checkBox1->Checked = true;
    class CheckedProp {
        Control* c;
    public:
        explicit CheckedProp(Control* o) : c(o) {}
        CheckedProp& operator=(bool v) { if (c->Handle) SendMessageW(c->Handle, BM_SETCHECK, v ? BST_CHECKED : BST_UNCHECKED, 0); return *this; }
        operator bool() const { return c->Handle && SendMessageW(c->Handle, BM_GETCHECK, 0, 0) == BST_CHECKED; }
    } Checked{ this };
    class CheckStateProp {
        Control* c;
    public:
        explicit CheckStateProp(Control* o) : c(o) {}
        CheckStateProp& operator=(CheckState st) {
            if (c->Handle) SendMessageW(c->Handle, BM_SETCHECK,
                st == CheckState::Checked ? BST_CHECKED : st == CheckState::Indeterminate ? BST_INDETERMINATE : BST_UNCHECKED, 0);
            return *this;
        }
        operator CheckState() const {
            if (!c->Handle) return CheckState::Unchecked;
            LRESULT r = SendMessageW(c->Handle, BM_GETCHECK, 0, 0);
            return r == BST_CHECKED ? CheckState::Checked : r == BST_INDETERMINATE ? CheckState::Indeterminate : CheckState::Unchecked;
        }
    } CheckState{ this };
    // ThreeState: BS_AUTO3STATE stili gerektirir (yaratma sirasinda); bilgi amacli saklanir
    void SetThreeState(bool v) { TagStored3S = v ? 1 : 0; }
    bool GetThreeState() const { return TagStored3S != 0; }
    int  TagStored3S = 0;
};

class RadioButton : public CheckBox {
public:
    RadioButton(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h)
        : CheckBox(parent, id, text, x, y, w, h) {
        if (Handle) {
            // stili radio'ya cevir (CreateWindow sonrasi stil degisimi guvenli)
            LONG_PTR st = GetWindowLongPtrW(Handle, GWL_STYLE);
            SetWindowLongPtrW(Handle, GWL_STYLE, (st & ~BS_AUTOCHECKBOX) | BS_AUTORADIOBUTTON);
        }
    }
};

// ============================================================================
//  GroupBox / Label / LinkLabel
// ============================================================================
class GroupBox : public Control {
public:
    GroupBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"BUTTON", text.c_str(),
            BS_GROUPBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
};

class Label : public Control {
public:
    Label(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"STATIC", text.c_str(),
            SS_LEFT | SS_NOTIFY | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    // Win32 STATIC otomatik buyumez: AutoSize atamasi bilgi amaclidir.
    // Istenen davranis icin Size = M2God::Size(genislik, yukseklik) kullanin.
    class AutoSizeProp {
        Control* c;
    public:
        explicit AutoSizeProp(Control* o) : c(o) {}
        AutoSizeProp& operator=(bool v) { (void)v; return *this; }
        operator bool() const { return false; }
    } AutoSize{ this };
    class BorderStyleProp {
        Control* c;
    public:
        explicit BorderStyleProp(Control* o) : c(o) {}
        BorderStyleProp& operator=(M2God::BorderStyle b) {
            if (!c->Handle) return *this;
            switch (b) {
            case M2God::BorderStyle::None:        RestyleWindow(c->Handle, 0, WS_BORDER, 0, WS_EX_CLIENTEDGE); break;
            case M2God::BorderStyle::FixedSingle: RestyleWindow(c->Handle, WS_BORDER, 0, 0, WS_EX_CLIENTEDGE); break;
            case M2God::BorderStyle::Fixed3D:     RestyleWindow(c->Handle, 0, WS_BORDER, WS_EX_CLIENTEDGE, 0); break;
            }
            return *this;
        }
    } BorderStyle{ this };
    class TextAlignProp {
        Control* c;
    public:
        explicit TextAlignProp(Control* o) : c(o) {}
        TextAlignProp& operator=(ContentAlignment a) {
            if (!c->Handle) return *this;
            LONG_PTR add, rem = SS_TYPEMASK;
            switch (a) {
            case ContentAlignment::MiddleCenter:
            case ContentAlignment::BottomCenter:
            case ContentAlignment::TopCenter:    add = SS_CENTER; break;
            case ContentAlignment::MiddleRight:
            case ContentAlignment::TopRight:
            case ContentAlignment::BottomRight:  add = SS_RIGHT; break;
            default:                             add = SS_LEFT; break;
            }
            RestyleWindow(c->Handle, add, rem);
            return *this;
        }
    } TextAlign{ this };
    class AutoEllipsisProp {
        Control* c;
    public:
        explicit AutoEllipsisProp(Control* o) : c(o) {}
        AutoEllipsisProp& operator=(bool v) {
            if (c->Handle) RestyleWindow(c->Handle, v ? SS_ENDELLIPSIS : 0, v ? 0 : SS_ENDELLIPSIS);
            return *this;
        }
    } AutoEllipsis{ this };
};

class LinkLabel : public Control {
public:
    LinkLabel(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"STATIC", text.c_str(),
            SS_LEFT | SS_NOTIFY | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) {
            SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
            CtrlColorInfo(Handle).fore = RGB(0, 0, 255);   // link mavisi
        }
    }
    bool LinkVisited = false;   // bilgi amacli (Ziyaret edildi rengi icin)
};

// ============================================================================
//  TextBox (MaskedTextBox da bu siniftir)
// ============================================================================
class TextBox : public Control {
public:
    TextBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h, bool multiline = false) {
        DWORD st = WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;
        if (multiline) st |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
        else st |= ES_AUTOHSCROLL;
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text.c_str(), st, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    // textBox1->ReadOnly = true;
    class ReadOnlyProp {
        Control* c;
    public:
        explicit ReadOnlyProp(Control* o) : c(o) {}
        ReadOnlyProp& operator=(bool v) { if (c->Handle) SendMessageW(c->Handle, EM_SETREADONLY, v ? TRUE : FALSE, 0); return *this; }
        operator bool() const { return c->Handle && (GetWindowLongW(c->Handle, GWL_STYLE) & ES_READONLY) != 0; }
    } ReadOnly{ this };
    // textBox1->PasswordChar = L'*';
    class PasswordCharProp {
        Control* c;
    public:
        explicit PasswordCharProp(Control* o) : c(o) {}
        PasswordCharProp& operator=(wchar_t ch) {
            if (c->Handle) { SendMessageW(c->Handle, EM_SETPASSWORDCHAR, (WPARAM)ch, 0); InvalidateRect(c->Handle, nullptr, TRUE); }
            return *this;
        }
        operator wchar_t() const {
            return c->Handle ? (wchar_t)SendMessageW(c->Handle, EM_GETPASSWORDCHAR, 0, 0) : (wchar_t)0;
        }
    } PasswordChar{ this };
    class MaxLengthProp {
        Control* c;
    public:
        explicit MaxLengthProp(Control* o) : c(o) {}
        MaxLengthProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, EM_SETLIMITTEXT, (WPARAM)v, 0); return *this; }
        operator int() const { return c->Handle ? (int)SendMessageW(c->Handle, EM_GETLIMITTEXT, 0, 0) : 0; }
    } MaxLength{ this };
    class TextAlignProp {
        Control* c;
    public:
        explicit TextAlignProp(Control* o) : c(o) {}
        TextAlignProp& operator=(HorizontalAlignment a) {
            if (!c->Handle) return *this;
            LONG_PTR add, rem = ES_LEFT | ES_CENTER | ES_RIGHT;
            switch (a) {
            case HorizontalAlignment::Center: add = ES_CENTER; break;
            case HorizontalAlignment::Right:  add = ES_RIGHT; break;
            default:                          add = ES_LEFT; break;
            }
            RestyleWindow(c->Handle, add, rem);
            return *this;
        }
    } TextAlign{ this };
    class CharacterCasingProp {
        Control* c;
    public:
        explicit CharacterCasingProp(Control* o) : c(o) {}
        CharacterCasingProp& operator=(CharacterCasing v) {
            if (!c->Handle) return *this;
            LONG_PTR add, rem = ES_UPPERCASE | ES_LOWERCASE;
            switch (v) {
            case CharacterCasing::Upper: add = ES_UPPERCASE; break;
            case CharacterCasing::Lower: add = ES_LOWERCASE; break;
            default:                     add = 0; break;
            }
            RestyleWindow(c->Handle, add, rem);
            return *this;
        }
    } CharacterCasing{ this };
    class UseSystemPasswordCharProp {
        Control* c;
    public:
        explicit UseSystemPasswordCharProp(Control* o) : c(o) {}
        UseSystemPasswordCharProp& operator=(bool v) {
            if (c->Handle) { SendMessageW(c->Handle, EM_SETPASSWORDCHAR, v ? (WPARAM)L'\u25CF' : 0, 0); InvalidateRect(c->Handle, nullptr, TRUE); }
            return *this;
        }
    } UseSystemPasswordChar{ this };
    // Multiline: yaratma sirasinda belirlenir (ES_MULTILINE); sonradan bilgi amacli
    class MultiLineProp {
        Control* c;
    public:
        explicit MultiLineProp(Control* o) : c(o) {}
        MultiLineProp& operator=(bool v) { (void)v; return *this; }
        operator bool() const {
            return c->Handle && (GetWindowLongW(c->Handle, GWL_STYLE) & ES_MULTILINE) != 0;
        }
    } MultiLine{ this };
    // WordWrap: multiline EDIT'te yatay kaydirma stilidir; bilgi amacli
    class WordWrapProp {
        Control* c;
    public:
        explicit WordWrapProp(Control* o) : c(o) {}
        WordWrapProp& operator=(bool v) {
            if (c->Handle && (GetWindowLongW(c->Handle, GWL_STYLE) & ES_MULTILINE))
                RestyleWindow(c->Handle, v ? 0 : (LONG_PTR)WS_HSCROLL, v ? (LONG_PTR)WS_HSCROLL : 0);
            return *this;
        }
    } WordWrap{ this };

    // ---- metotlar ----
    void AppendText(const std::wstring& s) {
        if (!Handle) return;
        int len = GetWindowTextLengthW(Handle);
        SendMessageW(Handle, EM_SETSEL, (WPARAM)len, (LPARAM)len);
        SendMessageW(Handle, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
    }
    void Clear()     { if (Handle) SetWindowTextW(Handle, L""); }
    void Copy()      { if (Handle) SendMessageW(Handle, WM_COPY, 0, 0); }
    void Cut()       { if (Handle) SendMessageW(Handle, WM_CUT, 0, 0); }
    void Paste()     { if (Handle) SendMessageW(Handle, WM_PASTE, 0, 0); }
    void Undo()      { if (Handle) SendMessageW(Handle, EM_UNDO, 0, 0); }
    void SelectAll() { if (Handle) SendMessageW(Handle, EM_SETSEL, 0, -1); }
    void ScrollToCaret() { if (Handle) SendMessageW(Handle, EM_SCROLLCARET, 0, 0); }
    int  GetLineCount()  { return Handle ? (int)SendMessageW(Handle, EM_GETLINECOUNT, 0, 0) : 0; }
};

// MaskedTextBox — Win32 EDIT ile uygulanir; Mask/PromptChar bilgi amacli saklanir
class MaskedTextBox : public TextBox {
public:
    std::wstring Mask;        // or: L"00-00" (dogrulama kullaniciya aittir)
    wchar_t PromptChar = L'_';
    std::wstring TextMaskFormat = L"IncludeLiterals";
    MaskedTextBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h, bool = false)
        : TextBox(parent, id, text, x, y, w, h, false) {}
};

// ============================================================================
//  NumericUpDown — EDIT + msctls_updown32 (gercek ok butonlari)
// ============================================================================
class NumericUpDown : public Control {
public:
    HWND Buddy = NULL;      // EDIT
    HWND UpDown = NULL;     // msctls_updown32
    int  IncrementValue = 1;
    int  MinValue = 0, MaxValue = 100;
    int  DecimalPlacesStored = 0;
    bool HexadecimalStored = false;

    NumericUpDown(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h, bool = false) {
        Buddy = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text.c_str(),
            WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Handle = Buddy;
        Id = id;
        if (Buddy) SendMessageW(Buddy, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
        UpDown = CreateWindowExW(0, L"msctls_updown32", L"",
            WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY | UDS_HOTTRACK,
            0, 0, 18, h, parent, (HMENU)(INT_PTR)(id + 50000), GetModuleHandleW(nullptr), nullptr);
        if (UpDown) {
            SendMessageW(UpDown, UDM_SETRANGE32, 0, 100);
            SendMessageW(UpDown, UDM_SETPOS32, 0, _wtoi(text.c_str()));
        }
    }
    // numericUpDown1->Value = 5;
    class ValueProp {
        NumericUpDown* c;
    public:
        explicit ValueProp(NumericUpDown* o) : c(o) {}
        ValueProp& operator=(int v) {
            if (v < c->MinValue) v = c->MinValue;
            if (v > c->MaxValue) v = c->MaxValue;
            if (c->UpDown) SendMessageW(c->UpDown, UDM_SETPOS32, 0, (LPARAM)v);
            else if (c->Buddy) { wchar_t b[32]; swprintf(b, 32, L"%d", v); SetWindowTextW(c->Buddy, b); }
            return *this;
        }
        operator int() const {
            if (c->UpDown) return (int)SendMessageW(c->UpDown, UDM_GETPOS32, 0, 0);
            wchar_t b[32] = L""; if (c->Buddy) GetWindowTextW(c->Buddy, b, 32);
            return _wtoi(b);
        }
    } Value{ this };
    class MinimumProp {
        NumericUpDown* c;
    public:
        explicit MinimumProp(NumericUpDown* o) : c(o) {}
        MinimumProp& operator=(int v) { c->MinValue = v; if (c->UpDown) { int mx = c->MaxValue; SendMessageW(c->UpDown, UDM_SETRANGE32, (WPARAM)v, (LPARAM)mx); } return *this; }
        operator int() const { return c->MinValue; }
    } Minimum{ this };
    class MaximumProp {
        NumericUpDown* c;
    public:
        explicit MaximumProp(NumericUpDown* o) : c(o) {}
        MaximumProp& operator=(int v) { c->MaxValue = v; if (c->UpDown) { int mn = c->MinValue; SendMessageW(c->UpDown, UDM_SETRANGE32, (WPARAM)mn, (LPARAM)v); } return *this; }
        operator int() const { return c->MaxValue; }
    } Maximum{ this };
    class IncrementProp {
        NumericUpDown* c;
    public:
        explicit IncrementProp(NumericUpDown* o) : c(o) {}
        IncrementProp& operator=(int v) { c->IncrementValue = v > 0 ? v : 1; return *this; }
        operator int() const { return c->IncrementValue; }
    } Increment{ this };
    class ReadOnlyProp {
        Control* c;
    public:
        explicit ReadOnlyProp(Control* o) : c(o) {}
        ReadOnlyProp& operator=(bool v) { if (c->Handle) SendMessageW(c->Handle, EM_SETREADONLY, v ? TRUE : FALSE, 0); return *this; }
        operator bool() const { return c->Handle && (GetWindowLongW(c->Handle, GWL_STYLE) & ES_READONLY) != 0; }
    } ReadOnly{ this };
    class HexadecimalProp {
        NumericUpDown* c;
    public:
        explicit HexadecimalProp(NumericUpDown* o) : c(o) {}
        HexadecimalProp& operator=(bool v) { c->HexadecimalStored = v; return *this; }
        operator bool() const { return c->HexadecimalStored; }
    } Hexadecimal{ this };
    void UpButton()   { int v = (int)Value + c()->IncrementValue; Value = v; }
    void DownButton() { int v = (int)Value - c()->IncrementValue; Value = v; }
private:
    NumericUpDown* c() { return this; }
};

// ============================================================================
//  RichTextBox — RichEdit20W (RTF + metotlar)
// ============================================================================
class RichTextBox : public Control {
public:
    RichTextBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        EnsureRichEdit();
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"RichEdit20W", text.c_str(),
            WS_BORDER | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
            x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    class ReadOnlyProp {
        Control* c;
    public:
        explicit ReadOnlyProp(Control* o) : c(o) {}
        ReadOnlyProp& operator=(bool v) { if (c->Handle) SendMessageW(c->Handle, EM_SETREADONLY, v ? TRUE : FALSE, 0); return *this; }
        operator bool() const { return c->Handle && (GetWindowLongW(c->Handle, GWL_STYLE) & ES_READONLY) != 0; }
    } ReadOnly{ this };
    class MaxLengthProp {
        Control* c;
    public:
        explicit MaxLengthProp(Control* o) : c(o) {}
        MaxLengthProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, EM_EXLIMITTEXT, 0, (LPARAM)v); return *this; }
    } MaxLength{ this };
    // Secili metnin rengi
    class SelectionColorProp {
        RichTextBox* c;
    public:
        explicit SelectionColorProp(RichTextBox* o) : c(o) {}
        SelectionColorProp& operator=(COLORREF cr) {
            if (!c->Handle) return *this;
            CHARFORMATW cf; ZeroMemory(&cf, sizeof(cf));
            cf.cbSize = sizeof(cf);
            cf.dwMask = CFM_COLOR;
            cf.crTextColor = cr;
            SendMessageW(c->Handle, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
            return *this;
        }
    } SelectionColor{ this };
    // Secili metin
    class SelectedTextProp {
        RichTextBox* c;
    public:
        explicit SelectedTextProp(RichTextBox* o) : c(o) {}
        SelectedTextProp& operator=(const std::wstring& s) {
            if (c->Handle) SendMessageW(c->Handle, EM_REPLACESEL, TRUE, (LPARAM)s.c_str());
            return *this;
        }
        operator std::wstring() const {
            if (!c->Handle) return L"";
            wchar_t buf[1024] = L"";
            SendMessageW(c->Handle, EM_GETSELTEXT, 0, (LPARAM)buf);
            return std::wstring(buf);
        }
    } SelectedText{ this };
    // BackColor — RichEdit WM_CTLCOLOR gondermez; EM_SETBKCOLOR kullanir
    virtual void OnBackColorChanged() override {
        if (Handle) SendMessageW(Handle, EM_SETBKCOLOR, 0, (LPARAM)CtrlColorInfo(Handle).back);
    }
    virtual void OnForeColorChanged() override {
        if (!Handle) return;
        CHARFORMATW cf; ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = CtrlColorInfo(Handle).fore;
        SendMessageW(Handle, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }

    void AppendText(const std::wstring& s) {
        if (!Handle) return;
        CHARRANGE cr; cr.cpMin = cr.cpMax = -1;
        SendMessageW(Handle, EM_EXSETSEL, 0, (LPARAM)&cr);
        SendMessageW(Handle, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
    }
    void Clear()     { if (Handle) SetWindowTextW(Handle, L""); }
    void Copy()      { if (Handle) SendMessageW(Handle, WM_COPY, 0, 0); }
    void Cut()       { if (Handle) SendMessageW(Handle, WM_CUT, 0, 0); }
    void Paste()     { if (Handle) SendMessageW(Handle, WM_PASTE, 0, 0); }
    void Undo()      { if (Handle) SendMessageW(Handle, EM_UNDO, 0, 0); }
    void Redo()      { if (Handle) SendMessageW(Handle, EM_REDO, 0, 0); }
    void SelectAll() { if (Handle) { CHARRANGE cr; cr.cpMin = 0; cr.cpMax = -1; SendMessageW(Handle, EM_EXSETSEL, 0, (LPARAM)&cr); } }
    // Dosya kaydet/yukle — RTF (EM_STREAM) veya duz metin
    static DWORD CALLBACK M2OutStreamCb(DWORD_PTR cookie, LPBYTE pb, LONG cb, LONG* pcb) {
        HANDLE hf = (HANDLE)(INT_PTR)cookie;
        DWORD wr = 0;
        if (cb > 0 && hf && hf != INVALID_HANDLE_VALUE) WriteFile(hf, pb, (DWORD)cb, &wr, nullptr);
        if (pcb) *pcb = cb;
        return 0;
    }
    struct InCtx { std::vector<char> data; size_t pos = 0; };
    static DWORD CALLBACK M2InStreamCb(DWORD_PTR cookie, LPBYTE pb, LONG cb, LONG* pcb) {
        InCtx* ctx = (InCtx*)cookie;
        LONG n = (LONG)(ctx->data.size() - ctx->pos);
        if (n > cb) n = cb;
        if (n > 0) { memcpy(pb, ctx->data.data() + ctx->pos, (size_t)n); ctx->pos += (size_t)n; }
        if (pcb) *pcb = n;
        return 0;
    }
    void SaveFile(const std::wstring& path, RichTextBoxStreamType type = RichTextBoxStreamType::RichText) {
        if (!Handle) return;
        HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hf == INVALID_HANDLE_VALUE) return;
        bool rtf = (type == RichTextBoxStreamType::RichText);
        if (rtf) {
            EDITSTREAM es; ZeroMemory(&es, sizeof(es));
            es.dwCookie = (DWORD_PTR)(INT_PTR)hf;
            es.pfnCallback = &M2OutStreamCb;
            SendMessageW(Handle, EM_STREAMOUT, (WPARAM)SF_RTF, (LPARAM)&es);
        } else {
            int n = GetWindowTextLengthW(Handle);
            std::vector<wchar_t> buf((size_t)n + 2, 0);
            GetWindowTextW(Handle, buf.data(), n + 1);
            DWORD wr = 0;
            WriteFile(hf, "\xFF\xFE", 2, &wr, nullptr);                       // UTF-16 BOM
            WriteFile(hf, buf.data(), (DWORD)(n * sizeof(wchar_t)), &wr, nullptr);
        }
        CloseHandle(hf);
    }
    void LoadFile(const std::wstring& path, RichTextBoxStreamType type = RichTextBoxStreamType::RichText) {
        if (!Handle) return;
        HANDLE hf = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hf == INVALID_HANDLE_VALUE) return;
        bool rtf = (type == RichTextBoxStreamType::RichText);
        DWORD sz = GetFileSize(hf, nullptr), rd = 0;
        std::vector<char> data((size_t)sz + 1, 0);
        ReadFile(hf, data.data(), sz, &rd, nullptr);
        CloseHandle(hf);
        if (rtf) {
            InCtx ctx; ctx.data.swap(data);
            EDITSTREAM es; ZeroMemory(&es, sizeof(es));
            es.dwCookie = (DWORD_PTR)&ctx;
            es.pfnCallback = &M2InStreamCb;
            SendMessageW(Handle, EM_STREAMIN, (WPARAM)SF_RTF, (LPARAM)&es);
        } else {
            // UTF-16 BOM varsa dogrudan; degilse UTF-8/ANSI cevir
            std::wstring txt;
            if (sz >= 2 && data[0] == (char)0xFF && data[1] == (char)0xFE)
                txt.assign((const wchar_t*)(data.data() + 2), (size_t)((sz - 2) / 2));
            else {
                int wl = MultiByteToWideChar(CP_UTF8, 0, data.data(), (int)sz, nullptr, 0);
                txt.resize((size_t)(wl > 0 ? wl : 0));
                if (wl > 0) MultiByteToWideChar(CP_UTF8, 0, data.data(), (int)sz, &txt[0], wl);
            }
            SetWindowTextW(Handle, txt.c_str());
        }
    }
};

// ============================================================================
//  ComboBox
// ============================================================================
class ComboBox : public Control {
public:
    ComboBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"COMBOBOX", text.c_str(),
            WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    // comboBox1->Items->Add(L"...");  Items->SelectedIndex = 0;  Items->Count
    class ItemsProp {
        ComboBox* c;
    public:
        explicit ItemsProp(ComboBox* o) : c(o) {}
        ItemsProp* operator->() { return this; }
        void Clear() { if (c->Handle) SendMessageW(c->Handle, CB_RESETCONTENT, 0, 0); }
        void Add(const std::wstring& s) { if (c->Handle) SendMessageW(c->Handle, CB_ADDSTRING, 0, (LPARAM)s.c_str()); }
        void AddRange(std::initializer_list<const wchar_t*> ls) { for (auto* s : ls) Add(s); }
        void AddRange(const std::vector<std::wstring>& ls) { for (auto& s : ls) Add(s); }
        void Insert(int i, const std::wstring& s) { if (c->Handle) SendMessageW(c->Handle, CB_INSERTSTRING, (WPARAM)i, (LPARAM)s.c_str()); }
        void RemoveAt(int i) { if (c->Handle) SendMessageW(c->Handle, CB_DELETESTRING, (WPARAM)i, 0); }
        int  GetCount() const { return c->Handle ? (int)SendMessageW(c->Handle, CB_GETCOUNT, 0, 0) : 0; }
        bool Contains(const std::wstring& s) const { return IndexOf(s) >= 0; }
        int  IndexOf(const std::wstring& s) const {
            return c->Handle ? (int)SendMessageW(c->Handle, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)s.c_str()) : -1;
        }
        std::wstring GetItem(int i) const {
            if (!c->Handle || i < 0 || i >= GetCount()) return L"";
            wchar_t buf[256] = L"";
            SendMessageW(c->Handle, CB_GETLBTEXT, (WPARAM)i, (LPARAM)buf);
            return buf;
        }
        int  GetSelectedIndex() const { return c->Handle ? (int)SendMessageW(c->Handle, CB_GETCURSEL, 0, 0) : -1; }
        void SetSelectedIndex(int i) { if (c->Handle) SendMessageW(c->Handle, CB_SETCURSEL, (WPARAM)i, 0); }
        std::wstring GetSelectedItem() const { return GetItem(GetSelectedIndex()); }
        void BeginUpdate() { if (c->Handle) SendMessageW(c->Handle, WM_SETREDRAW, FALSE, 0); }
        void EndUpdate()   { if (c->Handle) { SendMessageW(c->Handle, WM_SETREDRAW, TRUE, 0); InvalidateRect(c->Handle, nullptr, TRUE); } }
        class SelIdxProp {
            ItemsProp* p;
        public:
            explicit SelIdxProp(ItemsProp* o) : p(o) {}
            SelIdxProp& operator=(int i) { p->SetSelectedIndex(i); return *this; }
            operator int() const { return p->GetSelectedIndex(); }
        } SelectedIndex{ this };
        class SelItemProp {
            ItemsProp* p;
        public:
            explicit SelItemProp(ItemsProp* o) : p(o) {}
            operator std::wstring() const { return p->GetSelectedItem(); }
        } SelectedItem{ this };
        class CountProp {
            ItemsProp* p;
        public:
            explicit CountProp(ItemsProp* o) : p(o) {}
            operator int() const { return p->GetCount(); }
        } Count{ this };
    } Items{ this };
    // comboBox1->DropDownStyle = M2God::ComboBoxStyle::DropDownList;
    class DropDownStyleProp {
        Control* c;
    public:
        explicit DropDownStyleProp(Control* o) : c(o) {}
        DropDownStyleProp& operator=(M2God::ComboBoxStyle v) {
            if (!c->Handle) return *this;
            LONG_PTR add, rem = CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST;
            switch (v) {
            case M2God::ComboBoxStyle::Simple:         add = CBS_SIMPLE; break;
            case M2God::ComboBoxStyle::DropDown:       add = CBS_DROPDOWN; break;
            default:                                   add = CBS_DROPDOWNLIST; break;
            }
            RestyleWindow(c->Handle, add, rem);
            return *this;
        }
    } DropDownStyle{ this };
    class DroppedDownProp {
        Control* c;
    public:
        explicit DroppedDownProp(Control* o) : c(o) {}
        DroppedDownProp& operator=(bool v) { if (c->Handle) SendMessageW(c->Handle, CB_SHOWDROPDOWN, v ? TRUE : FALSE, 0); return *this; }
        operator bool() const { return c->Handle && SendMessageW(c->Handle, CB_GETDROPPEDSTATE, 0, 0) != 0; }
    } DroppedDown{ this };
    class MaxLengthProp {
        Control* c;
    public:
        explicit MaxLengthProp(Control* o) : c(o) {}
        MaxLengthProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, CB_LIMITTEXT, (WPARAM)v, 0); return *this; }
    } MaxLength{ this };
    bool Sorted = false;            // yaratma sirasinda etkili (LBS_SORT/CBS_SORT)
    void BeginUpdate() { if (Handle) SendMessageW(Handle, WM_SETREDRAW, FALSE, 0); }
    void EndUpdate()   { if (Handle) { SendMessageW(Handle, WM_SETREDRAW, TRUE, 0); InvalidateRect(Handle, nullptr, TRUE); } }
};

// ============================================================================
//  ListBox
// ============================================================================
class ListBox : public Control {
public:
    ListBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"LISTBOX", text.c_str(),
            WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    class ItemsProp {
        ListBox* c;
    public:
        explicit ItemsProp(ListBox* o) : c(o) {}
        ItemsProp* operator->() { return this; }
        void Clear() { if (c->Handle) SendMessageW(c->Handle, LB_RESETCONTENT, 0, 0); }
        void Add(const std::wstring& s) { if (c->Handle) SendMessageW(c->Handle, LB_ADDSTRING, 0, (LPARAM)s.c_str()); }
        void AddRange(std::initializer_list<const wchar_t*> ls) { for (auto* s : ls) Add(s); }
        void AddRange(const std::vector<std::wstring>& ls) { for (auto& s : ls) Add(s); }
        void Insert(int i, const std::wstring& s) { if (c->Handle) SendMessageW(c->Handle, LB_INSERTSTRING, (WPARAM)i, (LPARAM)s.c_str()); }
        void RemoveAt(int i) { if (c->Handle) SendMessageW(c->Handle, LB_DELETESTRING, (WPARAM)i, 0); }
        int  GetCount() const { return c->Handle ? (int)SendMessageW(c->Handle, LB_GETCOUNT, 0, 0) : 0; }
        bool Contains(const std::wstring& s) const { return IndexOf(s) >= 0; }
        int  IndexOf(const std::wstring& s) const {
            return c->Handle ? (int)SendMessageW(c->Handle, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)s.c_str()) : -1;
        }
        std::wstring GetItem(int i) const {
            if (!c->Handle || i < 0 || i >= GetCount()) return L"";
            wchar_t buf[256] = L"";
            SendMessageW(c->Handle, LB_GETTEXT, (WPARAM)i, (LPARAM)buf);
            return buf;
        }
        int  GetSelectedIndex() const { return c->Handle ? (int)SendMessageW(c->Handle, LB_GETCURSEL, 0, 0) : -1; }
        void SetSelectedIndex(int i) { if (c->Handle) SendMessageW(c->Handle, LB_SETCURSEL, (WPARAM)i, 0); }
        std::wstring GetSelectedItem() const { return GetItem(GetSelectedIndex()); }
        void BeginUpdate() { if (c->Handle) SendMessageW(c->Handle, WM_SETREDRAW, FALSE, 0); }
        void EndUpdate()   { if (c->Handle) { SendMessageW(c->Handle, WM_SETREDRAW, TRUE, 0); InvalidateRect(c->Handle, nullptr, TRUE); } }
        class SelIdxProp {
            ItemsProp* p;
        public:
            explicit SelIdxProp(ItemsProp* o) : p(o) {}
            SelIdxProp& operator=(int i) { p->SetSelectedIndex(i); return *this; }
            operator int() const { return p->GetSelectedIndex(); }
        } SelectedIndex{ this };
        class SelItemProp {
            ItemsProp* p;
        public:
            explicit SelItemProp(ItemsProp* o) : p(o) {}
            operator std::wstring() const { return p->GetSelectedItem(); }
        } SelectedItem{ this };
        class CountProp {
            ItemsProp* p;
        public:
            explicit CountProp(ItemsProp* o) : p(o) {}
            operator int() const { return p->GetCount(); }
        } Count{ this };
    } Items{ this };
    // Coklu secim (SelectionMode::MultiSimple/MultiExtended icin yaratma stili gereklidir)
    class SelectionModeProp {
        Control* c;
    public:
        explicit SelectionModeProp(Control* o) : c(o) {}
        SelectionModeProp& operator=(M2God::SelectionMode v) {
            if (c->Handle)
                RestyleWindow(c->Handle,
                    (v == M2God::SelectionMode::MultiSimple || v == M2God::SelectionMode::MultiExtended) ? LBS_MULTIPLESEL | LBS_EXTENDEDSEL : 0,
                    (v == M2God::SelectionMode::MultiSimple || v == M2God::SelectionMode::MultiExtended) ? 0 : (LBS_MULTIPLESEL | LBS_EXTENDEDSEL));
            return *this;
        }
    } SelectionMode{ this };
    class TopIndexProp {
        Control* c;
    public:
        explicit TopIndexProp(Control* o) : c(o) {}
        TopIndexProp& operator=(int i) { if (c->Handle) SendMessageW(c->Handle, LB_SETTOPINDEX, (WPARAM)i, 0); return *this; }
        operator int() const { return c->Handle ? (int)SendMessageW(c->Handle, LB_GETTOPINDEX, 0, 0) : 0; }
    } TopIndex{ this };
    class ItemHeightProp {
        Control* c;
    public:
        explicit ItemHeightProp(Control* o) : c(o) {}
        ItemHeightProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, LB_SETITEMHEIGHT, 0, MAKELPARAM(v, 0)); return *this; }
        operator int() const { if (!c->Handle) return 0; LONG r = SendMessageW(c->Handle, LB_GETITEMHEIGHT, 0, 0); return (int)r; }
    } ItemHeight{ this };
    bool Sorted = false;
    void ClearSelected() { if (Handle) SendMessageW(Handle, LB_SETSEL, FALSE, (LPARAM)-1); }
    bool GetSelected(int i) const { return Handle && SendMessageW(Handle, LB_GETSEL, (WPARAM)i, 0) > 0; }
    void SetSelected(int i, bool v) { if (Handle) SendMessageW(Handle, LB_SETSEL, v ? TRUE : FALSE, (LPARAM)i); }
};

// ============================================================================
//  CheckedListBox — LVS_LIST + LVS_EX_CHECKBOXES ListView ile uygulanir
// ============================================================================
class CheckedListBox : public Control {
public:
    CheckedListBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysListView32", text.c_str(),
            LVS_LIST | LVS_SHOWSELALWAYS | WS_BORDER | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) {
            SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
            ListView_SetExtendedListViewStyle(Handle, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
        }
    }
    class ItemsProp {
        CheckedListBox* c;
    public:
        explicit ItemsProp(CheckedListBox* o) : c(o) {}
        ItemsProp* operator->() { return this; }
        void Clear() { if (c->Handle) SendMessageW(c->Handle, LVM_DELETEALLITEMS, 0, 0); }
        void Add(const std::wstring& s) {
            if (!c->Handle) return;
            LVITEMW li = { 0 };
            li.mask = LVIF_TEXT;
            li.iItem = (int)SendMessageW(c->Handle, LVM_GETITEMCOUNT, 0, 0);
            li.pszText = (LPWSTR)s.c_str();
            SendMessageW(c->Handle, LVM_INSERTITEMW, 0, (LPARAM)&li);
        }
        void AddRange(std::initializer_list<const wchar_t*> ls) { for (auto* s : ls) Add(s); }
        void AddRange(const std::vector<std::wstring>& ls) { for (auto& s : ls) Add(s); }
        void Insert(int i, const std::wstring& s) {
            if (!c->Handle) return;
            LVITEMW li = { 0 };
            li.mask = LVIF_TEXT;
            li.iItem = i;
            li.pszText = (LPWSTR)s.c_str();
            SendMessageW(c->Handle, LVM_INSERTITEMW, 0, (LPARAM)&li);
        }
        void RemoveAt(int i) { if (c->Handle) SendMessageW(c->Handle, LVM_DELETEITEM, (WPARAM)i, 0); }
        int  GetCount() const { return c->Handle ? (int)SendMessageW(c->Handle, LVM_GETITEMCOUNT, 0, 0) : 0; }
        std::wstring GetItem(int i) const {
            if (!c->Handle || i < 0 || i >= GetCount()) return L"";
            wchar_t buf[256] = L"";
            LVITEMW li = { 0 };
            li.mask = LVIF_TEXT;
            li.iItem = i;
            li.pszText = buf;
            li.cchTextMax = 256;
            SendMessageW(c->Handle, LVM_GETITEMTEXTW, (WPARAM)i, (LPARAM)&li);
            return buf;
        }
        int  GetSelectedIndex() const { return c->Handle ? (int)SendMessageW(c->Handle, LVM_GETNEXTITEM, (WPARAM)-1, MAKELPARAM(LVNI_SELECTED, 0)) : -1; }
        std::wstring GetSelectedItem() const { return GetItem(GetSelectedIndex()); }
        class SelIdxProp {
            ItemsProp* p;
        public:
            explicit SelIdxProp(ItemsProp* o) : p(o) {}
            SelIdxProp& operator=(int i) {
                if (p->c->Handle) {
                    LVITEMW li = { 0 };
                    li.mask = LVIF_STATE;
                    li.state = LVIS_SELECTED;
                    li.stateMask = LVIS_SELECTED;
                    SendMessageW(p->c->Handle, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&li);
                    if (i >= 0) SendMessageW(p->c->Handle, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&li);
                }
                return *this;
            }
            operator int() const { return p->GetSelectedIndex(); }
        } SelectedIndex{ this };
        class SelItemProp {
            ItemsProp* p;
        public:
            explicit SelItemProp(ItemsProp* o) : p(o) {}
            operator std::wstring() const { return p->GetSelectedItem(); }
        } SelectedItem{ this };
        class CountProp {
            ItemsProp* p;
        public:
            explicit CountProp(ItemsProp* o) : p(o) {}
            operator int() const { return p->GetCount(); }
        } Count{ this };
    } Items{ this };
    // checkedListBox1->SetItemChecked(0, true);
    void SetItemChecked(int i, bool v) {
        if (!Handle || i < 0) return;
        LVITEMW li = { 0 };
        li.mask = LVIF_STATE;
        li.state = INDEXTOSTATEIMAGEMASK(v ? 2 : 1);
        li.stateMask = LVIS_STATEIMAGEMASK;
        SendMessageW(Handle, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&li);
    }
    bool GetItemChecked(int i) const {
        if (!Handle || i < 0) return false;
        return (SendMessageW(Handle, LVM_GETITEMSTATE, (WPARAM)i, LVIS_STATEIMAGEMASK) >> 12) == 2;
    }
    // Isaretli ogelerin sayisi/indeksleri
    int CheckedItemCount() const {
        int n = (int)Items.Count, cnt = 0;
        for (int i = 0; i < n; i++) if (GetItemChecked(i)) cnt++;
        return cnt;
    }
    bool GetItemCheckedByIndex(int i) const { return GetItemChecked(i); }
    bool CheckOnClick = false;      // davranissal bilgi
    bool ThreeDCheckBoxes = false;
};

// ============================================================================
//  ListView — Columns/Items API + View/CheckBoxes/GridLines
// ============================================================================
class ListView : public Control {
public:
    ListView(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysListView32", text.c_str(),
            LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) {
            SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
            ListView_SetExtendedListViewStyle(Handle, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        }
    }
    // listView1->Columns->Add(L"Ad", 140);
    class ColumnsProp {
        ListView* c;
    public:
        explicit ColumnsProp(ListView* o) : c(o) {}
        ColumnsProp* operator->() { return this; }
        void Clear() { if (c->Handle) { while ((int)SendMessageW(c->Handle, LVM_DELETECOLUMN, 0, 0) != -1) {} } }
        void Add(const std::wstring& title, int width = 120) {
            if (!c->Handle) return;
            LVCOLUMNW lc = { 0 };
            lc.mask = LVCF_TEXT | LVCF_WIDTH;
            lc.pszText = (LPWSTR)title.c_str();
            lc.cx = width;
            int n = (int)SendMessageW(c->Handle, LVM_GETITEMCOUNT, 0, 0);
            SendMessageW(c->Handle, LVM_INSERTCOLUMNW, (WPARAM)n, (LPARAM)&lc);
        }
        void RemoveAt(int i) { if (c->Handle) SendMessageW(c->Handle, LVM_DELETECOLUMN, (WPARAM)i, 0); }
        int  Count() const { return c->Handle ? Header_GetItemCount(c->Handle) : 0; }
        void SetWidth(int i, int w) {
            if (!c->Handle) return;
            LVCOLUMNW lc = { 0 };
            lc.mask = LVCF_WIDTH;
            lc.cx = w;
            SendMessageW(c->Handle, LVM_SETCOLUMNW, (WPARAM)i, (LPARAM)&lc);
        }
    } Columns{ this };
    // listView1->Items->Add(L"satir");  Items->Add(L"ad", L"soyad");
    class ItemsProp {
        ListView* c;
    public:
        explicit ItemsProp(ListView* o) : c(o) {}
        ItemsProp* operator->() { return this; }
        void Clear() { if (c->Handle) SendMessageW(c->Handle, LVM_DELETEALLITEMS, 0, 0); }
        int Add(const std::wstring& s) {
            if (!c->Handle) return -1;
            LVITEMW li = { 0 };
            li.mask = LVIF_TEXT;
            li.iItem = (int)SendMessageW(c->Handle, LVM_GETITEMCOUNT, 0, 0);
            li.pszText = (LPWSTR)s.c_str();
            return (int)SendMessageW(c->Handle, LVM_INSERTITEMW, 0, (LPARAM)&li);
        }
        int Add(const std::wstring& s, const std::wstring& sub) {
            int idx = Add(s);
            if (idx >= 0) SetSub(idx, 1, sub);
            return idx;
        }
        int Add(const std::wstring& a, const std::wstring& b, const std::wstring& c3) {
            int idx = Add(a, b);
            if (idx >= 0) SetSub(idx, 2, c3);
            return idx;
        }
        int Insert(int i, const std::wstring& s) {
            if (!c->Handle) return -1;
            LVITEMW li = { 0 };
            li.mask = LVIF_TEXT;
            li.iItem = i;
            li.pszText = (LPWSTR)s.c_str();
            return (int)SendMessageW(c->Handle, LVM_INSERTITEMW, 0, (LPARAM)&li);
        }
        void RemoveAt(int i) { if (c->Handle) SendMessageW(c->Handle, LVM_DELETEITEM, (WPARAM)i, 0); }
        int  GetCount() const { return c->Handle ? (int)SendMessageW(c->Handle, LVM_GETITEMCOUNT, 0, 0) : 0; }
        void SetSub(int i, int col, const std::wstring& s) {
            if (!c->Handle) return;
            LVITEMW li = { 0 };
            li.mask = LVIF_TEXT;
            li.iItem = i;
            li.iSubItem = col;
            li.pszText = (LPWSTR)s.c_str();
            SendMessageW(c->Handle, LVM_SETITEMTEXTW, (WPARAM)i, (LPARAM)&li);
        }
        std::wstring GetSub(int i, int col) const {
            if (!c->Handle) return L"";
            wchar_t buf[256] = L"";
            LVITEMW li = { 0 };
            li.mask = LVIF_TEXT;
            li.iItem = i;
            li.iSubItem = col;
            li.pszText = buf;
            li.cchTextMax = 256;
            SendMessageW(c->Handle, LVM_GETITEMTEXTW, (WPARAM)i, (LPARAM)&li);
            return buf;
        }
        int  GetSelectedIndex() const { return c->Handle ? (int)SendMessageW(c->Handle, LVM_GETNEXTITEM, (WPARAM)-1, MAKELPARAM(LVNI_SELECTED, 0)) : -1; }
        std::wstring GetSelectedItem() const { return GetSub(GetSelectedIndex(), 0); }
        void EnsureVisible(int i) { if (c->Handle) SendMessageW(c->Handle, LVM_ENSUREVISIBLE, (WPARAM)i, 0); }
        void BeginUpdate() { if (c->Handle) SendMessageW(c->Handle, WM_SETREDRAW, FALSE, 0); }
        void EndUpdate()   { if (c->Handle) { SendMessageW(c->Handle, WM_SETREDRAW, TRUE, 0); InvalidateRect(c->Handle, nullptr, TRUE); } }
        class SelIdxProp {
            ItemsProp* p;
        public:
            explicit SelIdxProp(ItemsProp* o) : p(o) {}
            SelIdxProp& operator=(int i) {
                if (p->c->Handle) {
                    LVITEMW li = { 0 };
                    li.mask = LVIF_STATE;
                    li.state = LVIS_SELECTED;
                    li.stateMask = LVIS_SELECTED;
                    SendMessageW(p->c->Handle, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&li);
                    if (i >= 0) SendMessageW(p->c->Handle, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&li);
                }
                return *this;
            }
            operator int() const { return p->GetSelectedIndex(); }
        } SelectedIndex{ this };
        class SelItemProp {
            ItemsProp* p;
        public:
            explicit SelItemProp(ItemsProp* o) : p(o) {}
            operator std::wstring() const { return p->GetSelectedItem(); }
        } SelectedItem{ this };
        class CountProp {
            ItemsProp* p;
        public:
            explicit CountProp(ItemsProp* o) : p(o) {}
            operator int() const { return p->GetCount(); }
        } Count{ this };
    } Items{ this };
    // listView1->View = M2God::View::Details;
    class ViewProp {
        Control* c;
    public:
        explicit ViewProp(Control* o) : c(o) {}
        ViewProp& operator=(M2God::View v) {
            if (c->Handle) {
                LONG lv = (v == M2God::View::Details) ? LVS_REPORT :
                          (v == M2God::View::List) ? LVS_LIST :
                          (v == M2God::View::SmallIcon) ? LVS_SMALLICON : LVS_ICON;
                SendMessageW(c->Handle, LVM_SETVIEW, (WPARAM)lv, 0);
            }
            return *this;
        }
    } View{ this };
    // listView1->CheckBoxes = true;
    class CheckBoxesProp {
        Control* c;
    public:
        explicit CheckBoxesProp(Control* o) : c(o) {}
        CheckBoxesProp& operator=(bool v) {
            if (c->Handle) ListView_SetExtendedListViewStyleEx(c->Handle, LVS_EX_CHECKBOXES, v ? LVS_EX_CHECKBOXES : 0);
            return *this;
        }
        operator bool() const {
            return c->Handle && (ListView_GetExtendedListViewStyle(c->Handle) & LVS_EX_CHECKBOXES) != 0;
        }
    } CheckBoxes{ this };
    class FullRowSelectProp {
        Control* c;
    public:
        explicit FullRowSelectProp(Control* o) : c(o) {}
        FullRowSelectProp& operator=(bool v) {
            if (c->Handle) ListView_SetExtendedListViewStyleEx(c->Handle, LVS_EX_FULLROWSELECT, v ? LVS_EX_FULLROWSELECT : 0);
            return *this;
        }
    } FullRowSelect{ this };
    class GridLinesProp {
        Control* c;
    public:
        explicit GridLinesProp(Control* o) : c(o) {}
        GridLinesProp& operator=(bool v) {
            if (c->Handle) ListView_SetExtendedListViewStyleEx(c->Handle, LVS_EX_GRIDLINES, v ? LVS_EX_GRIDLINES : 0);
            return *this;
        }
    } GridLines{ this };
    class HeaderStyleProp {
        Control* c;
    public:
        explicit HeaderStyleProp(Control* o) : c(o) {}
        HeaderStyleProp& operator=(M2God::HeaderStyle hs) {
            if (!c->Handle) return *this;
            LONG_PTR add = 0, rem = 0;
            switch (hs) {
            case M2God::HeaderStyle::None:        add = LVS_NOCOLUMNHEADER; break;
            case M2God::HeaderStyle::NonClickable: add = LVS_NOSORTHEADER; break;
            default: rem = LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER; break;
            }
            RestyleWindow(c->Handle, add, rem);
            return *this;
        }
    } HeaderStyle{ this };
    // item isaretlemesi (CheckBoxes = true iken)
    void SetItemChecked(int i, bool v) {
        if (!Handle || i < 0) return;
        LVITEMW li = { 0 };
        li.mask = LVIF_STATE;
        li.state = INDEXTOSTATEIMAGEMASK(v ? 2 : 1);
        li.stateMask = LVIS_STATEIMAGEMASK;
        SendMessageW(Handle, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&li);
    }
    bool GetItemChecked(int i) const {
        if (!Handle || i < 0) return false;
        return (SendMessageW(Handle, LVM_GETITEMSTATE, (WPARAM)i, LVIS_STATEIMAGEMASK) >> 12) == 2;
    }
    bool MultiSelect = true;   // yaratma stili (LVS_SINGLESEL); bilgi amacli
    M2God::SortOrder Sorting = M2God::SortOrder::None;
    void Clear() { if (Handle) { SendMessageW(Handle, LVM_DELETEALLITEMS, 0, 0); } }
};

// ============================================================================
//  TreeView — index tabanli Nodes API
// ============================================================================
class TreeView : public Control {
public:
    TreeView(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysTreeView32", text.c_str(),
            TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_BORDER | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    static std::map<HWND, std::vector<HTREEITEM>>& NodeTable() {
        static std::map<HWND, std::vector<HTREEITEM>> tbl;
        return tbl;
    }
    std::vector<HTREEITEM>& Nodes_() { return NodeTable()[Handle]; }
    // treeView1->Nodes->Add(L"kok");  ->AddChild(0, L"cocuk");
    class NodesProp {
        TreeView* c;
    public:
        explicit NodesProp(TreeView* o) : c(o) {}
        NodesProp* operator->() { return this; }
        void Clear() {
            if (c->Handle) { SendMessageW(c->Handle, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT); c->Nodes_().clear(); }
        }
        int Add(const std::wstring& s) { return AddChild(-1, s); }
        int AddChild(int parentIdx, const std::wstring& s) {
            if (!c->Handle) return -1;
            TVINSERTSTRUCTW is = { 0 };
            is.hInsertAfter = TVI_LAST;
            is.item.mask = TVIF_TEXT;
            is.item.pszText = (LPWSTR)s.c_str();
            std::vector<HTREEITEM>& tbl = c->Nodes_();
            if (parentIdx >= 0 && parentIdx < (int)tbl.size()) is.hParent = tbl[parentIdx];
            else { is.hParent = TVI_ROOT; parentIdx = -1; }
            HTREEITEM h = (HTREEITEM)SendMessageW(c->Handle, TVM_INSERTITEMW, 0, (LPARAM)&is);
            tbl.push_back(h);
            return (int)tbl.size() - 1;
        }
        void RemoveAt(int i) {
            if (!c->Handle || i < 0 || i >= (int)c->Nodes_().size()) return;
            HTREEITEM h = c->Nodes_()[i];
            if (h) SendMessageW(c->Handle, TVM_DELETEITEM, 0, (LPARAM)h);
            c->Nodes_().erase(c->Nodes_().begin() + i);
        }
        int GetCount() const { return (int)(c->NodeTable().count(c->Handle) ? c->NodeTable()[c->Handle].size() : 0); }
        std::wstring GetText(int i) const {
            if (!c->Handle || i < 0 || i >= GetCount()) return L"";
            wchar_t buf[256] = L"";
            TVITEMW ti = { 0 };
            ti.mask = TVIF_TEXT;
            ti.hItem = c->NodeTable()[c->Handle][i];
            ti.pszText = buf;
            ti.cchTextMax = 256;
            SendMessageW(c->Handle, TVM_GETITEMW, 0, (LPARAM)&ti);
            return buf;
        }
        void Expand(int i) {
            if (c->Handle && i >= 0 && i < GetCount())
                TreeView_Expand(c->Handle, c->NodeTable()[c->Handle][i], TVE_EXPAND);
        }
        void ExpandAll() {
            if (!c->Handle) return;
            HTREEITEM h = TreeView_GetRoot(c->Handle);
            while (h) { TreeView_Expand(c->Handle, h, TVE_EXPAND); h = TreeView_GetNextItem(c->Handle, h, TVGN_NEXTVISIBLE); }
        }
        class CountProp {
            NodesProp* p;
        public:
            explicit CountProp(NodesProp* o) : p(o) {}
            operator int() const { return p->GetCount(); }
        } Count{ this };
    } Nodes{ this };
    class SelectedIndexProp {
        TreeView* c;
    public:
        explicit SelectedIndexProp(TreeView* o) : c(o) {}
        SelectedIndexProp& operator=(int i) {
            if (!c->Handle) return *this;
            if (i >= 0 && i < (int)c->Nodes_().size())
                SendMessageW(c->Handle, TVM_SELECTITEM, (WPARAM)TVGN_CARET, (LPARAM)c->Nodes_()[i]);
            return *this;
        }
        operator int() const {
            if (!c->Handle) return -1;
            HTREEITEM h = TreeView_GetSelection(c->Handle);
            std::vector<HTREEITEM>& tbl = c->Nodes_();
            for (int i = 0; i < (int)tbl.size(); i++) if (tbl[i] == h) return i;
            return -1;
        }
    } SelectedIndex{ this };
    class SelectedTextProp {
        TreeView* c;
    public:
        explicit SelectedTextProp(TreeView* o) : c(o) {}
        operator std::wstring() const {
            int i = (int)c->SelectedIndex;
            return i >= 0 ? c->Nodes.GetText(i) : L"";
        }
    } SelectedText{ this };
    class CheckBoxesProp {
        Control* c;
    public:
        explicit CheckBoxesProp(Control* o) : c(o) {}
        CheckBoxesProp& operator=(bool v) {
            // TVS_CHECKBOXES yaratmadan once ayarlanmali; ogeler eklenmeden uygulandiginda calisir
            if (c->Handle) RestyleWindow(c->Handle, v ? TVS_CHECKBOXES : 0, v ? 0 : TVS_CHECKBOXES);
            return *this;
        }
    } CheckBoxes{ this };
    bool GetItemChecked(int i) const {
        std::vector<HTREEITEM>& tbl = NodeTable()[Handle];
        if (!Handle || i < 0 || i >= (int)tbl.size()) return false;
        TVITEMW ti = { 0 };
        ti.mask = TVIF_STATE;
        ti.hItem = tbl[i];
        ti.stateMask = TVIS_STATEIMAGEMASK;
        SendMessageW(Handle, TVM_GETITEMW, 0, (LPARAM)&ti);
        return (((ti.state & TVIS_STATEIMAGEMASK) >> 12) == 2);
    }
    void SetItemChecked(int i, bool v) {
        if (!Handle || i < 0 || i >= (int)Nodes_().size()) return;
        TVITEMW ti = { 0 };
        ti.mask = TVIF_STATE;
        ti.hItem = Nodes_()[i];
        ti.stateMask = TVIS_STATEIMAGEMASK;
        ti.state = INDEXTOSTATEIMAGEMASK(v ? 2 : 1);
        SendMessageW(Handle, TVM_SETITEMW, 0, (LPARAM)&ti);
    }
    class ShowLinesProp {
        Control* c;
    public:
        explicit ShowLinesProp(Control* o) : c(o) {}
        ShowLinesProp& operator=(bool v) { if (c->Handle) RestyleWindow(c->Handle, v ? TVS_HASLINES : 0, v ? 0 : TVS_HASLINES); return *this; }
    } ShowLines{ this };
    class ShowPlusMinusProp {
        Control* c;
    public:
        explicit ShowPlusMinusProp(Control* o) : c(o) {}
        ShowPlusMinusProp& operator=(bool v) { if (c->Handle) RestyleWindow(c->Handle, v ? TVS_HASBUTTONS : 0, v ? 0 : TVS_HASBUTTONS); return *this; }
    } ShowPlusMinus{ this };
    class ShowRootLinesProp {
        Control* c;
    public:
        explicit ShowRootLinesProp(Control* o) : c(o) {}
        ShowRootLinesProp& operator=(bool v) { if (c->Handle) RestyleWindow(c->Handle, v ? TVS_LINESATROOT : 0, v ? 0 : TVS_LINESATROOT); return *this; }
    } ShowRootLines{ this };
    class HotTrackingProp {
        Control* c;
    public:
        explicit HotTrackingProp(Control* o) : c(o) {}
        HotTrackingProp& operator=(bool v) { if (c->Handle) RestyleWindow(c->Handle, v ? TVS_TRACKSELECT : 0, v ? 0 : TVS_TRACKSELECT); return *this; }
    } HotTracking{ this };
    class IndentProp {
        Control* c;
    public:
        explicit IndentProp(Control* o) : c(o) {}
        IndentProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, TVM_SETINDENT, (WPARAM)v, 0); return *this; }
    } Indent{ this };
    std::wstring PathSeparator = L"\\";
};

// ============================================================================
//  ProgressBar
// ============================================================================
class ProgressBar : public Control {
public:
    ProgressBar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"msctls_progress32", text.c_str(),
            WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        SendMessageW(Handle, PBM_SETRANGE32, 0, 100);
    }
    class ValueProp {
        Control* c;
    public:
        explicit ValueProp(Control* o) : c(o) {}
        ValueProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, PBM_SETPOS, (WPARAM)v, 0); return *this; }
        operator int() const { return c->Handle ? (int)SendMessageW(c->Handle, PBM_GETPOS, 0, 0) : 0; }
    } Value{ this };
    class MinimumProp {
        Control* c;
    public:
        explicit MinimumProp(Control* o) : c(o) {}
        MinimumProp& operator=(int v) { if (c->Handle) { int mx = 100; PBRANGE pr; SendMessageW(c->Handle, PBM_GETRANGE, FALSE, (LPARAM)&pr); mx = pr.iHigh; SendMessageW(c->Handle, PBM_SETRANGE32, (WPARAM)v, (LPARAM)mx); } return *this; }
        operator int() const { if (!c->Handle) return 0; PBRANGE pr; SendMessageW(c->Handle, PBM_GETRANGE, TRUE, (LPARAM)&pr); return pr.iLow; }
    } Minimum{ this };
    class MaximumProp {
        Control* c;
    public:
        explicit MaximumProp(Control* o) : c(o) {}
        MaximumProp& operator=(int v) { if (c->Handle) { PBRANGE pr; SendMessageW(c->Handle, PBM_GETRANGE, TRUE, (LPARAM)&pr); SendMessageW(c->Handle, PBM_SETRANGE32, (WPARAM)pr.iLow, (LPARAM)v); } return *this; }
        operator int() const { if (!c->Handle) return 100; PBRANGE pr; SendMessageW(c->Handle, PBM_GETRANGE, FALSE, (LPARAM)&pr); return pr.iHigh; }
    } Maximum{ this };
    class StepProp {
        Control* c;
    public:
        explicit StepProp(Control* o) : c(o) {}
        StepProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, PBM_SETSTEP, (WPARAM)v, 0); return *this; }
    } Step{ this };
    class StyleProp {
        Control* c;
    public:
        explicit StyleProp(Control* o) : c(o) {}
        StyleProp& operator=(M2God::ProgressBarStyle st) {
            if (!c->Handle) return *this;
            if (st == M2God::ProgressBarStyle::Marquee) {
                RestyleWindow(c->Handle, PBS_MARQUEE, 0);
                SendMessageW(c->Handle, PBM_SETMARQUEE, TRUE, (LPARAM)30);
            } else {
                SendMessageW(c->Handle, PBM_SETMARQUEE, FALSE, 0);
                RestyleWindow(c->Handle, 0, PBS_MARQUEE);
            }
            return *this;
        }
    } Style{ this };
    void PerformStep() { if (Handle) SendMessageW(Handle, PBM_STEPIT, 0, 0); }
    void Increment(int n) { if (Handle) SendMessageW(Handle, PBM_DELTAPOS, (WPARAM)n, 0); }
    int  StepStored = 10;
};

// ============================================================================
//  TrackBar
// ============================================================================
class TrackBar : public Control {
public:
    TrackBar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"msctls_trackbar32", text.c_str(),
            WS_TABSTOP | TBS_HORZ | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        SendMessageW(Handle, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    }
    class ValueProp {
        Control* c;
    public:
        explicit ValueProp(Control* o) : c(o) {}
        ValueProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, TBM_SETPOS, TRUE, (LPARAM)v); return *this; }
        operator int() const { return c->Handle ? (int)SendMessageW(c->Handle, TBM_GETPOS, 0, 0) : 0; }
    } Value{ this };
    class MinimumProp {
        Control* c;
    public:
        explicit MinimumProp(Control* o) : c(o) {}
        MinimumProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, TBM_SETRANGEMIN, TRUE, (LPARAM)v); return *this; }
        operator int() const { return c->Handle ? (int)SendMessageW(c->Handle, TBM_GETRANGEMIN, 0, 0) : 0; }
    } Minimum{ this };
    class MaximumProp {
        Control* c;
    public:
        explicit MaximumProp(Control* o) : c(o) {}
        MaximumProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, TBM_SETRANGEMAX, TRUE, (LPARAM)v); return *this; }
        operator int() const { return c->Handle ? (int)SendMessageW(c->Handle, TBM_GETRANGEMAX, 0, 0) : 100; }
    } Maximum{ this };
    class TickFrequencyProp {
        Control* c;
    public:
        explicit TickFrequencyProp(Control* o) : c(o) {}
        TickFrequencyProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, TBM_SETTICFREQ, (WPARAM)(v > 0 ? v : 1), 0); return *this; }
    } TickFrequency{ this };
    class SmallChangeProp {
        Control* c;
    public:
        explicit SmallChangeProp(Control* o) : c(o) {}
        SmallChangeProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, TBM_SETLINESIZE, 0, (LPARAM)v); return *this; }
    } SmallChange{ this };
    class LargeChangeProp {
        Control* c;
    public:
        explicit LargeChangeProp(Control* o) : c(o) {}
        LargeChangeProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, TBM_SETPAGESIZE, 0, (LPARAM)v); return *this; }
    } LargeChange{ this };
    class TickStyleProp {
        Control* c;
    public:
        explicit TickStyleProp(Control* o) : c(o) {}
        TickStyleProp& operator=(M2God::TickStyle ts) {
            if (c->Handle) {
                if (ts == M2God::TickStyle::None) SendMessageW(c->Handle, TBM_CLEARTICS, FALSE, 0);
                else RestyleWindow(c->Handle, TBS_AUTOTICKS, 0);
            }
            return *this;
        }
    } TickStyle{ this };
    void SetRange(int mn, int mx, bool redraw = true) {
        if (Handle) SendMessageW(Handle, TBM_SETRANGE, redraw ? TRUE : FALSE, MAKELPARAM(mn, mx));
    }
};

// ============================================================================
//  HScrollBar / VScrollBar — SetScrollInfo tabanli
// ============================================================================
class ScrollBarBase : public Control {
public:
    ScrollBarBase() {}
    class ValueProp {
        ScrollBarBase* c;
    public:
        explicit ValueProp(ScrollBarBase* o) : c(o) {}
        ValueProp& operator=(int v) {
            if (c->Handle) {
                SCROLLINFO si = { sizeof(si) };
                si.fMask = SIF_ALL;
                GetScrollInfo(c->Handle, SB_CTL, &si);
                si.nPos = v; si.fMask = SIF_POS;
                SetScrollInfo(c->Handle, SB_CTL, &si, TRUE);
            }
            return *this;
        }
        operator int() const {
            if (!c->Handle) return 0;
            SCROLLINFO si = { sizeof(si), SIF_POS };
            GetScrollInfo(c->Handle, SB_CTL, &si);
            return si.nPos;
        }
    } Value{ this };
    class MinimumProp {
        ScrollBarBase* c;
    public:
        explicit MinimumProp(ScrollBarBase* o) : c(o) {}
        MinimumProp& operator=(int v) { c->SetRange2(v, -1); return *this; }
        operator int() const { return c->MinStored; }
    } Minimum{ this };
    class MaximumProp {
        ScrollBarBase* c;
    public:
        explicit MaximumProp(ScrollBarBase* o) : c(o) {}
        MaximumProp& operator=(int v) { c->SetRange2(-1, v); return *this; }
        operator int() const { return c->MaxStored; }
    } Maximum{ this };
    class SmallChangeProp {
        ScrollBarBase* c;
    public:
        explicit SmallChangeProp(ScrollBarBase* o) : c(o) {}
        SmallChangeProp& operator=(int v) { c->SmallStored = v; return *this; }
        operator int() const { return c->SmallStored; }
    } SmallChange{ this };
    class LargeChangeProp {
        ScrollBarBase* c;
    public:
        explicit LargeChangeProp(ScrollBarBase* o) : c(o) {}
        LargeChangeProp& operator=(int v) { c->LargeStored = v; c->SetRange2(-1, -1); return *this; }
        operator int() const { return c->LargeStored; }
    } LargeChange{ this };

    int MinStored = 0, MaxStored = 100, SmallStored = 1, LargeStored = 10;
    void SetRange2(int mn, int mx) {
        if (mn >= 0) MinStored = mn;
        if (mx >= 0) MaxStored = mx;
        if (!Handle) return;
        SCROLLINFO si = { sizeof(si) };
        si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        si.nMin = MinStored;
        si.nMax = MaxStored;
        si.nPage = (UINT)(LargeStored > 0 ? LargeStored : 1);
        si.nPos = MinStored;
        SetScrollInfo(Handle, SB_CTL, &si, TRUE);
    }
};
class HScrollBar : public ScrollBarBase {
public:
    HScrollBar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"SCROLLBAR", text.c_str(),
            SBS_HORZ | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        SetRange2(0, 100);
    }
};
class VScrollBar : public ScrollBarBase {
public:
    VScrollBar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"SCROLLBAR", text.c_str(),
            SBS_VERT | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        SetRange2(0, 100);
    }
};

// ============================================================================
//  DateTimePicker / MonthCalendar
// ============================================================================
class DateTimePicker : public Control {
public:
    DateTimePicker(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysDateTimePick32", text.c_str(),
            WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    // dateTimePicker1->Value = M2God::M2DateTime(2026, 1, 15);
    class ValueProp {
        DateTimePicker* c;
    public:
        explicit ValueProp(DateTimePicker* o) : c(o) {}
        ValueProp& operator=(const M2DateTime& dt) {
            if (c->Handle) { SYSTEMTIME st; dt.ToSystemTime(st); SendMessageW(c->Handle, DTM_SETSYSTEMTIME, (WPARAM)GDT_VALID, (LPARAM)&st); }
            return *this;
        }
        operator M2DateTime() const {
            SYSTEMTIME st;
            if (c->Handle && SendMessageW(c->Handle, DTM_GETSYSTEMTIME, 0, (LPARAM)&st) == GDT_VALID)
                return M2DateTime::FromSystemTime(st);
            return M2DateTime();
        }
    } Value{ this };
    class FormatProp {
        DateTimePicker* c;
    public:
        explicit FormatProp(DateTimePicker* o) : c(o) {}
        FormatProp& operator=(M2God::DateTimePickerFormat f) {
            if (!c->Handle) return *this;
            const wchar_t* fmt = L"dd.MM.yyyy";
            switch (f) {
            case M2God::DateTimePickerFormat::Long:   fmt = L"dddd, dd MMMM yyyy"; break;
            case M2God::DateTimePickerFormat::Short:  fmt = L"dd.MM.yyyy"; break;
            case M2God::DateTimePickerFormat::Time:   fmt = L"HH:mm:ss"; break;
            case M2God::DateTimePickerFormat::Custom: fmt = nullptr; break;   // CustomFormat ile birlikte
            }
            if (fmt) SendMessageW(c->Handle, DTM_SETFORMATW, 0, (LPARAM)fmt);
            return *this;
        }
    } Format{ this };
    class CustomFormatProp {
        DateTimePicker* c;
    public:
        explicit CustomFormatProp(DateTimePicker* o) : c(o) {}
        CustomFormatProp& operator=(const std::wstring& fmt) {
            if (c->Handle) SendMessageW(c->Handle, DTM_SETFORMATW, 0, (LPARAM)fmt.c_str());
            return *this;
        }
    } CustomFormat{ this };
    class CheckedProp {
        DateTimePicker* c;
    public:
        explicit CheckedProp(DateTimePicker* o) : c(o) {}
        CheckedProp& operator=(bool v) {
            if (!c->Handle) return *this;
            SYSTEMTIME st;
            if (!v) SendMessageW(c->Handle, DTM_SETSYSTEMTIME, (WPARAM)GDT_NONE, 0);
            else {
                if (SendMessageW(c->Handle, DTM_GETSYSTEMTIME, 0, (LPARAM)&st) != GDT_VALID)
                    GetLocalTime(&st);
                SendMessageW(c->Handle, DTM_SETSYSTEMTIME, (WPARAM)GDT_VALID, (LPARAM)&st);
            }
            return *this;
        }
        operator bool() const {
            if (!c->Handle) return false;
            SYSTEMTIME st;
            return SendMessageW(c->Handle, DTM_GETSYSTEMTIME, 0, (LPARAM)&st) == GDT_VALID;
        }
    } Checked{ this };
    bool ShowUpDown = false;     // yaratma stili (DTS_UPDOWN); bilgi amacli
    bool ShowCheckBox = false;   // yaratma stili (DTS_SHOWNONE); bilgi amacli
};

class MonthCalendar : public Control {
public:
    MonthCalendar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"SysMonthCal32", text.c_str(),
            WS_TABSTOP | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    void SetDate(int y, int m, int d) {
        if (!Handle) return;
        SYSTEMTIME st;
        ZeroMemory(&st, sizeof(st));
        st.wYear = (WORD)y; st.wMonth = (WORD)m; st.wDay = (WORD)d;
        SendMessageW(Handle, MCM_SETCURSEL, 0, (LPARAM)&st);
    }
    class MaxSelectionCountProp {
        MonthCalendar* c;
    public:
        explicit MaxSelectionCountProp(MonthCalendar* o) : c(o) {}
        MaxSelectionCountProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, MCM_SETMAXSELCOUNT, (WPARAM)v, 0); return *this; }
    } MaxSelectionCount{ this };
    class TitleBackColorProp {
        MonthCalendar* c;
    public:
        explicit TitleBackColorProp(MonthCalendar* o) : c(o) {}
        TitleBackColorProp& operator=(COLORREF cr) { if (c->Handle) SendMessageW(c->Handle, MCM_SETCOLOR, (WPARAM)MCSC_TITLEBK, (LPARAM)cr); return *this; }
    } TitleBackColor{ this };
    class TitleForeColorProp {
        MonthCalendar* c;
    public:
        explicit TitleForeColorProp(MonthCalendar* o) : c(o) {}
        TitleForeColorProp& operator=(COLORREF cr) { if (c->Handle) SendMessageW(c->Handle, MCM_SETCOLOR, (WPARAM)MCSC_TITLETEXT, (LPARAM)cr); return *this; }
    } TitleForeColor{ this };
    class TrailingForeColorProp {
        MonthCalendar* c;
    public:
        explicit TrailingForeColorProp(MonthCalendar* o) : c(o) {}
        TrailingForeColorProp& operator=(COLORREF cr) { if (c->Handle) SendMessageW(c->Handle, MCM_SETCOLOR, (WPARAM)MCSC_TRAILINGTEXT, (LPARAM)cr); return *this; }
    } TrailingForeColor{ this };
    class FirstDayOfWeekProp {
        MonthCalendar* c;
    public:
        explicit FirstDayOfWeekProp(MonthCalendar* o) : c(o) {}
        FirstDayOfWeekProp& operator=(int v) {
            if (c->Handle) { DWORD f = MAKELONG((v >= 0 && v <= 6) ? v : 0, TRUE); SendMessageW(c->Handle, MCM_SETFIRSTDAYOFWEEK, 0, (LPARAM)f); }
            return *this;
        }
    } FirstDayOfWeek{ this };
    bool ShowWeekNumbers = false;   // yaratma stili (MCS_WEEKNUMBERS)
    M2DateTime TodayDate() const {
        SYSTEMTIME st;
        if (Handle && SendMessageW(Handle, MCM_GETTODAY, 0, (LPARAM)&st)) return M2DateTime::FromSystemTime(st);
        return M2DateTime();
    }
};

// ============================================================================
//  TabControl
// ============================================================================
class TabControl : public Control {
public:
    TabControl(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysTabControl32", text.c_str(),
            WS_TABSTOP | WS_CLIPCHILDREN | TCS_TABS | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
    // tabControl1->Pages->Add(L"tabPage1");  Pages->SelectedIndex = 0;
    class PagesProp {
        TabControl* c;
    public:
        explicit PagesProp(TabControl* o) : c(o) {}
        PagesProp* operator->() { return this; }
        int GetCount() const { return c->Handle ? (int)SendMessageW(c->Handle, TCM_GETITEMCOUNT, 0, 0) : 0; }
        void Add(const std::wstring& title) {
            if (!c->Handle) return;
            TCITEMW ti = { 0 };
            ti.mask = TCIF_TEXT;
            ti.pszText = (LPWSTR)title.c_str();
            SendMessageW(c->Handle, TCM_INSERTITEMW, (WPARAM)GetCount(), (LPARAM)&ti);
        }
        void Insert(int i, const std::wstring& title) {
            if (!c->Handle) return;
            TCITEMW ti = { 0 };
            ti.mask = TCIF_TEXT;
            ti.pszText = (LPWSTR)title.c_str();
            SendMessageW(c->Handle, TCM_INSERTITEMW, (WPARAM)i, (LPARAM)&ti);
        }
        void RemoveAt(int i) { if (c->Handle) SendMessageW(c->Handle, TCM_DELETEITEM, (WPARAM)i, 0); }
        void Clear() { if (c->Handle) { while ((int)SendMessageW(c->Handle, TCM_DELETEITEM, 0, 0)) {} } }
        std::wstring GetText(int i) const {
            if (!c->Handle) return L"";
            wchar_t buf[256] = L"";
            TCITEMW ti = { 0 };
            ti.mask = TCIF_TEXT;
            ti.pszText = buf;
            ti.cchTextMax = 256;
            SendMessageW(c->Handle, TCM_GETITEMW, (WPARAM)i, (LPARAM)&ti);
            return buf;
        }
        class SelIdxProp {
            PagesProp* p;
        public:
            explicit SelIdxProp(PagesProp* o) : p(o) {}
            SelIdxProp& operator=(int i) { if (p->c->Handle) SendMessageW(p->c->Handle, TCM_SETCURSEL, (WPARAM)i, 0); return *this; }
            operator int() const { return p->c->Handle ? (int)SendMessageW(p->c->Handle, TCM_GETCURSEL, 0, 0) : -1; }
        } SelectedIndex{ this };
        class CountProp {
            PagesProp* p;
        public:
            explicit CountProp(PagesProp* o) : p(o) {}
            operator int() const { return p->GetCount(); }
        } Count{ this };
    } Pages{ this };
    PagesProp TabPages{ this };   // WinForms adi
    class HotTrackProp {
        Control* c;
    public:
        explicit HotTrackProp(Control* o) : c(o) {}
        HotTrackProp& operator=(bool v) { if (c->Handle) RestyleWindow(c->Handle, v ? TCS_HOTTRACK : 0, v ? 0 : TCS_HOTTRACK); return *this; }
    } HotTrack{ this };
    bool Multiline = false;   // yaratma stili (TCS_MULTILINE)
    int RowCount() const { return Handle ? (int)SendMessageW(Handle, TCM_GETROWCOUNT, 0, 0) : 0; }
};

// ============================================================================
//  StatusStrip — msctls_statusbar32 (duzgun SB_SETPARTS ile)
// ============================================================================
class StatusStrip : public Control {
public:
    StatusStrip(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"msctls_statusbar32", text.c_str(),
            WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
    }
    // statusStrip1->Parts->Add(L"Hazir");
    class PartsProp {
        StatusStrip* c;
    public:
        explicit PartsProp(StatusStrip* o) : c(o) {}
        PartsProp* operator->() { return this; }
        int Count() const { return c->Handle ? (int)SendMessageW(c->Handle, SB_GETPARTS, 0, 0) : 0; }
        void Add(const std::wstring& t) {
            if (!c->Handle) return;
            int n = Count();
            SendMessageW(c->Handle, SB_SETTEXTW, (WPARAM)(n | SBT_NOBORDERS), (LPARAM)t.c_str());
            // tum parcalari esit genislikte yeniden dagit
            RECT rc; GetClientRect(c->Handle, &rc);
            int W = rc.right - rc.left;
            int parts = n + 1;
            std::vector<int> edges((size_t)parts, 0);
            for (int i = 0; i < parts; i++) edges[(size_t)i] = (i == parts - 1) ? -1 : W * (i + 1) / parts;
            SendMessageW(c->Handle, SB_SETPARTS, (WPARAM)parts, (LPARAM)edges.data());
        }
        void SetText(int i, const std::wstring& t) {
            if (c->Handle) SendMessageW(c->Handle, SB_SETTEXTW, (WPARAM)(i | SBT_NOBORDERS), (LPARAM)t.c_str());
        }
        void Clear() {
            if (!c->Handle) return;
            int one = -1;
            SendMessageW(c->Handle, SB_SETPARTS, (WPARAM)1, (LPARAM)&one);
            SendMessageW(c->Handle, SB_SETTEXTW, (WPARAM)(0 | SBT_NOBORDERS), (LPARAM)L"");
        }
    } Parts{ this };
    PartsProp Items{ this };   // WinForms adi
    bool SizingGrip = true;
};

// ============================================================================
//  MenuStrip / ToolStrip — gercek HMENU (Ust tarafa baglanir)
//  Kullanim: menuStrip1->Items->Add(L"Dosya"); menuStrip1->Apply();
//  Tiklamalar: menuStrip1->ItemClick = [](int idx){ ... };  (WndProc yonlendirir)
// ============================================================================
class MenuStrip : public Control {
public:
    struct Item { std::wstring Text; std::vector<int> Children; bool Separator = false; };
    std::vector<Item> Items_;
    HWND ParentHwnd = NULL;
    std::function<void(int)> ItemClick;   // index
    HMENU hMenu = NULL;
    static const UINT FirstId = 0x7000;

    MenuStrip(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        ParentHwnd = parent;      // pencere yaratilmaz — menü Form'a baglanir
        Id = id;
        (void)text; (void)x; (void)y; (void)w; (void)h;
    }
    class ItemsProp {
        MenuStrip* c;
    public:
        explicit ItemsProp(MenuStrip* o) : c(o) {}
        ItemsProp* operator->() { return this; }
        int Add(const std::wstring& text) {
            c->Items_.push_back({ text, {}, false });
            return (int)c->Items_.size() - 1;
        }
        int AddSeparator() {
            c->Items_.push_back({ L"", {}, true });
            return (int)c->Items_.size() - 1;
        }
        int AddChild(int parentIdx, const std::wstring& text) {
            c->Items_.push_back({ text, {}, false });
            int idx = (int)c->Items_.size() - 1;
            if (parentIdx >= 0 && parentIdx < (int)c->Items_.size())
                c->Items_[(size_t)parentIdx].Children.push_back(idx);
            return idx;
        }
        int Count() const { return (int)c->Items_.size(); }
        void Clear() { c->Items_.clear(); }
    } Items{ this };
    void Apply() {   // menüyü forma bagla
        if (!ParentHwnd || Items_.empty()) return;
        if (hMenu) { DestroyMenu(hMenu); hMenu = nullptr; }
        hMenu = ::CreateMenu();
        BuildMenu(hMenu, -1);
        SetMenu(ParentHwnd, hMenu);
        DrawMenuBar(ParentHwnd);
    }
    void DispatchCommand(UINT id) {   // uretilen WndProc cagirir
        if (id >= FirstId && id < FirstId + (UINT)Items_.size() && ItemClick)
            ItemClick((int)(id - FirstId));
    }
    ~MenuStrip() { if (hMenu) DestroyMenu(hMenu); }
private:
    void BuildMenu(HMENU menu, int parentIdx) {
        for (int i = 0; i < (int)Items_.size(); i++) {
            const Item& it = Items_[(size_t)i];
            if (parentIdx < 0) {
                // ust seviye: cocuklari olanlar popup, olmayanlar da popup (basit kural)
                if (it.Separator) { ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr); continue; }
                HMENU sub = ::CreatePopupMenu();
                ::AppendMenuW(menu, MF_POPUP, (UINT_PTR)sub, it.Text.c_str());
                BuildMenu(sub, i);
            } else {
                bool isChildOf = false;
                for (int ch : Items_[(size_t)parentIdx].Children) if (ch == i) { isChildOf = true; break; }
                if (!isChildOf) continue;
                if (it.Separator) { ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr); continue; }
                if (!it.Children.empty()) {
                    HMENU sub = ::CreatePopupMenu();
                    ::AppendMenuW(menu, MF_POPUP, (UINT_PTR)sub, it.Text.c_str());
                    BuildMenu(sub, i);
                } else {
                    ::AppendMenuW(menu, MF_STRING, (UINT)(FirstId + i), it.Text.c_str());
                }
            }
        }
    }
};
typedef MenuStrip ToolStrip;

// ============================================================================
//  ContextMenuStrip — TrackPopupMenu ile
// ============================================================================
class ContextMenuStrip : public Control {
public:
    struct Item { std::wstring Text; std::vector<int> Children; bool Separator = false; };
    std::vector<Item> Items_;
    HWND ParentHwnd = NULL;
    std::function<void(int)> ItemClick;
    static const UINT FirstId = 0x7100;
    ContextMenuStrip(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        ParentHwnd = parent;
        Id = id;
        (void)text; (void)x; (void)y; (void)w; (void)h;
    }
    class ItemsProp {
        ContextMenuStrip* c;
    public:
        explicit ItemsProp(ContextMenuStrip* o) : c(o) {}
        ItemsProp* operator->() { return this; }
        int Add(const std::wstring& text) { c->Items_.push_back({ text, {}, false }); return (int)c->Items_.size() - 1; }
        int AddSeparator() { c->Items_.push_back({ L"", {}, true }); return (int)c->Items_.size() - 1; }
        int AddChild(int parentIdx, const std::wstring& text) {
            c->Items_.push_back({ text, {}, false });
            int idx = (int)c->Items_.size() - 1;
            if (parentIdx >= 0 && parentIdx < (int)c->Items_.size())
                c->Items_[(size_t)parentIdx].Children.push_back(idx);
            return idx;
        }
        int Count() const { return (int)c->Items_.size(); }
        void Clear() { c->Items_.clear(); }
    } Items{ this };
    void Show(int screenX, int screenY) {
        if (!ParentHwnd || Items_.empty()) return;
        HMENU menu = ::CreatePopupMenu();
        BuildMenu(menu, -1);
        SetForegroundWindow(ParentHwnd);
        int cmd = (int)TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                                      screenX, screenY, 0, ParentHwnd, nullptr);
        DestroyMenu(menu);
        if (cmd >= (int)FirstId && cmd < (int)FirstId + (int)Items_.size() && ItemClick)
            ItemClick(cmd - (int)FirstId);
    }
    void Show(const Point& screenPoint) { Show(screenPoint.X, screenPoint.Y); }
private:
    void BuildMenu(HMENU menu, int parentIdx) {
        for (int i = 0; i < (int)Items_.size(); i++) {
            const Item& it = Items_[(size_t)i];
            if (parentIdx < 0) {
                if (it.Separator) { ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr); continue; }
                if (!it.Children.empty()) {
                    HMENU sub = ::CreatePopupMenu();
                    ::AppendMenuW(menu, MF_POPUP, (UINT_PTR)sub, it.Text.c_str());
                    BuildMenu(sub, i);
                } else ::AppendMenuW(menu, MF_STRING, (UINT)(FirstId + i), it.Text.c_str());
            } else {
                bool isChildOf = false;
                for (int ch : Items_[(size_t)parentIdx].Children) if (ch == i) { isChildOf = true; break; }
                if (!isChildOf) continue;
                if (it.Separator) { ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr); continue; }
                if (!it.Children.empty()) {
                    HMENU sub = ::CreatePopupMenu();
                    ::AppendMenuW(menu, MF_POPUP, (UINT_PTR)sub, it.Text.c_str());
                    BuildMenu(sub, i);
                } else ::AppendMenuW(menu, MF_STRING, (UINT)(FirstId + i), it.Text.c_str());
            }
        }
    }
};

// ============================================================================
//  Panel / PictureBox / Timer
// ============================================================================
class Panel : public Control {
public:
    Panel(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        EnsurePanelClass();
        Handle = CreateWindowExW(0, L"M2GOD_PANEL_GEN", text.c_str(),
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
    }
    class BorderStyleProp {
        Control* c;
    public:
        explicit BorderStyleProp(Control* o) : c(o) {}
        BorderStyleProp& operator=(M2God::BorderStyle b) {
            if (!c->Handle) return *this;
            switch (b) {
            case M2God::BorderStyle::None:        RestyleWindow(c->Handle, 0, WS_BORDER, 0, WS_EX_CLIENTEDGE); break;
            case M2God::BorderStyle::FixedSingle: RestyleWindow(c->Handle, WS_BORDER, 0, 0, WS_EX_CLIENTEDGE); break;
            case M2God::BorderStyle::Fixed3D:     RestyleWindow(c->Handle, 0, WS_BORDER, WS_EX_CLIENTEDGE, 0); break;
            }
            return *this;
        }
    } BorderStyle{ this };
    bool AutoScroll = false;   // Win32'de otomatik kaydirma yok; bilgi amacli
};

// PictureBox — ozel pencere sinifi: Load/SizeMode/Image (PNG/JPG/GIF destekli)
// ---------------------------------------------------------------------------
//  GDI+ DINAMIK yukleyici: gdiplus.dll her Windows'ta hazirdir. Ek olarak
//  HICbir .lib veya header GEREKMEZ — VS2019 ve mingw'de oldugu gibi derlenir.
// ---------------------------------------------------------------------------
struct M2GpApi {
    void* mod = nullptr;
    ULONG_PTR token = 0;
    bool started = false;
    int (WINAPI* Startup)(ULONG_PTR*, void*, void*) = nullptr;
    void (WINAPI* Shutdown)(ULONG_PTR) = nullptr;
    int (WINAPI* LoadImageFromFile)(const wchar_t*, void**) = nullptr;
    int (WINAPI* DisposeImage)(void*) = nullptr;
    int (WINAPI* CloneImage)(void*, void**) = nullptr;
    int (WINAPI* GetImageWidth)(void*, unsigned int*) = nullptr;
    int (WINAPI* GetImageHeight)(void*, unsigned int*) = nullptr;
    int (WINAPI* CreateFromHDC)(HDC, void**) = nullptr;
    int (WINAPI* DeleteGraphics)(void*) = nullptr;
    int (WINAPI* DrawImageRect)(void*, void*, float, float, float, float) = nullptr;
    int (WINAPI* SetInterpolationMode)(void*, int) = nullptr;

    bool FunctionsOk() const {
        return mod && Startup && LoadImageFromFile && CreateFromHDC
            && DrawImageRect && GetImageWidth && GetImageHeight;
    }
    bool Ensure() {
        if (!mod) return false;
        if (!started) {
            // GdiplusStartupInput ile ayni yerlesim (UINT32;void*;BOOL;BOOL)
            struct In { unsigned int ver; void* dbg; int supbg; int supext; } in;
            in.ver = 1; in.dbg = nullptr; in.supbg = 0; in.supext = 0;
            started = (Startup(&token, &in, nullptr) == 0 /*Ok*/);
        }
        return started;
    }
};
inline M2GpApi& M2Gp() {
    static M2GpApi api;
    if (!api.mod) {
        api.mod = (void*)LoadLibraryW(L"gdiplus.dll");
        if (api.mod) {
            HMODULE m = (HMODULE)api.mod;
#define M2GPLOAD(fld, name) api.fld = (decltype(api.fld))(void*)GetProcAddress(m, name)
            M2GPLOAD(Startup,               "GdiplusStartup");
            M2GPLOAD(Shutdown,              "GdiplusShutdown");
            M2GPLOAD(LoadImageFromFile,     "GdipLoadImageFromFile");
            M2GPLOAD(DisposeImage,          "GdipDisposeImage");
            M2GPLOAD(CloneImage,            "GdipCloneImage");
            M2GPLOAD(GetImageWidth,         "GdipGetImageWidth");
            M2GPLOAD(GetImageHeight,        "GdipGetImageHeight");
            M2GPLOAD(CreateFromHDC,         "GdipCreateFromHDC");
            M2GPLOAD(DeleteGraphics,        "GdipDeleteGraphics");
            M2GPLOAD(DrawImageRect,        "GdipDrawImageRect");
            M2GPLOAD(SetInterpolationMode,  "GdipSetInterpolationMode");
#undef M2GPLOAD
        }
    }
    return api;
}
inline bool M2GdiplusEnsure() {
    M2GpApi& g = M2Gp();
    return g.FunctionsOk() && g.Ensure();
}

// ---------------------------------------------------------------------------
//  M2God::Image — dosyadan resim (PNG/JPG/GIF/BMP/TIFF/ICO/EMF/WMF)
//      pictureBox1->Image = M2God::Image::FromFile(L"resim.png");
//      if (img) { int w = img.Width(); int h = img.Height(); }
// ---------------------------------------------------------------------------
class Image {
    void* impl = nullptr;   // GpImage*
public:
    Image() {}
    explicit Image(void* p) : impl(p) {}
    Image(const Image& o) : impl(nullptr) {
        if (o.impl && M2Gp().CloneImage) {
            void* c = nullptr;
            if (M2Gp().CloneImage(o.impl, &c) == 0 /*Ok*/ && c) impl = c;
        }
    }
    Image& operator=(const Image& o) {
        if (this != &o) {
            Dispose();
            if (o.impl && M2Gp().CloneImage) {
                void* c = nullptr;
                if (M2Gp().CloneImage(o.impl, &c) == 0 /*Ok*/ && c) impl = c;
            }
        }
        return *this;
    }
    Image(Image&& o) : impl(o.impl) { o.impl = nullptr; }
    Image& operator=(Image&& o) {
        if (this != &o) { Dispose(); impl = o.impl; o.impl = nullptr; }
        return *this;
    }
    ~Image() { Dispose(); }

    static Image FromFile(const std::wstring& path) {
        Image r;
        if (M2GdiplusEnsure() && M2Gp().LoadImageFromFile) {
            void* p = nullptr;
            if (M2Gp().LoadImageFromFile(path.c_str(), &p) == 0 /*Ok*/ && p) r.impl = p;
            else if (p && M2Gp().DisposeImage) M2Gp().DisposeImage(p);
        }
        return r;
    }
    bool IsValid() const { return impl != nullptr; }
    operator bool() const { return impl != nullptr; }
    int Width()  const { unsigned int v = 0; if (impl && M2Gp().GetImageWidth)  M2Gp().GetImageWidth(impl, &v);  return (int)v; }
    int Height() const { unsigned int v = 0; if (impl && M2Gp().GetImageHeight) M2Gp().GetImageHeight(impl, &v); return (int)v; }
    void* Native() const { return impl; }
    void  Steal() { impl = nullptr; }
    void  Dispose() { if (impl) { if (M2Gp().DisposeImage) M2Gp().DisposeImage(impl); impl = nullptr; } }
};

// ---------------------------------------------------------------------------
//  PictureBox — resim kutusu
//      pictureBox1->Image = M2God::Image::FromFile(L"resim.png");
//      pictureBox1->Image = (HBITMAP)LoadImageW(...);            // eski BMP yolu
//      pictureBox1->Load(L"resim.jpg");  pictureBox1->ImageLocation = L"x.png";
//      pictureBox1->SizeMode = M2God::PictureBoxSizeMode::Zoom;
// ---------------------------------------------------------------------------
struct PicState {
    HBITMAP bmp = nullptr;      // GDI yolu (LoadImage ile yuklenen BMP)
    void* gimg = nullptr;       // GDI+ yolu (GpImage* — PNG/JPG/GIF/...)
    int sm = 0;                 // M2God::PictureBoxSizeMode
    std::wstring loc;
};
inline std::map<HWND, PicState>& PicTable() {
    static std::map<HWND, PicState> tbl;
    return tbl;
}
inline void M2PicNoImage(HDC dc, const RECT& rc) {
    // resim-yok isareti (capraz cizgiler)
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
    HPEN op = (HPEN)SelectObject(dc, pen);
    MoveToEx(dc, rc.left, rc.top, nullptr); LineTo(dc, rc.right, rc.bottom);
    MoveToEx(dc, rc.right - 1, rc.top, nullptr); LineTo(dc, rc.left, rc.bottom - 1);
    SelectObject(dc, op); DeleteObject(pen);
}
inline void M2DrawPic(HWND h, HDC dc, RECT rc, PicState& ps) {
    COLORREF back = RGB(240, 240, 240);
    HBRUSH br = CreateSolidBrush(back); FillRect(dc, &rc, br); DeleteObject(br);
    int iw = 0, ih = 0;
    if (ps.gimg) {
        unsigned int w = 0, hh = 0;
        if (M2Gp().GetImageWidth)  M2Gp().GetImageWidth(ps.gimg, &w);
        if (M2Gp().GetImageHeight) M2Gp().GetImageHeight(ps.gimg, &hh);
        iw = (int)w; ih = (int)hh;
    } else if (ps.bmp) {
        BITMAP bm;
        if (GetObjectW(ps.bmp, sizeof(bm), &bm)) { iw = bm.bmWidth; ih = bm.bmHeight; }
    }
    if (iw <= 0 || ih <= 0) { M2PicNoImage(dc, rc); return; }
    int cw = rc.right - rc.left, ch = rc.bottom - rc.top;
    int dx = 0, dy = 0, dw = iw, dh = ih;
    bool stretch = false;
    switch (ps.sm) {
    case 1:  // StretchImage — kutuyu tamamen doldur (en-boy bozulabilir)
        dw = cw; dh = ch; stretch = true; break;
    case 2:  // AutoSize — kutuyu resme gore buyut
        if (h) SetWindowPos(h, nullptr, 0, 0, iw, ih, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    case 3: { // CenterImage — ortala (tasma kirpilir)
        dx = (cw - iw) / 2 < 0 ? 0 : (cw - iw) / 2;
        dy = (ch - ih) / 2 < 0 ? 0 : (ch - ih) / 2;
        break; }
    case 4: { // Zoom — en-boy koru, ortala
        double sc = (double)cw / iw; if ((double)ih * sc > ch) sc = (double)ch / ih;
        dw = (int)(iw * sc); dh = (int)(ih * sc);
        dx = (cw - dw) / 2; dy = (ch - dh) / 2;
        stretch = true; break; }
    default:  // Normal — sol ust, dogal boyut
        break;
    }
    if (ps.gimg) {
        void* gfx = nullptr;
        if (M2Gp().CreateFromHDC && M2Gp().CreateFromHDC(dc, &gfx) == 0 /*Ok*/ && gfx) {
            if (stretch && M2Gp().SetInterpolationMode) M2Gp().SetInterpolationMode(gfx, 7 /*HighQualityBicubic*/);
            if (M2Gp().DrawImageRect) M2Gp().DrawImageRect(gfx, ps.gimg, (float)dx, (float)dy, (float)dw, (float)dh);
            if (M2Gp().DeleteGraphics) M2Gp().DeleteGraphics(gfx);
        }
        return;
    }
    HDC mdc = CreateCompatibleDC(dc);
    HGDIOBJ old = SelectObject(mdc, ps.bmp);
    if (stretch) {
        SetStretchBltMode(dc, HALFTONE);
        StretchBlt(dc, dx, dy, dw, dh, mdc, 0, 0, iw, ih, SRCCOPY);
    } else {
        BitBlt(dc, dx, dy, dw, dh, mdc, 0, 0, SRCCOPY);
    }
    SelectObject(mdc, old);
    DeleteDC(mdc);
}
inline LRESULT CALLBACK M2PicProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        std::map<HWND, PicState>::iterator it = PicTable().find(h);
        if (it != PicTable().end()) M2DrawPic(h, dc, rc, it->second);
        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_NCDESTROY) {   // resimleri serbest birak
        std::map<HWND, PicState>::iterator it = PicTable().find(h);
        if (it != PicTable().end()) {
            if (it->second.gimg && M2Gp().DisposeImage) M2Gp().DisposeImage(it->second.gimg);
            if (it->second.bmp) DeleteObject(it->second.bmp);
            PicTable().erase(it);
        }
    }
    return DefWindowProcW(h, m, w, l);
}
inline void EnsurePicClass() {
    static bool done = false; if (done) return; done = true;
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = M2PicProc;
    wc.hInstance   = GetModuleHandleW(nullptr);
    wc.hCursor     = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"M2GOD_PIC_GEN";
    RegisterClassW(&wc);
}
class PictureBox : public Control {
public:
    PictureBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        EnsurePicClass();
        Handle = CreateWindowExW(0, L"M2GOD_PIC_GEN", text.c_str(),
            SS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_BORDER, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) PicTable()[Handle] = PicState();
    }
    // pictureBox1->SizeMode = M2God::PictureBoxSizeMode::StretchImage;
    class SizeModeProp {
        PictureBox* c;
    public:
        explicit SizeModeProp(PictureBox* o) : c(o) {}
        SizeModeProp& operator=(M2God::PictureBoxSizeMode v) {
            if (c->Handle) { PicTable()[c->Handle].sm = (int)v; InvalidateRect(c->Handle, nullptr, TRUE); }
            return *this;
        }
        operator int() const { return c->Handle ? PicTable()[c->Handle].sm : 0; }
    } SizeMode{ this };
    // pictureBox1->Image = M2God::Image::FromFile(L"resim.png");
    // pictureBox1->Image = (HBITMAP)LoadImageW(...);   // eski BMP yolu
    class ImageProp {
        PictureBox* c;
    public:
        explicit ImageProp(PictureBox* o) : c(o) {}
        ImageProp& operator=(M2God::Image img) {   // degerle al (gecici/kopya -> sahiplen)
            if (c->Handle) {
                PicState& st = PicTable()[c->Handle];
                if (st.gimg && M2Gp().DisposeImage) M2Gp().DisposeImage(st.gimg);
                st.gimg = img.Native();
                img.Steal();
                st.bmp = nullptr;
                InvalidateRect(c->Handle, nullptr, TRUE);
            }
            return *this;
        }
        ImageProp& operator=(HBITMAP bmp) {
            if (c->Handle) {
                PicState& st = PicTable()[c->Handle];
                if (st.gimg && M2Gp().DisposeImage) M2Gp().DisposeImage(st.gimg);
                st.gimg = nullptr;
                st.bmp = bmp;
                InvalidateRect(c->Handle, nullptr, TRUE);
            }
            return *this;
        }
        operator HBITMAP() const { return c->Handle ? PicTable()[c->Handle].bmp : nullptr; }
    } Image{ this };
    class ImageLocationProp {
        PictureBox* c;
    public:
        explicit ImageLocationProp(PictureBox* o) : c(o) {}
        ImageLocationProp& operator=(const std::wstring& path) {
            if (c->Handle) { PicTable()[c->Handle].loc = path; c->Load(path); }
            return *this;
        }
        operator std::wstring() const { return c->Handle ? PicTable()[c->Handle].loc : std::wstring(); }
    } ImageLocation{ this };
    // pictureBox1->Load(L"resim.png");  -> PNG/JPG/GIF/TIFF/BMP yukler, true = basarili
    bool Load(const std::wstring& path) {
        if (!Handle) return false;
        PicState& st = PicTable()[Handle];
        if (M2GdiplusEnsure() && M2Gp().LoadImageFromFile) {
            void* p = nullptr;
            if (M2Gp().LoadImageFromFile(path.c_str(), &p) == 0 /*Ok*/ && p) {
                if (st.gimg && M2Gp().DisposeImage) M2Gp().DisposeImage(st.gimg);
                st.gimg = p; st.bmp = nullptr; st.loc = path;
                InvalidateRect(Handle, nullptr, TRUE);
                return true;
            }
            if (p && M2Gp().DisposeImage) M2Gp().DisposeImage(p);
        }
        HBITMAP bmp = (HBITMAP)LoadImageW(nullptr, path.c_str(), IMAGE_BITMAP, 0, 0,
            LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (bmp) {
            if (st.gimg && M2Gp().DisposeImage) M2Gp().DisposeImage(st.gimg);
            st.gimg = nullptr; st.bmp = bmp; st.loc = path;
            InvalidateRect(Handle, nullptr, TRUE);
            return true;
        }
        return false;
    }
};

// ============================================================================
//  Timer — gercek Win32 zamanlayici (SetTimer/KillTimer, WM_TIMER dispatch)
// ============================================================================
class Timer {
    bool _enabled = false;
public:
    HWND Owner  = NULL;   // sahibi olan form penceresi
    UINT Id     = 0;      // WM_TIMER wParam
    UINT IntervalMs = 1000;
    std::wstring Tag;

    Timer(HWND owner, UINT id) : Owner(owner), Id(id) {}
    virtual ~Timer() { Stop(); }

    void Start() {
        if (!Owner) return;
        if (_enabled) KillTimer(Owner, Id);
        SetTimer(Owner, Id, IntervalMs, nullptr);
        _enabled = true;
    }
    void Stop() { if (Owner) KillTimer(Owner, Id); _enabled = false; }

    // timer1->Enabled = false;  -> Stop()
    // timer1->Enabled = true;   -> Start()
    class EnabledProp {
        Timer* t;
    public:
        explicit EnabledProp(Timer* o) : t(o) {}
        EnabledProp& operator=(bool v) { if (v) t->Start(); else t->Stop(); return *this; }
        operator bool() const { return t->_enabled; }
    } Enabled{ this };

    // timer1->Interval = 500; (calisir durumda aninda yeniden ayarlanir)
    class IntervalProp {
        Timer* t;
    public:
        explicit IntervalProp(Timer* o) : t(o) {}
        IntervalProp& operator=(UINT ms) {
            t->IntervalMs = ms;
            if (t->_enabled && t->Owner) { KillTimer(t->Owner, t->Id); SetTimer(t->Owner, t->Id, ms, nullptr); }
            return *this;
        }
        operator UINT() const { return t->IntervalMs; }
    } Interval{ this };
};

// ============================================================================
//  Bilesenler (gorsel olmayan) — NotifyIcon, ToolTip, BackgroundWorker, ...
// ============================================================================
static UINT M2NextIconId() { static UINT id = 1; return id++; }

class NotifyIcon {
public:
    HWND Owner = NULL;
    UINT Id = 0;
    NOTIFYICONDATAW nid = { 0 };
    bool Added = false;
    std::wstring BalloonTipTitle, BalloonTipText;
    std::function<void()> Click, DoubleClick;

    NotifyIcon(HWND owner) : Owner(owner) {
        Id = M2NextIconId();
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = owner;
        nid.uID = Id;
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage = WM_APP + 0x4E1;
        nid.hIcon = LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
        nid.szTip[0] = 0;
    }
    ~NotifyIcon() { Visible_Set(false); }
    class IconProp {
        NotifyIcon* c;
    public:
        explicit IconProp(NotifyIcon* o) : c(o) {}
        IconProp& operator=(HICON h) {
            if (c->nid.hIcon && c->nid.hIcon != LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION)) DestroyIcon(c->nid.hIcon);
            c->nid.hIcon = h;
            if (c->Added) Shell_NotifyIconW(NIM_MODIFY, &c->nid);
            return *this;
        }
    } Icon{ this };
    class TextProp {
        NotifyIcon* c;
    public:
        explicit TextProp(NotifyIcon* o) : c(o) {}
        TextProp& operator=(const std::wstring& s) {
            M2StrNCpy(c->nid.szTip, s.c_str(), 63);
            c->nid.szTip[63] = 0;
            if (c->Added) Shell_NotifyIconW(NIM_MODIFY, &c->nid);
            return *this;
        }
        operator std::wstring() const { return c->nid.szTip; }
    } Text{ this };
    void Visible_Set(bool v) {
        if (v && !Added) { Shell_NotifyIconW(NIM_ADD, &nid); Added = true; }
        else if (!v && Added) { Shell_NotifyIconW(NIM_DELETE, &nid); Added = false; }
    }
    class VisibleProp {
        NotifyIcon* c;
    public:
        explicit VisibleProp(NotifyIcon* o) : c(o) {}
        VisibleProp& operator=(bool v) { c->Visible_Set(v); return *this; }
        operator bool() const { return c->Added; }
    } Visible{ this };
    void ShowBalloonTip(int timeoutMs = 500) {
        nid.uFlags |= NIF_INFO;
        M2StrNCpy(nid.szInfo, BalloonTipText.c_str(), 255); nid.szInfo[255] = 0;
        M2StrNCpy(nid.szInfoTitle, BalloonTipTitle.c_str(), 63); nid.szInfoTitle[63] = 0;
        nid.uTimeout = (UINT)timeoutMs;
        nid.dwInfoFlags = NIIF_INFO;
        Shell_NotifyIconW(NIM_MODIFY, &nid);
        nid.uFlags &= ~NIF_INFO;
    }
    // uretilen WndProc WM_APP+0x4E1'de cagirir
    void Dispatch(LPARAM lParam) {
        if (lParam == WM_LBUTTONUP && Click) Click();
        else if (lParam == WM_LBUTTONDBLCLK && DoubleClick) DoubleClick();
    }
};

// ---------------------------------------------------------------------------
class ToolTip {
public:
    HWND hTip = NULL;
    HWND Owner = NULL;
    ToolTip(HWND owner) : Owner(owner) {
        hTip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            owner, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (hTip) {
            TOOLINFOW ti = { 0 };
            ti.cbSize = sizeof(ti);
            ti.hwnd = owner;
            SendMessageW(hTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
        }
    }
    ~ToolTip() { if (hTip) DestroyWindow(hTip); }
    // toolTip1->SetToolTip(button1, L"Tikla");
    void SetToolTip(HWND ctrl, const std::wstring& text) {
        if (!hTip || !ctrl) return;
        TOOLINFOW ti = { 0 };
        ti.cbSize = sizeof(ti);
        ti.hwnd = ctrl;
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
        ti.uId = (UINT_PTR)ctrl;
        ti.lpszText = (LPWSTR)text.c_str();
        SendMessageW(hTip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
        SendMessageW(hTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }
    void SetToolTip(Control* c, const std::wstring& text) { SetToolTip(c->Handle, text); }
    class ActiveProp {
        ToolTip* c;
    public:
        explicit ActiveProp(ToolTip* o) : c(o) {}
        ActiveProp& operator=(bool v) { if (c->hTip) SendMessageW(c->hTip, TTM_ACTIVATE, v ? TRUE : FALSE, 0); return *this; }
    } Active{ this };
    class InitialDelayProp {
        ToolTip* c;
    public:
        explicit InitialDelayProp(ToolTip* o) : c(o) {}
        InitialDelayProp& operator=(int ms) { if (c->hTip) SendMessageW(c->hTip, TTM_SETDELAYTIME, (WPARAM)TTDT_INITIAL, (LPARAM)ms); return *this; }
    } InitialDelay{ this };
    class AutoPopDelayProp {
        ToolTip* c;
    public:
        explicit AutoPopDelayProp(ToolTip* o) : c(o) {}
        AutoPopDelayProp& operator=(int ms) { if (c->hTip) SendMessageW(c->hTip, TTM_SETDELAYTIME, (WPARAM)TTDT_AUTOPOP, (LPARAM)ms); return *this; }
    } AutoPopDelay{ this };
    class ReshowDelayProp {
        ToolTip* c;
    public:
        explicit ReshowDelayProp(ToolTip* o) : c(o) {}
        ReshowDelayProp& operator=(int ms) { if (c->hTip) SendMessageW(c->hTip, TTM_SETDELAYTIME, (WPARAM)TTDT_RESHOW, (LPARAM)ms); return *this; }
    } ReshowDelay{ this };
    class ToolTipTitleProp {
        ToolTip* c;
    public:
        explicit ToolTipTitleProp(ToolTip* o) : c(o) {}
        ToolTipTitleProp& operator=(const std::wstring& t) {
            if (c->hTip) SendMessageW(c->hTip, TTM_SETTITLEW, (WPARAM)1, (LPARAM)t.c_str());
            return *this;
        }
    } ToolTipTitle{ this };
};

// ---------------------------------------------------------------------------
class BackgroundWorker {
public:
    HWND Owner = NULL;
    bool WorkerReportsProgress = false;
    bool WorkerSupportsCancellation = false;
    std::function<void()> DoWork;             // arka plan is parcasinin gövdesi
    std::function<void(int)> ProgressChanged; // ReportProgress ile tetiklenir
    std::function<void()> RunWorkerCompleted;
    volatile bool CancellationPendingFlag = false;
    volatile bool IsBusyFlag = false;

    BackgroundWorker() {}
    void RunWorkerAsync() {
        if (IsBusyFlag || !DoWork) return;
        CancellationPendingFlag = false;
        IsBusyFlag = true;
        struct Ctx { BackgroundWorker* w; std::function<void()> work; };
        Ctx* ctx = new Ctx{ this, DoWork };
        HANDLE h = CreateThread(nullptr, 0, &BackgroundWorker::M2ThreadProc, ctx, 0, nullptr);
        if (h) CloseHandle(h);
    }
    static DWORD WINAPI M2ThreadProc(LPVOID p) {
        struct Ctx { BackgroundWorker* w; std::function<void()> work; };
        Ctx* ctx = (Ctx*)p;
        ctx->work();
        ctx->w->IsBusyFlag = false;
        if (ctx->w->Owner) PostMessageW(ctx->w->Owner, WM_APP + 0x4E3, 0, 0);
        delete ctx;
        return 0;
    }
    void CancelAsync() { CancellationPendingFlag = true; }
    bool CancellationPending() const { return CancellationPendingFlag; }
    bool IsBusy() const { return IsBusyFlag; }
    void ReportProgress(int percent) {
        if (Owner) PostMessageW(Owner, WM_APP + 0x4E2, (WPARAM)percent, 0);
    }
    // uretilen WndProc icinden:
    void DispatchProgress(UINT percent) {
        if (WorkerReportsProgress && ProgressChanged) ProgressChanged((int)percent);
    }
    void DispatchCompleted() { if (RunWorkerCompleted) RunWorkerCompleted(); }
};

// ---------------------------------------------------------------------------
class Process {
public:
    std::wstring FileName, Arguments, WorkingDirectory;
    HANDLE hProc = nullptr;

    Process() {}
    // Process::Start(L"notepad.exe") — statik kullanim
    static int Start(const std::wstring& file, const std::wstring& args = L"") {
        std::wstring cmd = L"\"" + file + L"\"";
        if (!args.empty()) cmd += L" " + args;
        STARTUPINFOW si = { 0 }; si.cb = sizeof(si);
        PROCESS_INFORMATION pi = { 0 };
        std::vector<wchar_t> buf(cmd.begin(), cmd.end());
        buf.push_back(0);
        if (CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return (int)pi.dwProcessId;
        }
        return 0;
    }
    int Start() {
        std::wstring cmd = FileName.empty() ? L"" : L"\"" + FileName + L"\"";
        if (!Arguments.empty()) cmd += L" " + Arguments;
        STARTUPINFOW si = { 0 }; si.cb = sizeof(si);
        PROCESS_INFORMATION pi = { 0 };
        std::vector<wchar_t> buf(cmd.begin(), cmd.end());
        buf.push_back(0);
        if (CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr,
            WorkingDirectory.empty() ? nullptr : WorkingDirectory.c_str(), &si, &pi)) {
            CloseHandle(pi.hThread);
            hProc = pi.hProcess;
            return (int)pi.dwProcessId;
        }
        return 0;
    }
    bool WaitForExit(int timeoutMs = -1) {
        if (!hProc) return false;
        DWORD r = WaitForSingleObject(hProc, timeoutMs < 0 ? INFINITE : (DWORD)timeoutMs);
        return r == WAIT_OBJECT_0;
    }
    bool HasExited() {
        if (!hProc) return true;
        DWORD code = 0;
        if (GetExitCodeProcess(hProc, &code)) return code != STILL_ACTIVE;
        return false;
    }
    int ExitCode() {
        if (!hProc) return -1;
        DWORD code = 0;
        GetExitCodeProcess(hProc, &code);
        return (int)code;
    }
    void Kill() {
        if (hProc) { TerminateProcess(hProc, 1); CloseHandle(hProc); hProc = nullptr; }
    }
    ~Process() { if (hProc) CloseHandle(hProc); }
};

// ---------------------------------------------------------------------------
class SerialPort {
public:
    std::wstring PortName = L"COM1";
    int BaudRate = 9600;
    int DataBits = 8;
    std::wstring Parity = L"None";     // None/Even/Odd
    std::wstring StopBits = L"One";
    HANDLE hPort = nullptr;

    SerialPort() {}
    ~SerialPort() { Close(); }
    bool IsOpen() const { return hPort && hPort != INVALID_HANDLE_VALUE; }
    bool Open() {
        if (IsOpen()) return true;
        std::wstring name = (PortName.rfind(L"\\\\.\\", 0) == 0) ? PortName : L"\\\\.\\" + PortName;
        hPort = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hPort == INVALID_HANDLE_VALUE) { hPort = nullptr; return false; }
        std::wstring dcbStr = L"baud=" + std::to_wstring(BaudRate) + L" parity=" +
            (Parity == L"Even" ? L"E" : Parity == L"Odd" ? L"O" : L"N") +
            L" data=" + std::to_wstring(DataBits) +
            L" stop=" + (StopBits == L"Two" ? L"2" : StopBits == L"OnePointFive" ? L"1" : L"1");
        DCB dcb = { 0 }; dcb.DCBlength = sizeof(dcb);
        if (!BuildCommDCBW(dcbStr.c_str(), &dcb)) { CloseHandle(hPort); hPort = nullptr; return false; }
        if (!SetCommState(hPort, &dcb)) { CloseHandle(hPort); hPort = nullptr; return false; }
        COMMTIMEOUTS to = { 0 };
        to.ReadIntervalTimeout = 50;
        to.ReadTotalTimeoutConstant = 50;
        to.ReadTotalTimeoutMultiplier = 10;
        to.WriteTotalTimeoutConstant = 50;
        to.WriteTotalTimeoutMultiplier = 10;
        SetCommTimeouts(hPort, &to);
        return true;
    }
    void Close() { if (hPort) { CloseHandle(hPort); hPort = nullptr; } }
    bool Write(const std::wstring& s) {
        if (!IsOpen()) return false;
        // UTF-16'yi ANSI'ye cevir (cihazlar genellikle ASCII ister)
        int n = WideCharToMultiByte(CP_ACP, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
        std::vector<char> buf((size_t)n);
        WideCharToMultiByte(CP_ACP, 0, s.c_str(), (int)s.size(), buf.data(), n, nullptr, nullptr);
        DWORD wr = 0;
        return WriteFile(hPort, buf.data(), (DWORD)buf.size(), &wr, nullptr) != 0;
    }
    bool WriteLine(const std::wstring& s) { return Write(s + L"\r\n"); }
    std::wstring ReadExisting() {
        if (!IsOpen()) return L"";
        DWORD errors = 0;
        COMSTAT cs = { 0 };
        if (!ClearCommError(hPort, &errors, &cs) || cs.cbInQue == 0) return L"";
        std::vector<char> buf((size_t)cs.cbInQue, 0);
        DWORD rd = 0;
        if (!ReadFile(hPort, buf.data(), (DWORD)buf.size(), &rd, nullptr) || rd == 0) return L"";
        int wl = MultiByteToWideChar(CP_ACP, 0, buf.data(), (int)rd, nullptr, 0);
        std::wstring out((size_t)(wl > 0 ? wl : 0), 0);
        if (wl > 0) MultiByteToWideChar(CP_ACP, 0, buf.data(), (int)rd, &out[0], wl);
        return out;
    }
};

// ============================================================================
//  Diyaloglar — OpenFileDialog / SaveFileDialog / ColorDialog / FontDialog /
//              FolderBrowserDialog  (ShowDialog() -> M2God::DialogResult)
// ============================================================================
class FileDialog {
public:
    std::wstring FileName, Title, InitialDirectory;
    std::wstring Filter = L"Tüm Dosyalar (*.*)|*.*";
    int FilterIndex = 1;
    bool Multiselect = false, RestoreDirectory = false, CheckFileExists = true;
    std::vector<std::wstring> FileNames;
    std::vector<wchar_t> bufFile;

    void PrepBuf() {
        bufFile.assign(32768, 0);
        if (!FileName.empty()) M2StrNCpy(bufFile.data(), FileName.c_str(), 32767);
    }
    void PrepFilter(std::vector<wchar_t>& f) {
        f.assign(Filter.begin(), Filter.end());
        f.push_back(0);
        for (auto& ch : f) if (ch == L'|') ch = 0;
        f.push_back(0); f.push_back(0);
    }
    void CollectFiles(const OPENFILENAMEW& ofn) {
        FileNames.clear();
        wchar_t* p = ofn.lpstrFile;
        if (ofn.Flags & OFN_ALLOWMULTISELECT) {
            std::wstring dir(p);
            size_t dl = dir.size();
            p += dl + 1;
            while (*p) { FileNames.push_back(dir + L"\\" + p); p += wcslen(p) + 1; }
            if (FileNames.empty()) FileNames.push_back(dir);
        }
        FileName = ofn.lpstrFile;
        if (FileNames.empty()) FileNames.push_back(FileName);
    }
};

class OpenFileDialog : public FileDialog {
public:
    M2God::DialogResult ShowDialog() {
        PrepBuf();
        std::vector<wchar_t> f; PrepFilter(f);
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = Form::Handle;
        ofn.lpstrFile = bufFile.data();
        ofn.nMaxFile = (DWORD)bufFile.size();
        ofn.lpstrFilter = f.data();
        ofn.nFilterIndex = (DWORD)FilterIndex;
        ofn.lpstrTitle = Title.empty() ? nullptr : Title.c_str();
        ofn.lpstrInitialDir = InitialDirectory.empty() ? nullptr : InitialDirectory.c_str();
        ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | (Multiselect ? OFN_ALLOWMULTISELECT : 0) |
                    (RestoreDirectory ? OFN_NOCHANGEDIR : 0) |
                    (CheckFileExists ? OFN_FILEMUSTEXIST : 0);
        if (!GetOpenFileNameW(&ofn)) return M2God::DialogResult::Cancel;
        CollectFiles(ofn);
        return M2God::DialogResult::OK;
    }
};

class SaveFileDialog : public FileDialog {
public:
    bool OverwritePrompt = true, CreatePrompt = false;
    std::wstring DefaultExt;
    M2God::DialogResult ShowDialog() {
        PrepBuf();
        std::vector<wchar_t> f; PrepFilter(f);
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = Form::Handle;
        ofn.lpstrFile = bufFile.data();
        ofn.nMaxFile = (DWORD)bufFile.size();
        ofn.lpstrFilter = f.data();
        ofn.nFilterIndex = (DWORD)FilterIndex;
        ofn.lpstrTitle = Title.empty() ? nullptr : Title.c_str();
        ofn.lpstrInitialDir = InitialDirectory.empty() ? nullptr : InitialDirectory.c_str();
        ofn.lpstrDefExt = DefaultExt.empty() ? nullptr : DefaultExt.c_str();
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST |
                    (OverwritePrompt ? OFN_OVERWRITEPROMPT : 0) |
                    (CreatePrompt ? OFN_CREATEPROMPT : 0) |
                    (RestoreDirectory ? OFN_NOCHANGEDIR : 0);
        if (!GetSaveFileNameW(&ofn)) return M2God::DialogResult::Cancel;
        CollectFiles(ofn);
        return M2God::DialogResult::OK;
    }
};

class ColorDialog {
public:
    COLORREF Color = RGB(0, 0, 0);
    bool FullOpen = false, AnyColor = true, SolidColorOnly = false;
    M2God::DialogResult ShowDialog() {
        CHOOSECOLORW cc = { 0 };
        static COLORREF custom[16] = { 0 };
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = Form::Handle;
        cc.rgbResult = Color;
        cc.lpCustColors = custom;
        cc.Flags = CC_FULLOPEN | CC_ANYCOLOR | (FullOpen ? CC_FULLOPEN : 0) |
                   (AnyColor ? CC_ANYCOLOR : 0) | (SolidColorOnly ? CC_SOLIDCOLOR : 0);
        if (!ChooseColorW(&cc)) return M2God::DialogResult::Cancel;
        Color = cc.rgbResult;
        return M2God::DialogResult::OK;
    }
};

class FontDialog {
public:
    HFONT Font = nullptr;
    std::wstring FontName_ = L"Segoe UI";
    int FontSize_ = 9;
    bool Bold = false, Italic = false, Underline = false, Strikeout = false;
    COLORREF Color = RGB(0, 0, 0);
    M2God::DialogResult ShowDialog() {
        LOGFONTW lf = { 0 };
        HDC dc = GetDC(nullptr);
        lf.lfHeight = -MulDiv(FontSize_, GetDeviceCaps(dc, LOGPIXELSY), 72);
        ReleaseDC(nullptr, dc);
        M2StrNCpy(lf.lfFaceName, FontName_.c_str(), 31);
        lf.lfWeight = Bold ? FW_BOLD : FW_NORMAL;
        lf.lfItalic = Italic ? TRUE : FALSE;
        lf.lfUnderline = Underline ? TRUE : FALSE;
        lf.lfStrikeOut = Strikeout ? TRUE : FALSE;
        CHOOSEFONTW cf = { 0 };
        cf.lStructSize = sizeof(cf);
        cf.hwndOwner = Form::Handle;
        cf.lpLogFont = &lf;
        cf.rgbColors = Color;
        cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_SCREENFONTS;
        if (!ChooseFontW(&cf)) return M2God::DialogResult::Cancel;
        FontName_ = lf.lfFaceName;
        Bold = (lf.lfWeight >= FW_BOLD);
        Italic = lf.lfItalic != 0;
        Underline = lf.lfUnderline != 0;
        Strikeout = lf.lfStrikeOut != 0;
        FontSize_ = -MulDiv(lf.lfHeight, 72, GetDeviceCaps(dc, LOGPIXELSY));
        Font = CreateFontIndirectW(&lf);
        Color = cf.rgbColors;
        return M2God::DialogResult::OK;
    }
    std::wstring FontName() const { return FontName_; }
    int FontSize() const { return FontSize_; }
};

class FolderBrowserDialog {
public:
    std::wstring SelectedPath, Description;
    M2God::DialogResult ShowDialog() {
        BROWSEINFOW bi = { 0 };
        wchar_t path[MAX_PATH] = L"";
        bi.hwndOwner = Form::Handle;
        bi.pszDisplayName = path;
        bi.lpszTitle = Description.empty() ? L"Klasör seçin" : Description.c_str();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (!pidl) return M2God::DialogResult::Cancel;
        if (!SHGetPathFromIDListW(pidl, path)) { CoTaskMemFree(pidl); return M2God::DialogResult::Cancel; }
        CoTaskMemFree(pidl);
        SelectedPath = path;
        return M2God::DialogResult::OK;
    }
};

// ============================================================================
//  MsgBox — WinForms tarzi mesaj kutusu
// ============================================================================
inline DialogResult MsgBox(const std::wstring& text,
                           const std::wstring& caption = L"",
                           MessageBoxButtons buttons = MessageBoxButtons::OK,
                           MessageBoxIcon icon = MessageBoxIcon::Information) {
    UINT mb = MB_TOPMOST;
    switch (buttons) {
    case MessageBoxButtons::OKCancel:         mb |= MB_OKCANCEL; break;
    case MessageBoxButtons::AbortRetryIgnore: mb |= MB_ABORTRETRYIGNORE; break;
    case MessageBoxButtons::YesNoCancel:      mb |= MB_YESNOCANCEL; break;
    case MessageBoxButtons::YesNo:            mb |= MB_YESNO; break;
    case MessageBoxButtons::RetryCancel:      mb |= MB_RETRYCANCEL; break;
    default:                                  mb |= MB_OK; break;
    }
    mb |= (UINT)icon;   // 16/32/48/64 == MB_ICONERROR/QUESTION/WARNING/INFORMATION
    int r = MessageBoxW(Form::Handle ? Form::Handle : GetActiveWindow(),
                        text.c_str(), caption.c_str(), mb);
    return (DialogResult)r;   // IDOK(1)/IDCANCEL(2)/IDYES(6)/IDNO(7) ayni degerler
}

// Desteklenmeyen tipler icin yer tutucu kutu
class Placeholder : public Control {
public:
    Placeholder(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", text.c_str(),
            SS_NOTIFY | WS_CHILD | WS_VISIBLE | SS_CENTER, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), MAKELPARAM(TRUE, 0));
    }
};

} // namespace M2God
)M2GODGLOBAL"