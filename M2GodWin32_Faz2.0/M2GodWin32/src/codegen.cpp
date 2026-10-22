// codegen.cpp — kod üretici (Bölüm 1: Form1.h/cpp + Global.h üretimi)
// Faz 2.4: M2God nesne sarmalayıcı kütüphanesi (Global.h) + gerçek Timer
//   - timer1->Enabled = false;  button1->Text = L"Merhaba";  sözdizimi
//   - WM_TIMER gerçek dispatch (SetTimer/KillTimer)
//   - Enabled/Visible/Font/BackColor/ForeColor/Items üretilen koda taşınır
#include "codegen.h"
#include "app.h"
#include "catalog.h"
#include "factory.h"
#include <sstream>

namespace codegen {

std::wstring IdMacroOf(const std::wstring& ctrlName) {
    std::wstring up = ctrlName;
    for (auto& ch : up) {
        if (iswalnum(ch)) ch = (wchar_t)towupper(ch);
        else ch = L'_';
    }
    return L"IDC_" + up;
}

// metin kaçışı: C++ geniş dize sabiti içinde güvenli hale getirir
static std::wstring EscCppStr(const std::wstring& s) {
    std::wstring e;
    for (wchar_t ch : s) {
        if (ch == L'\\') e += L"\\\\";
        else if (ch == L'"') e += L"\\\"";
        else if (ch == L'\n') e += L"\\n";
        else if (ch == L'\r') e += L"\\r";
        else if (ch == L'\t') e += L"\\t";
        else e += ch;
    }
    return e;
}

// tip → Global.h sarmalayıcı sınıf adı
static std::wstring M2TypeOf(const std::wstring& type) {
    struct M { const wchar_t* a; const wchar_t* b; };
    static const M map[] = {
        { L"Button", L"Button" }, { L"CheckBox", L"CheckBox" }, { L"RadioButton", L"RadioButton" },
        { L"GroupBox", L"GroupBox" }, { L"Label", L"Label" }, { L"LinkLabel", L"Label" },
        { L"TextBox", L"TextBox" }, { L"MaskedTextBox", L"TextBox" }, { L"NumericUpDown", L"NumericUpDown" },
        { L"RichTextBox", L"RichTextBox" }, { L"ComboBox", L"ComboBox" },
        { L"ListBox", L"ListBox" }, { L"CheckedListBox", L"ListBox" },
        { L"ListView", L"ListView" }, { L"TreeView", L"TreeView" },
        { L"ProgressBar", L"ProgressBar" }, { L"TrackBar", L"TrackBar" },
        { L"HScrollBar", L"HScrollBar" }, { L"VScrollBar", L"VScrollBar" },
        { L"DateTimePicker", L"DateTimePicker" }, { L"MonthCalendar", L"MonthCalendar" },
        { L"TabControl", L"TabControl" }, { L"StatusStrip", L"StatusStrip" },
        { L"Panel", L"Panel" }, { L"FlowLayoutPanel", L"Panel" }, { L"TableLayoutPanel", L"Panel" },
        { L"SplitContainer", L"Panel" }, { L"PictureBox", L"PictureBox" },
    };
    for (auto& m : map) if (_wcsicmp(m.a, type.c_str()) == 0) return m.b;
    return L"Placeholder";
}

const char* GlobalVersionMark() { return "M2GOD-GLOBAL v2402"; }

// ============================================================================
//  Global.h şablonu — M2God nesne sarmalayıcı kütüphanesi (header-only)
//  Kullanıcı olay kodlarında: timer1->Enabled = false; button1->Text = L"Hi";
// ============================================================================
std::wstring GenerateGlobalH() {
    static const char kTemplate[] = R"M2GODGLOBAL(
// ============================================================================
//  Global.h — M2GodWin32 UI Designer (Faz 2.4) nesne sarmalayıcı kutuphanesi
//  M2GOD-GLOBAL v2402
//  !! BU DOSYAYI DUZENLEMEYIN — tasarimci gerektiginde gunceller.
//  Amac: Form1 olay kodlarinda WinForms benzeri sozdizimi (saf Win32 uzerinde):
//      timer1->Enabled = false;                 // Timer durdurur
//      timer1->Enabled = true;                  // Timer baslatir
//      timer1->Interval = 500;                  // ms
//      button1->Text = L"Merhaba";
//      button1->Enabled = false;  button1->Visible = false;
//      textBox1->Text = button1->Text;
//      comboBox1->Items->Add(L"Secenek");
//      checkBox1->Checked = true;
//  HWND gerekirse: button1->Handle
//  Derleme: VS2019 C++17 — ek kutuphane gerekmez (header-only).
// ============================================================================
#pragma once
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <cstdio>
#include <string>
#include <map>
#include <initializer_list>
#include <type_traits>

namespace M2God {

// dize birleştirme: L"Selim: " + comboBox1->Items->SelectedItem gibi
// property nesneleri de dogal sozdizimiyle birlestirilebilsin (ADL icin M2God'da)
template <typename T, typename = std::enable_if_t<std::is_class_v<T> && std::is_convertible_v<const T&, std::wstring>>>
inline std::wstring operator+(const wchar_t* a, const T& b) {
    return std::wstring(a) + (std::wstring)b;
}

// ---- UI fontu --------------------------------------------------------------
inline HFONT GetUIFont() {
    static HFONT f = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    return f;
}
// Ozel font: M2God::MakeFont(L"Tahoma", 10, true)
inline HFONT MakeFont(const wchar_t* name, int pt, bool bold = false,
                      bool italic = false, bool underline = false) {
    HDC dc = GetDC(nullptr);
    int hgt = -MulDiv(pt > 0 ? pt : 9, GetDeviceCaps(dc, LOGPIXELSY), 72);
    ReleaseDC(nullptr, dc);
    return CreateFontW(hgt, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
        italic ? TRUE : FALSE, underline ? TRUE : FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, name);
}

// ---- renk kayit defteri (BackColor/ForeColor WM_CTLCOLOR* ile uygulanir) ----
struct ColorInfo { COLORREF back = 0xFFFFFFFF; COLORREF fore = 0xFFFFFFFF; };
inline ColorInfo& CtrlColorInfo(HWND h) {
    static std::map<HWND, ColorInfo> tbl;
    return tbl[h];
}
// Form1_WndProc icindeki WM_CTLCOLORSTATIC/WM_CTLCOLOREDIT/... case'lerinden cagrilir
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
            return (LRESULT)b;
        }
        return (LRESULT)it->second;
    }
    SetBkMode(dc, TRANSPARENT);
    return (LRESULT)GetStockObject(HOLLOW_BRUSH);
}

// ---- yardimci sinif kayitlari ----------------------------------------------
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

// ============================================================================
//  Control — tum gorsel kontrollerin temeli (property vekilleri)
// ============================================================================
class Control {
public:
    HWND Handle = NULL;
    int  Id     = 0;

    Control() {}
    Control(HWND h, int id) : Handle(h), Id(id) {}
    virtual ~Control() {}

    // ---- Text ----
    class TextProp {
        Control* c;
    public:
        explicit TextProp(Control* o) : c(o) {}
        TextProp& operator=(const std::wstring& s) { if (c->Handle) SetWindowTextW(c->Handle, s.c_str()); return *this; }
        TextProp& operator=(const wchar_t* s) { return *this = std::wstring(s ? s : L""); }
        operator std::wstring() const {
            wchar_t buf[1024]; int n = c->Handle ? GetWindowTextW(c->Handle, buf, 1024) : 0;
            return std::wstring(buf, n);
        }
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

    // ---- Font (HFONT atanir) ----
    class FontProp {
        Control* c;
    public:
        explicit FontProp(Control* o) : c(o) {}
        FontProp& operator=(HFONT f) { if (c->Handle) SendMessageW(c->Handle, WM_SETFONT, (WPARAM)f, TRUE); return *this; }
    } Font{ this };

    // ---- BackColor / ForeColor ----
    class BackColorProp {
        Control* c;
    public:
        explicit BackColorProp(Control* o) : c(o) {}
        BackColorProp& operator=(COLORREF v) { CtrlColorInfo(c->Handle).back = v; if (c->Handle) InvalidateRect(c->Handle, nullptr, TRUE); return *this; }
        operator COLORREF() const { return CtrlColorInfo(c->Handle).back; }
    } BackColor{ this };
    class ForeColorProp {
        Control* c;
    public:
        explicit ForeColorProp(Control* o) : c(o) {}
        ForeColorProp& operator=(COLORREF v) { CtrlColorInfo(c->Handle).fore = v; if (c->Handle) InvalidateRect(c->Handle, nullptr, TRUE); return *this; }
        operator COLORREF() const { return CtrlColorInfo(c->Handle).fore; }
    } ForeColor{ this };

    // ---- konum/boyut ----
    void SetLocation(int x, int y) { if (Handle) SetWindowPos(Handle, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE); }
    void SetSize(int w, int h)     { if (Handle) SetWindowPos(Handle, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); }
    void Focus()                   { if (Handle) SetFocus(Handle); }
    void Hide() { Visible = false; }
    void Show() { Visible = true; }
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
//  Somut kontroller — kurucular gercek Win32 kontrolunu yaratir
// ============================================================================
class Button : public Control {
public:
    Button(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"BUTTON", text.c_str(),
            BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};

class CheckBox : public Control {
public:
    CheckBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"BUTTON", text.c_str(),
            BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
    // checkBox1->Checked = true;
    class CheckedProp {
        Control* c;
    public:
        explicit CheckedProp(Control* o) : c(o) {}
        CheckedProp& operator=(bool v) { if (c->Handle) SendMessageW(c->Handle, BM_SETCHECK, v ? BST_CHECKED : BST_UNCHECKED, 0); return *this; }
        operator bool() const { return c->Handle && SendMessageW(c->Handle, BM_GETCHECK, 0, 0) == BST_CHECKED; }
    } Checked{ this };
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

class GroupBox : public Control {
public:
    GroupBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"BUTTON", text.c_str(),
            BS_GROUPBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};

class Label : public Control {
public:
    Label(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"STATIC", text.c_str(),
            SS_LEFT | SS_NOTIFY | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};

class TextBox : public Control {
public:
    TextBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h, bool multiline = false) {
        DWORD st = WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;
        if (multiline) st |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_HSCROLL;
        else st |= ES_AUTOHSCROLL;
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text.c_str(), st, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
    // textBox1->ReadOnly = true;
    class ReadOnlyProp {
        Control* c;
    public:
        explicit ReadOnlyProp(Control* o) : c(o) {}
        ReadOnlyProp& operator=(bool v) { if (c->Handle) SendMessageW(c->Handle, EM_SETREADONLY, v ? TRUE : FALSE, 0); return *this; }
        operator bool() const { return c->Handle && (GetWindowLongW(c->Handle, GWL_STYLE) & ES_READONLY) != 0; }
    } ReadOnly{ this };
};

class NumericUpDown : public TextBox {
public:
    NumericUpDown(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h, bool = false)
        : TextBox(parent, id, text, x, y, w, h, false) {
        if (Handle) {
            LONG_PTR st = GetWindowLongPtrW(Handle, GWL_STYLE);
            SetWindowLongPtrW(Handle, GWL_STYLE, (st & ~ES_AUTOHSCROLL) | ES_NUMBER);
        }
    }
    // numericUpDown1->Value = 5;
    class ValueProp {
        Control* c;
    public:
        explicit ValueProp(Control* o) : c(o) {}
        ValueProp& operator=(int v) { if (c->Handle) { wchar_t b[32]; swprintf(b, 32, L"%d", v); SetWindowTextW(c->Handle, b); } return *this; }
        operator int() const { wchar_t b[32] = L""; if (c->Handle) GetWindowTextW(c->Handle, b, 32); return _wtoi(b); }
    } Value{ this };
};

class RichTextBox : public Control {
public:
    RichTextBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        EnsureRichEdit();
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"RichEdit20W", text.c_str(),
            WS_BORDER | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
            x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};

class ComboBox : public Control {
public:
    ComboBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"COMBOBOX", text.c_str(),
            CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
    // comboBox1->Items->Add(L"...");  comboBox1->Items->Clear();
    // comboBox1->Items->SelectedIndex = 0;  comboBox1->Items->Count
    class ItemsProp {
        ComboBox* c;
    public:
        explicit ItemsProp(ComboBox* o) : c(o) {}
        ItemsProp* operator->() { return this; }
        void Clear() { if (c->Handle) SendMessageW(c->Handle, CB_RESETCONTENT, 0, 0); }
        void Add(const std::wstring& s) { if (c->Handle) SendMessageW(c->Handle, CB_ADDSTRING, 0, (LPARAM)s.c_str()); }
        void AddRange(std::initializer_list<const wchar_t*> ls) { for (auto* s : ls) Add(s); }
        void RemoveAt(int i) { if (c->Handle) SendMessageW(c->Handle, CB_DELETESTRING, (WPARAM)i, 0); }
        int  GetCount() const { return c->Handle ? (int)SendMessageW(c->Handle, CB_GETCOUNT, 0, 0) : 0; }
        int  GetSelectedIndex() const { return c->Handle ? (int)SendMessageW(c->Handle, CB_GETCURSEL, 0, 0) : -1; }
        void SetSelectedIndex(int i) { if (c->Handle) SendMessageW(c->Handle, CB_SETCURSEL, (WPARAM)i, 0); }
        std::wstring GetSelectedItem() const {
            if (!c->Handle) return L"";
            int i = GetSelectedIndex(); if (i < 0) return L"";
            wchar_t buf[256] = L"";
            SendMessageW(c->Handle, CB_GETLBTEXT, (WPARAM)i, (LPARAM)buf);
            return buf;
        }
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
};

class ListBox : public Control {
public:
    ListBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"LISTBOX", text.c_str(),
            WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
    // listBox1->Items->Add(L"..."); listBox1->Items->SelectedIndex
    class ItemsProp {
        ListBox* c;
    public:
        explicit ItemsProp(ListBox* o) : c(o) {}
        ItemsProp* operator->() { return this; }
        void Clear() { if (c->Handle) SendMessageW(c->Handle, LB_RESETCONTENT, 0, 0); }
        void Add(const std::wstring& s) { if (c->Handle) SendMessageW(c->Handle, LB_ADDSTRING, 0, (LPARAM)s.c_str()); }
        void AddRange(std::initializer_list<const wchar_t*> ls) { for (auto* s : ls) Add(s); }
        void RemoveAt(int i) { if (c->Handle) SendMessageW(c->Handle, LB_DELETESTRING, (WPARAM)i, 0); }
        int  GetCount() const { return c->Handle ? (int)SendMessageW(c->Handle, LB_GETCOUNT, 0, 0) : 0; }
        int  GetSelectedIndex() const { return c->Handle ? (int)SendMessageW(c->Handle, LB_GETCURSEL, 0, 0) : -1; }
        void SetSelectedIndex(int i) { if (c->Handle) SendMessageW(c->Handle, LB_SETCURSEL, (WPARAM)i, 0); }
        std::wstring GetSelectedItem() const {
            if (!c->Handle) return L"";
            int i = GetSelectedIndex(); if (i < 0) return L"";
            wchar_t buf[256] = L"";
            SendMessageW(c->Handle, LB_GETTEXT, (WPARAM)i, (LPARAM)buf);
            return buf;
        }
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
};

class ListView : public Control {
public:
    ListView(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysListView32", text.c_str(),
            LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) {
            SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
            ListView_SetExtendedListViewStyle(Handle, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        }
    }
    // listView1->Columns->Add(L"Ad", 140);
    class ColumnsProp {
        ListView* c;
    public:
        explicit ColumnsProp(ListView* o) : c(o) {}
        ColumnsProp* operator->() { return this; }
        void Clear() { if (c->Handle) { while ((int)SendMessageW(c->Handle, LVM_DELETECOLUMN, 0, 0) > 0) {} } }
        void Add(const std::wstring& title, int width = 120) {
            if (!c->Handle) return;
            LVCOLUMNW lc = { 0 };
            lc.mask = LVCF_TEXT | LVCF_WIDTH;
            lc.pszText = (LPWSTR)title.c_str();
            lc.cx = width;
            int n = (int)SendMessageW(c->Handle, LVM_GETITEMCOUNT, 0, 0);
            SendMessageW(c->Handle, LVM_INSERTCOLUMNW, n, (LPARAM)&lc);
        }
    } Columns{ this };
};

class TreeView : public Control {
public:
    TreeView(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysTreeView32", text.c_str(),
            TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_BORDER | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};

class ProgressBar : public Control {
public:
    ProgressBar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"msctls_progress32", text.c_str(),
            WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        SendMessageW(Handle, PBM_SETRANGE32, 0, 100);
    }
    // progressBar1->Value = 50;  progressBar1->Maximum = 200;
    class ValueProp {
        Control* c;
    public:
        explicit ValueProp(Control* o) : c(o) {}
        ValueProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, PBM_SETPOS, (WPARAM)v, 0); return *this; }
        operator int() const { return c->Handle ? (int)SendMessageW(c->Handle, PBM_GETPOS, 0, 0) : 0; }
    } Value{ this };
    class MaximumProp {
        Control* c;
    public:
        explicit MaximumProp(Control* o) : c(o) {}
        MaximumProp& operator=(int v) { if (c->Handle) SendMessageW(c->Handle, PBM_SETRANGE32, 0, (LPARAM)v); return *this; }
    } Maximum{ this };
};

class TrackBar : public Control {
public:
    TrackBar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"msctls_trackbar32", text.c_str(),
            TBS_HORZ | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
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
};

class HScrollBar : public Control {
public:
    HScrollBar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"SCROLLBAR", text.c_str(),
            SBS_HORZ | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
    }
};
class VScrollBar : public Control {
public:
    VScrollBar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"SCROLLBAR", text.c_str(),
            SBS_VERT | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
    }
};

class DateTimePicker : public Control {
public:
    DateTimePicker(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysDateTimePick32", text.c_str(),
            DTS_SHORTDATECENTURYFORMAT | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};
class MonthCalendar : public Control {
public:
    MonthCalendar(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"SysMonthCal32", text.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};

class TabControl : public Control {
public:
    TabControl(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysTabControl32", text.c_str(),
            TCS_TABS | WS_CLIPCHILDREN | WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
    // tabControl1->Pages->Add(L"tabPage1");
    class PagesProp {
        TabControl* c;
    public:
        explicit PagesProp(TabControl* o) : c(o) {}
        PagesProp* operator->() { return this; }
        int Count() const { return c->Handle ? (int)SendMessageW(c->Handle, TCM_GETITEMCOUNT, 0, 0) : 0; }
        void Add(const std::wstring& title) {
            if (!c->Handle) return;
            TCITEMW ti = { 0 };
            ti.mask = TCIF_TEXT;
            ti.pszText = (LPWSTR)title.c_str();
            SendMessageW(c->Handle, TCM_INSERTITEMW, (WPARAM)Count(), (LPARAM)&ti);
        }
    } Pages{ this };
};

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
        void Add(const std::wstring& t) { if (c->Handle) { int n = (int)SendMessageW(c->Handle, SB_GETPARTS, 0, 0); SendMessageW(c->Handle, SB_SETTEXTW, n | SBT_NOBORDERS, (LPARAM)t.c_str()); } }
    } Parts{ this };
};

class Panel : public Control {
public:
    Panel(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        EnsurePanelClass();
        Handle = CreateWindowExW(0, L"M2GOD_PANEL_GEN", text.c_str(),
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
    }
};
class PictureBox : public Control {
public:
    PictureBox(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(0, L"STATIC", text.c_str(),
            SS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_BORDER, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};
// Desteklenmeyen tipler icin yer tutucu kutu
class Placeholder : public Control {
public:
    Placeholder(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        Handle = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", text.c_str(),
            SS_NOTIFY | WS_CHILD | WS_VISIBLE, x, y, w, h,
            parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
        Id = id;
        if (Handle) SendMessageW(Handle, WM_SETFONT, (WPARAM)GetUIFont(), TRUE);
    }
};

} // namespace M2God
)M2GODGLOBAL";
    return Utf8ToWide(kTemplate);
}

// kontrol için CREATE satırı üretimi (PARENT-AWARE, FAZ 2.4 NESNE STİLİ)
static std::wstring EmitCreateCall(const Ctrl* c) {
    std::wostringstream o;
    if (c->type == L"Timer") {
        o << L"    " << c->name << L" = new M2God::Timer(hWnd, " << IdMacroOf(c->name) << L");\n";
        o << L"    " << c->name << L"->Interval = " << c->Get(L"Interval", L"100") << L";\n";
        if (_wcsicmp(c->Get(L"Enabled", L"False").c_str(), L"true") == 0)
            o << L"    " << c->name << L"->Enabled = true;   // tasarımda Enabled=True\n";
        else
            o << L"    // " << c->name << L"->Enabled = true;   // Timer'ı başlatmak için\n";
        return o.str();
    }
    factory::Win32Class m = factory::MapClass(c->type);
    if (m.cls.empty()) {
        o << L"    // " << c->type << L" bileşeni (Global.h sarmalayıcısı yok)\n";
        return o.str();
    }
    std::wstring parentVar = c->parent ? (c->parent->name + L"->Handle") : std::wstring(L"hWnd");
    if (c->parent)
        o << L"    // " << c->name << L" → " << c->parent->name << L" içine (child)\n";
    std::wstring txt = c->Get(L"Text");
    std::wstring esc = EscCppStr(txt);
    std::wstring ctorArgs = parentVar + L", " + IdMacroOf(c->name) + L", L\"" + esc + L"\", " +
                            std::to_wstring(c->x) + L", " + std::to_wstring(c->y) + L", " +
                            std::to_wstring(c->w) + L", " + std::to_wstring(c->h);
    // TextBox ailesi: multiline parametresi
    if (c->type == L"TextBox" || c->type == L"MaskedTextBox" || c->type == L"NumericUpDown") {
        bool ml = (c->type == L"TextBox") && _wcsicmp(c->Get(L"MultiLine", L"False").c_str(), L"true") == 0;
        ctorArgs += ml ? L", true" : L", false";
    }
    o << L"    " << c->name << L" = new M2God::" << M2TypeOf(c->type) << L"(" << ctorArgs << L");\n";
    // --- özellik emisyonu ---
    // Items (ComboBox/ListBox): "Items" prop'u satır satır eklenir
    auto itItems = c->props.find(L"Items");
    if (itItems != c->props.end() && !itItems->second.empty()) {
        o << L"    " << c->name << L"->Items->Clear();\n";
        for (auto& line : Split(itItems->second, L'\n')) {
            std::wstring t = Trim(line);
            if (!t.empty())
                o << L"    " << c->name << L"->Items->Add(L\"" << EscCppStr(t) << L"\");\n";
        }
    }
    // SelectedIndex
    {
        std::wstring si = c->Get(L"SelectedIndex");
        if (!si.empty() && iswdigit(si[0]) && _wtoi(si.c_str()) >= 0)
            o << L"    " << c->name << L"->Items->SelectedIndex = " << _wtoi(si.c_str()) << L";\n";
    }
    // BackColor / ForeColor
    {
        std::wstring bv = c->Get(L"BackColor");
        if (!bv.empty() && _wcsicmp(bv.c_str(), L"Control") != 0) {
            COLORREF cr;
            if (ParseColor(bv, cr))
                o << L"    " << c->name << L"->BackColor = RGB(" << (int)GetRValue(cr) << L", "
                  << (int)GetGValue(cr) << L", " << (int)GetBValue(cr) << L");\n";
        }
        std::wstring fv = c->Get(L"ForeColor");
        if (!fv.empty() && _wcsicmp(fv.c_str(), L"ControlText") != 0) {
            COLORREF cr;
            if (ParseColor(fv, cr))
                o << L"    " << c->name << L"->ForeColor = RGB(" << (int)GetRValue(cr) << L", "
                  << (int)GetGValue(cr) << L", " << (int)GetBValue(cr) << L");\n";
        }
    }
    // Font
    {
        std::wstring fnt = c->Get(L"Font", L"Segoe UI, 9pt");
        std::wstring fn; int pt; bool bo, it, un;
        if (ParseFontValue(fnt, fn, pt, bo, it, un)) {
            bool isDefault = (_wcsicmp(fn.c_str(), L"Segoe UI") == 0 && pt == 9 && !bo && !it && !un);
            if (!isDefault) {
                o << L"    " << c->name << L"->Font = M2God::MakeFont(L\"" << EscCppStr(fn) << L"\", "
                  << pt << L", " << (bo ? L"true" : L"false") << L", " << (it ? L"true" : L"false")
                  << L", " << (un ? L"true" : L"false") << L");\n";
            }
        }
    }
    // Checked (CheckBox/RadioButton)
    if ((c->type == L"CheckBox" || c->type == L"RadioButton") && _wcsicmp(c->Get(L"Checked", L"False").c_str(), L"true") == 0)
        o << L"    " << c->name << L"->Checked = true;\n";
    // Enabled / Visible
    if (_wcsicmp(c->Get(L"Enabled", L"True").c_str(), L"false") == 0)
        o << L"    " << c->name << L"->Enabled = false;\n";
    if (_wcsicmp(c->Get(L"Visible", L"True").c_str(), L"false") == 0)
        o << L"    " << c->name << L"->Visible = false;\n";
    // konteyner bildirim köprüsü: konteyner çocuklarının WM_COMMAND/WM_NOTIFY'ı forma taşınır
    if (IsContainerType(c->type))
        o << L"    SetWindowSubclass(" << c->name << L"->Handle, ContainerBridgeProc, 1, 0);\n";
    return o.str();
}

// HANDLES bloğu (cpp tanımları) — hem üretimde hem senkronda kullanılır
// Not: Timer her koşulda M2God::Timer; winClass'ı boş (sarmalayıcısı olmayan) bileşenler atlanır
static std::wstring EmitHandlesBlock(bool asExtern) {
    std::wostringstream h;
    for (auto* c : g_model.ctrls) {
        if (c->type == L"Timer") {
            h << (asExtern ? L"extern " : L"") << L"M2God::Timer* " << c->name << (asExtern ? L";\n" : L" = nullptr;\n");
            continue;
        }
        if (c->nonVisual) continue;
        factory::Win32Class mc = factory::MapClass(c->type);
        if (mc.cls.empty()) continue;   // Global.h sarmalayıcısı olmayan bileşen
        h << (asExtern ? L"extern " : L"") << L"M2God::" << M2TypeOf(c->type)
          << L"* " << c->name << (asExtern ? L";\n" : L" = nullptr;\n");
    }
    return h.str();
}

std::wstring GenerateForm1Cpp() {
    std::wostringstream o;
    o << L"// Form1.cpp — M2GodWin32 UI Designer (Faz 2.4) tarafından üretilmiştir.\n";
    o << L"// Nesne sözdizimi için Global.h: timer1->Enabled = false; button1->Text = L\"...\";\n";
    o << L"// Marker blokları (//== M2GOD:... ) tasarımcı senkronizasyonu içindir; dışını düzenleyebilirsiniz.\n\n";
    o << L"#include \"Form1.h\"\n";
    o << L"#include <commctrl.h>\n\n";
    o << L"#pragma comment(lib, \"Comctl32.lib\")\n\n";
    o << L"namespace M2GodForm {\n\n";
    o << L"static HFONT  hFont     = NULL;\n";
    o << L"static HBRUSH hBackBrush= NULL;\n";
    o << L"static HBRUSH hCtrlBrush= NULL;\n\n";
    o << L"static void EnsureStatics(HINSTANCE hInst) {\n";
    o << L"    if (hFont) return;\n";
    o << L"    hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,\n";
    o << L"        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,\n";
    o << L"        DEFAULT_PITCH | FF_DONTCARE, L\"Segoe UI\");\n";
    o << L"    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES };\n";
    o << L"    InitCommonControlsEx(&icc);\n";
    o << L"}\n\n";
    // HANDLES
    o << L"//== M2GOD:HANDLES-BEGIN\n";
    o << L"// Not: kontroller artık M2God nesneleridir (Global.h). HWND için: button1->Handle\n";
    o << EmitHandlesBlock(false);
    o << L"//== M2GOD:HANDLES-END\n\n";
    // CREATE
    o << L"//== M2GOD:CREATE-BEGIN\n";
    o << L"// Konteyner bildirim köprüsü: GroupBox/TabControl/Panel içine yaratılan alt kontrollerin\n";
    o << L"// bildirimleri (WM_COMMAND/WM_NOTIFY) doğrudan konteynere gider; köprü bunları ana forma taşır.\n";
    o << L"static LRESULT CALLBACK ContainerBridgeProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {\n";
    o << L"    if (m == WM_COMMAND || m == WM_NOTIFY) {\n";
    o << L"        HWND p = GetParent(h);\n";
    o << L"        if (p) { SendMessageW(p, m, w, l); return 0; }\n";
    o << L"    }\n";
    o << L"    return DefSubclassProc(h, m, w, l);\n";
    o << L"}\n\n";
    o << L"void InitializeControls(HWND hWnd) {\n";
    o << L"    HINSTANCE hInst = GetModuleHandleW(NULL);\n";
    o << L"    EnsureStatics(hInst);\n";
    std::function<void(Ctrl*)> emitBranch = [&](Ctrl* parent) {
        for (auto* c : g_model.ChildrenOf(parent)) {
            if (c->nonVisual || c->type == L"Timer") continue; // bileşenler sonda üretilir
            o << EmitCreateCall(c) << L"\n";
            emitBranch(c);
        }
    };
    emitBranch(nullptr);
    for (auto* c : g_model.ctrls) if (c->nonVisual && c->type != L"Timer") o << EmitCreateCall(c) << L"\n";
    for (auto* c : g_model.ctrls) if (c->type == L"Timer") o << EmitCreateCall(c) << L"\n";
    o << L"}\n\n";
    // CleanupControls — WM_DESTROY'da timer durdurma + nesne serbest bırakma
    o << L"void CleanupControls() {\n";
    o << L"    // Timer'lar önce durdurulur\n";
    for (auto* c : g_model.ctrls)
        if (c->type == L"Timer")
            o << L"    if (" << c->name << L") { " << c->name << L"->Stop(); delete " << c->name << L"; " << c->name << L" = nullptr; }\n";
    for (int i = (int)g_model.ctrls.size() - 1; i >= 0; i--) {
        Ctrl* c = g_model.ctrls[i];
        if (c->nonVisual || c->type == L"Timer") continue;
        o << L"    if (" << c->name << L") { delete " << c->name << L"; " << c->name << L" = nullptr; }\n";
    }
    o << L"}\n";
    o << L"//== M2GOD:CREATE-END\n\n";
    // EVENTS
    o << L"//== M2GOD:EVENTS-BEGIN  (olay işleyicileri; mevcut gövdeler senkronda korunur)\n";
    std::vector<std::wstring> emitted;
    auto emitStub = [&](const std::wstring& hname) {
        for (auto& e : emitted) if (_wcsicmp(e.c_str(), hname.c_str()) == 0) return;
        emitted.push_back(hname);
        o << L"void " << hname << L"(HWND hWnd) {\n";
        o << L"    // TODO: " << hname << L" — kodunuzu buraya yazın (timer1->Enabled = false; gibi)\n";
        o << L"}\n\n";
    };
    for (auto* c : g_model.ctrls)
        for (auto& kv : c->events) emitStub(kv.second);
    for (auto& kv : g_model.formEvents) emitStub(kv.second);
    o << L"//== M2GOD:EVENTS-END\n\n";
    o << L"} // namespace M2GodForm\n\n";
    // WNDPROC
    o << L"//== M2GOD:WNDPROC-BEGIN\n";
    o << L"static LRESULT CALLBACK Form1_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {\n";
    o << L"    switch (message) {\n";
    o << L"    case WM_CREATE:\n";
    o << L"        M2GodForm::InitializeControls(hWnd);\n";
    if (g_model.formEvents.count(L"Load"))
        o << L"        M2GodForm::" << g_model.formEvents[L"Load"] << L"(hWnd);\n";
    else
        o << L"        // Form1_Load burada çağrılır (Olaylar sekmesinden ekleyin)\n";
    o << L"        return 0;\n";
    // WM_COMMAND router
    o << L"    case WM_COMMAND: {\n";
    o << L"        int id = LOWORD(wParam), ev = HIWORD(wParam);\n";
    o << L"        switch (id) {\n";
    for (auto* c : g_model.ctrls) {
        if (c->events.empty()) continue;
        bool routed = false;
        for (auto& kv : c->events) {
            const std::wstring& evName = kv.first;
            std::wstring notify;
            if (evName == L"Click" || evName == L"CheckedChanged") notify = L"BN_CLICKED";
            else if (evName == L"TextChanged") notify = L"EN_CHANGE";
            else if (evName == L"SelectedIndexChanged") {
                if (c->type == L"ComboBox") notify = L"CBN_SELCHANGE";
                else notify = L"LBN_SELCHANGE";
            }
            if (notify.empty()) continue;
            routed = true;
            o << L"        case M2GodForm::" << IdMacroOf(c->name) << L":\n";
            o << L"            if (ev == " << notify << L") M2GodForm::" << kv.second << L"(hWnd);\n";
            o << L"            break;\n";
        }
        if (routed) o << L"\n";
    }
    o << L"        }\n";
    o << L"        break;\n";
    o << L"    }\n";
    // WM_TIMER — gerçek timer dispatch (Faz 2.4); çoklu timer tek case içinde
    {
        std::wostringstream timerCases;
        for (auto* c : g_model.ctrls) {
            if (c->type != L"Timer") continue;
            auto ev = c->events.find(L"Tick");
            if (ev == c->events.end()) continue;
            timerCases << L"        if (wParam == M2GodForm::" << IdMacroOf(c->name) << L") {\n";
            timerCases << L"            if (M2GodForm::" << c->name << L" && M2GodForm::" << c->name << L"->Enabled) M2GodForm::" << ev->second << L"(hWnd);\n";
            timerCases << L"            return 0;\n";
            timerCases << L"        }\n";
        }
        if (!timerCases.str().empty()) {
            o << L"    case WM_TIMER:\n";
            o << timerCases.str();
            o << L"        break;\n";
        }
    }
    // WM_CTLCOLOR* — BackColor/ForeColor uygulanır (Faz 2.4)
    o << L"    case WM_CTLCOLORSTATIC:\n";
    o << L"    case WM_CTLCOLOREDIT:\n";
    o << L"    case WM_CTLCOLORLISTBOX:\n";
    o << L"    case WM_CTLCOLORBTN:\n";
    o << L"    case WM_CTLCOLORSCROLLBAR: {\n";
    o << L"        LRESULT r = M2God::HandleCtlColor((HWND)lParam, (HDC)wParam);\n";
    o << L"        if (r) return r;\n";
    o << L"        break;\n";
    o << L"    }\n";
    o << L"    case WM_DESTROY:\n";
    o << L"        M2GodForm::CleanupControls();\n";
    o << L"        PostQuitMessage(0);\n";
    o << L"        return 0;\n";
    o << L"    }\n";
    o << L"    return DefWindowProcW(hWnd, message, wParam, lParam);\n";
    o << L"}\n\n";
    o << L"int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {\n";
    o << L"    M2GodForm::EnsureStatics(hInst);\n";
    o << L"    WNDCLASSW wc = { 0 };\n";
    o << L"    wc.lpfnWndProc  = Form1_WndProc;\n";
    o << L"    wc.hInstance    = hInst;\n";
    o << L"    wc.hCursor      = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);\n";
    // form backcolor
    std::wstring fb = g_model.formProps.count(L"BackColor") ? g_model.formProps[L"BackColor"] : L"Control";
    o << L"    wc.hbrBackground = CreateSolidBrush(RGB(";
    COLORREF cr; ParseColor(fb, cr);
    o << GetRValue(cr) << L"," << GetGValue(cr) << L"," << GetBValue(cr) << L"));\n";
    o << L"    wc.lpszClassName = L\"M2GodForm1\";\n";
    o << L"    RegisterClassW(&wc);\n\n";
    int fw = g_model.formW, fh = g_model.formH;
    o << L"    // Form1 boyutu (ClientSize: " << fw << L" × " << fh << L")\n";
    o << L"    RECT rc = { 0, 0, " << fw << L", " << fh << L" };\n";
    o << L"    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);\n";
    o << L"    HWND hWnd = CreateWindowW(L\"M2GodForm1\", L\"";
    o << (g_model.formProps.count(L"Text") ? g_model.formProps[L"Text"] : std::wstring(L"Form1"));
    o << L"\", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,\n";
    o << L"        rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);\n";
    o << L"    if (!hWnd) return 0;\n";
    o << L"    ShowWindow(hWnd, nCmdShow);\n";
    o << L"    UpdateWindow(hWnd);\n";
    o << L"    MSG msg;\n";
    o << L"    while (GetMessageW(&msg, NULL, 0, 0)) {\n";
    o << L"        TranslateMessage(&msg);\n";
    o << L"        DispatchMessageW(&msg);\n";
    o << L"    }\n";
    o << L"    return (int)msg.wParam;\n";
    o << L"}\n";
    o << L"//== M2GOD:WNDPROC-END\n";
    return o.str();
}

std::wstring GenerateForm1H() {
    std::wostringstream o;
    o << L"// Form1.h — M2GodWin32 UI Designer (Faz 2.4) tarafından üretilmiştir.\n";
    o << L"#pragma once\n";
    o << L"#include <windows.h>\n";
    o << L"//== M2GOD:INCLUDE-BEGIN\n";
    o << L"#include \"Global.h\"\n";
    o << L"//== M2GOD:INCLUDE-END\n\n";
    o << L"namespace M2GodForm {\n\n";
    // IDS — Timer bileşenleri dahil (WM_TIMER dispatch id'si olarak kullanılır)
    o << L"//== M2GOD:IDS-BEGIN\n";
    o << L"enum {\n";
    bool first = true;
    for (auto* c : g_model.ctrls) {
        if (c->nonVisual && c->type != L"Timer") continue;
        if (!first) o << L",\n";
        o << L"    " << IdMacroOf(c->name) << L" = " << (3000 + (UINT)(c->cid - 3000));
        first = false;
    }
    o << L"\n};\n";
    o << L"//== M2GOD:IDS-END\n\n";
    o << L"//== M2GOD:HANDLES-BEGIN\n";
    o << L"// Not: kontroller M2God nesneleridir (Global.h). HWND için: button1->Handle\n";
    o << EmitHandlesBlock(true);
    o << L"//== M2GOD:HANDLES-END\n\n";
    o << L"//== M2GOD:PROTO-BEGIN\n";
    o << L"void InitializeControls(HWND hWnd);\n";
    o << L"void CleanupControls();\n";
    o << L"//== M2GOD:PROTO-END\n\n";
    o << L"//== M2GOD:EVENTDECL-BEGIN\n";
    for (auto* c : g_model.ctrls) {
        for (auto& kv : c->events)
            o << L"void " << kv.second << L"(HWND hWnd);\n";
    }
    for (auto& kv : g_model.formEvents)
        o << L"void " << kv.second << L"(HWND hWnd);\n";
    o << L"//== M2GOD:EVENTDECL-END\n\n";
    o << L"} // namespace M2GodForm\n";
    return o.str();
}

// ===PART2===

// --- bölge değiştirme -----------------------------------------------------------

static bool ReplaceRegion(std::wstring& text, const wchar_t* beginMark, const wchar_t* endMark, const std::wstring& newContent) {
    size_t b = text.find(beginMark);
    if (b == std::wstring::npos) return false;
    size_t e = text.find(endMark, b);
    if (e == std::wstring::npos) return false;
    size_t contentStart = b + wcslen(beginMark);
    text = text.substr(0, contentStart) + L"\n" + newContent + text.substr(e);
    return true;
}

static bool HasRegion(const std::wstring& text, const wchar_t* beginMark) {
    return text.find(beginMark) != std::wstring::npos;
}

static int LineOfPos(const std::wstring& text, size_t pos) {
    int line = 1;
    for (size_t i = 0; i < pos && i < text.size(); i++)
        if (text[i] == L'\n') line++;
    return line;
}

// Global.h dosyasını garanti et: yoksa veya sürüm eskiyse üzerine yaz
static void EnsureGlobalHFile(const std::wstring& cppPath) {
    std::wstring dir = GetFileDir(cppPath);
    if (dir.empty()) return;
    std::wstring gpath = dir + L"\\Global.h";
    std::string cur;
    if (ReadFileText(gpath, cur)) {
        if (cur.find(GlobalVersionMark()) != std::string::npos) return; // güncel
    }
    WriteFileText(gpath, WideToUtf8(GenerateGlobalH()));
}

int EnsureHandler(const std::wstring& cppPath, const std::wstring& hPath,
                  const std::wstring& handlerName, const std::wstring& declLine) {
    std::string tmp8;
    if (!ReadFileText(cppPath, tmp8)) return 0;
    std::wstring cpp = Utf8ToWide(tmp8);
    std::wstring sig = L"void " + handlerName + L"(";
    size_t found = Lower(cpp).find(Lower(sig));
    if (found == std::wstring::npos) {
        // stub ekle
        std::wstring stub = L"void " + handlerName + L"(HWND hWnd) {\n    // TODO: " + handlerName + L"\n}\n\n";
        const wchar_t* endMark = L"//== M2GOD:EVENTS-END";
        size_t e = cpp.find(endMark);
        if (e != std::wstring::npos) {
            cpp.insert(e, stub);
        } else {
            cpp += L"\n" + stub;
        }
        WriteFileText(cppPath, WideToUtf8(cpp));
        found = Lower(cpp).find(Lower(sig));
    }
    // bildirimi .h'ye ekle (yoksa)
    if (!declLine.empty() && FileExists(hPath)) {
        std::string t8;
        ReadFileText(hPath, t8);
        std::wstring h = Utf8ToWide(t8);
        std::wstring decl = L"    " + declLine + L"\n";
        size_t dr = Lower(h).find(Lower(L"void " + declLine));
        if (dr == std::wstring::npos) {
            const wchar_t* endMark = L"//== M2GOD:EVENTDECL-END";
            size_t e = h.find(endMark);
            if (e != std::wstring::npos) h.insert(e, decl);
            else h += L"\n" + decl;
            WriteFileText(hPath, WideToUtf8(h));
        }
    }
    if (found == std::wstring::npos) return 0;
    return LineOfPos(cpp, found);
}

int SyncIntoFile(const std::wstring& cppPath, const std::wstring& hPath,
                 Ctrl* targetCtrl, const std::wstring& eventName, int* handlerLine) {
    std::wstring cpp;
    if (!FileExists(cppPath)) {
        cpp = GenerateForm1Cpp();
        WriteFileText(cppPath, WideToUtf8(cpp));
    } else {
        std::string t8;
        if (!ReadFileText(cppPath, t8)) return 0;
        cpp = Utf8ToWide(t8);
    }

    std::wstring h;
    bool hNew = false;
    if (!FileExists(hPath)) {
        h = GenerateForm1H();
        hNew = true;
    } else {
        std::string t8;
        if (!ReadFileText(hPath, t8)) return 0;
        h = Utf8ToWide(t8);
    }

    if (!HasRegion(cpp, L"//== M2GOD:HANDLES-BEGIN") || !HasRegion(cpp, L"//== M2GOD:CREATE-BEGIN")) {
        Status(L"Hata: Form1.cpp içinde M2GOD marker blokları yok");
        return 0;
    }
    // HANDLES bölgesi: M2God nesne tanımları (Faz 2.4 stili — eski HWND stili burada onarılır)
    ReplaceRegion(cpp, L"//== M2GOD:HANDLES-BEGIN", L"//== M2GOD:HANDLES-END", EmitHandlesBlock(false));
    // CREATE bölgesi: fonksiyon gövdesini yeniden üret
    std::wstring fresh = GenerateForm1Cpp();
    size_t cb = fresh.find(L"//== M2GOD:CREATE-BEGIN");
    size_t ce = fresh.find(L"//== M2GOD:CREATE-END");
    if (cb != std::wstring::npos && ce != std::wstring::npos) {
        std::wstring createBody = fresh.substr(cb + wcslen(L"//== M2GOD:CREATE-BEGIN"), ce - (cb + wcslen(L"//== M2GOD:CREATE-BEGIN")));
        ReplaceRegion(cpp, L"//== M2GOD:CREATE-BEGIN", L"//== M2GOD:CREATE-END", createBody);
    }
    // WNDPROC bölgesi de tazelenir: WM_TIMER/WM_CTLCOLOR/WM_COMMAND yönlendirmesi güncel
    size_t wb = fresh.find(L"//== M2GOD:WNDPROC-BEGIN");
    size_t we = fresh.find(L"//== M2GOD:WNDPROC-END");
    if (wb != std::wstring::npos && we != std::wstring::npos &&
        HasRegion(cpp, L"//== M2GOD:WNDPROC-BEGIN")) {
        std::wstring wndBody = fresh.substr(wb + wcslen(L"//== M2GOD:WNDPROC-BEGIN"), we - (wb + wcslen(L"//== M2GOD:WNDPROC-BEGIN")));
        ReplaceRegion(cpp, L"//== M2GOD:WNDPROC-BEGIN", L"//== M2GOD:WNDPROC-END", wndBody);
    }
    WriteFileText(cppPath, WideToUtf8(cpp));
    // header bölgeleri
    if (!hNew && !HasRegion(h, L"//== M2GOD:IDS-BEGIN")) {
        Status(L"Hata: Form1.h içinde M2GOD marker blokları yok");
        return 0;
    }
    {
        std::wstring freshH = GenerateForm1H();
        auto region = [&](const wchar_t* b, const wchar_t* e, std::wstring& dst) {
            size_t rb = freshH.find(b), re = freshH.find(e);
            if (rb != std::wstring::npos && re != std::wstring::npos)
                ReplaceRegion(dst, b, e, freshH.substr(rb + wcslen(b), re - (rb + wcslen(b))));
        };
        region(L"//== M2GOD:IDS-BEGIN", L"//== M2GOD:IDS-END", h);
        region(L"//== M2GOD:HANDLES-BEGIN", L"//== M2GOD:HANDLES-END", h);
        region(L"//== M2GOD:PROTO-BEGIN", L"//== M2GOD:PROTO-END", h);
        region(L"//== M2GOD:EVENTDECL-BEGIN", L"//== M2GOD:EVENTDECL-END", h);
        // eski (Faz 2.3 ve öncesi) header'da INCLUDE bölgesi yoksa Global.h include'unu garanti et
        if (h.find(L"#include \"Global.h\"") == std::wstring::npos) {
            size_t p = h.find(L"#pragma once");
            if (p != std::wstring::npos) {
                p += wcslen(L"#pragma once");
                h.insert(p, L"\n#include \"Global.h\"");
            }
        }
        WriteFileText(hPath, WideToUtf8(h));
    }
    // Global.h: yoksa veya sürüm eskiyse yaz (Faz 2.4)
    EnsureGlobalHFile(cppPath);
    // hedef handler
    int line = 0;
    if (targetCtrl && !eventName.empty()) {
        std::wstring hname = g_model.HandlerNameOf(targetCtrl, eventName);
        line = EnsureHandler(cppPath, hPath, hname, L"void " + hname + L"(HWND hWnd);");
    }
    if (handlerLine) *handlerLine = line;
    return 1;
}

bool EmitProject(const std::wstring& folder) {
    if (folder.empty()) return false;
    std::wstring cpp = folder + L"\\Form1.cpp";
    std::wstring h = folder + L"\\Form1.h";
    std::wstring gh = folder + L"\\Global.h";
    if (!WriteFileText(cpp, WideToUtf8(GenerateForm1Cpp()))) return false;
    if (!WriteFileText(h, WideToUtf8(GenerateForm1H()))) return false;
    WriteFileText(gh, WideToUtf8(GenerateGlobalH()));
    return true;
}

// --- Faz1.4 GeneratedForm.cpp içe aktarma -----------------------------------------
bool ImportLegacyGenerated(const std::wstring& cppPath) {
    std::string data;
    if (!ReadFileText(cppPath, data)) return false;
    std::wstring text = Utf8ToWide(data);
    g_model.ClearAll();
    g_model.PushUndo();
    int count = 0;
    size_t pos = 0;
    while (true) {
        size_t cw = text.find(L"CreateWindow", pos);
        if (cw == std::wstring::npos) break;
        size_t lp = text.find(L'(', cw);
        size_t semicolon = text.find(L';', cw);
        if (lp == std::wstring::npos || semicolon == std::wstring::npos) break;
        std::wstring line = text.substr(lp + 1, semicolon - lp);
        // tırnaklı dizeler
        std::vector<std::wstring> strs;
        size_t q = 0;
        while ((q = line.find(L'"', q)) != std::wstring::npos) {
            size_t q2 = line.find(L'"', q + 1);
            if (q2 == std::wstring::npos) break;
            strs.push_back(line.substr(q + 1, q2 - q - 1));
            q = q2 + 1;
        }
        // sayılar (x,y,w,h) — ikinci dizeden sonra, (HMENU) öncesi
        if (strs.size() >= 2) {
            std::wstring cls = strs[0], txt = strs[1];
            std::wstring rest = line.substr(line.find(L'"', line.find(L'"') + 1));
            size_t hm = Lower(rest).find(L"hmenu");
            if (hm == std::wstring::npos) hm = rest.size();
            std::wstring numsPart = rest.substr(0, hm);
            std::vector<int> nums;
            size_t i = 0;
            while (i < numsPart.size() && nums.size() < 4) {
                if (iswdigit(numsPart[i]) || (numsPart[i] == L'-' && i + 1 < numsPart.size() && iswdigit(numsPart[i + 1]))) {
                    int v = (int)wcstol(numsPart.c_str() + i, nullptr, 10);
                    nums.push_back(v);
                    while (i < numsPart.size() && (iswdigit(numsPart[i]) || numsPart[i] == L'-')) i++;
                } else i++;
            }
            if (nums.size() >= 4) {
                // sınıf → tip
                std::wstring type;
                std::wstring low = Lower(line);
                if (_wcsicmp(cls.c_str(), L"BUTTON") == 0) {
                    if (low.find(L"bs_autocheckbox") != std::wstring::npos) type = L"CheckBox";
                    else if (low.find(L"bs_autoradiobutton") != std::wstring::npos) type = L"RadioButton";
                    else if (low.find(L"bs_groupbox") != std::wstring::npos) type = L"GroupBox";
                    else type = L"Button";
                }
                else if (_wcsicmp(cls.c_str(), L"EDIT") == 0) type = L"TextBox";
                else if (_wcsicmp(cls.c_str(), L"STATIC") == 0) type = L"Label";
                else if (_wcsicmp(cls.c_str(), L"COMBOBOX") == 0) type = L"ComboBox";
                else if (_wcsicmp(cls.c_str(), L"LISTBOX") == 0) type = L"ListBox";
                else if (_wcsicmp(cls.c_str(), L"msctls_progress32") == 0) type = L"ProgressBar";
                else if (_wcsicmp(cls.c_str(), L"msctls_trackbar32") == 0) type = L"TrackBar";
                else if (_wcsicmp(cls.c_str(), L"SysListView32") == 0) type = L"ListView";
                else if (_wcsicmp(cls.c_str(), L"SysTreeView32") == 0) type = L"TreeView";
                else if (_wcsicmp(cls.c_str(), L"SysTabControl32") == 0) type = L"TabControl";
                else if (_wcsicmp(cls.c_str(), L"RichEdit20W") == 0) type = L"RichTextBox";
                else type = L"Label";
                Ctrl* c = g_model.Add(type, nums[0], nums[1]);
                if (c) {
                    c->x = nums[0]; c->y = nums[1]; c->w = nums[2]; c->h = nums[3];
                    c->props[L"Text"] = txt;
                    wchar_t b1[64], b2[64];
                    swprintf(b1, 64, L"%d, %d", c->x, c->y);
                    swprintf(b2, 64, L"%d, %d", c->w, c->h);
                    c->props[L"Location"] = b1; c->props[L"Size"] = b2;
                    count++;
                }
            }
        }
        pos = semicolon + 1;
    }
    return count > 0;
}

} // namespace codegen
