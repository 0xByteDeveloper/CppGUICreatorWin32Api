// factory.cpp — kontrol pencerelerinin oluşturulması ve özellik uygulaması
#include "factory.h"
#include "app.h"
#include "surface.h"
#include "catalog.h"
#include "json.h"

namespace factory {

struct Entry { const wchar_t* type; const wchar_t* cls; DWORD style; DWORD exstyle; int w, h; const wchar_t* styleText; };
static const Entry kMap[] = {
    { L"Button",            L"BUTTON",             WS_TABSTOP | BS_PUSHBUTTON, 0, 100, 32 , L"BS_PUSHBUTTON" },
    { L"CheckBox",          L"BUTTON",             WS_TABSTOP | BS_AUTOCHECKBOX, 0, 120, 28 , L"BS_PUSHBUTTON" },
    { L"RadioButton",       L"BUTTON",             WS_TABSTOP | BS_AUTORADIOBUTTON, 0, 130, 28 , L"BS_PUSHBUTTON" },
    { L"GroupBox",          L"BUTTON",             BS_GROUPBOX, 0, 200, 120 , L"BS_PUSHBUTTON" },
    { L"Label",             L"STATIC",             SS_LEFT | SS_NOTIFY, 0, 100, 23 , L"SS_LEFT | SS_NOTIFY" },
    { L"LinkLabel",         L"STATIC",             SS_LEFT | SS_NOTIFY, 0, 100, 23 , L"SS_LEFT | SS_NOTIFY" },
    { L"TextBox",           L"EDIT",               WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, 140, 26 , L"WS_BORDER | ES_AUTOHSCROLL" },
    { L"MaskedTextBox",     L"EDIT",               WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, 140, 26 , L"WS_BORDER | ES_AUTOHSCROLL" },
    { L"NumericUpDown",     L"EDIT",               WS_BORDER | WS_TABSTOP | ES_NUMBER, WS_EX_CLIENTEDGE, 100, 26 , L"WS_BORDER | ES_AUTOHSCROLL" },
    { L"RichTextBox",       L"RichEdit20W",        WS_BORDER | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, WS_EX_CLIENTEDGE, 180, 100 , L"WS_BORDER | ES_MULTILINE" },
    { L"ComboBox",          L"COMBOBOX",           WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 150, 120 , L"CBS_DROPDOWNLIST | WS_VSCROLL" },
    { L"ListBox",           L"LISTBOX",            WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 0, 140, 120 , L"WS_BORDER | WS_VSCROLL | LBS_NOTIFY" },
    { L"CheckedListBox",    L"LISTBOX",            WS_TABSTOP | WS_BORDER | WS_VSCROLL, 0, 140, 120 , L"WS_BORDER | WS_VSCROLL | LBS_NOTIFY" },
    { L"ListView",          L"SysListView32",      WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, 180, 120 , L"WS_BORDER | LVS_REPORT" },
    { L"TreeView",          L"SysTreeView32",      WS_TABSTOP | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, WS_EX_CLIENTEDGE, 180, 140 , L"WS_BORDER | TVS_HASLINES" },
    { L"ProgressBar",       L"msctls_progress32",  0, 0, 160, 26 , L"0" },
    { L"TrackBar",          L"msctls_trackbar32",  WS_TABSTOP | TBS_HORZ, 0, 160, 40 , L"TBS_HORZ" },
    { L"HScrollBar",        L"SCROLLBAR",          SBS_HORZ, 0, 160, 20 , L"0" },
    { L"VScrollBar",        L"SCROLLBAR",          SBS_VERT, 0, 20, 160 , L"0" },
    { L"DateTimePicker",    L"SysDateTimePick32",  WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT, WS_EX_CLIENTEDGE, 160, 28 , L"DTS_SHORTDATECENTURYFORMAT" },
    { L"MonthCalendar",     L"SysMonthCal32",      WS_TABSTOP, 0, 230, 180 , L"0" },
    { L"TabControl",        L"SysTabControl32",    WS_TABSTOP | WS_CLIPCHILDREN | TCS_TABS, WS_EX_CLIENTEDGE, 220, 150 , L"TCS_TABS" },
    { L"StatusStrip",       L"msctls_statusbar32", 0, 0, 200, 24 , L"0" },
    { L"Panel",             L"M2GOD_PANEL",        0, WS_EX_CLIENTEDGE, 180, 120 , L"0" },
    { L"FlowLayoutPanel",   L"M2GOD_PANEL",        0, WS_EX_CLIENTEDGE, 200, 120 , L"0" },
    { L"TableLayoutPanel",  L"M2GOD_PANEL",        0, WS_EX_CLIENTEDGE, 200, 140 , L"0" },
    { L"SplitContainer",    L"M2GOD_PANEL",        0, WS_EX_CLIENTEDGE, 220, 140 , L"0" },
    { L"PictureBox",        L"M2GOD_PICTURE",      SS_NOTIFY, 0, 120, 100 , L"SS_NOTIFY" },
    { L"WebBrowser",        L"M2GOD_PLACEHOLDER",  0, WS_EX_CLIENTEDGE, 220, 150 , L"0" },
    { L"DataGridView",      L"M2GOD_PLACEHOLDER",  0, WS_EX_CLIENTEDGE, 240, 150 , L"0" },
    { L"MenuStrip",         L"M2GOD_PLACEHOLDER",  0, 0, 200, 28 , L"0" },
    { L"ContextMenuStrip",  L"M2GOD_PLACEHOLDER",  0, 0, 140, 60 , L"0" },
    { L"ToolStrip",         L"M2GOD_PLACEHOLDER",  0, 0, 200, 26 , L"0" },
    { L"PropertyGrid",      L"M2GOD_PLACEHOLDER",  0, WS_EX_CLIENTEDGE, 200, 150 , L"0" },
    { L"PrintPreviewControl", L"M2GOD_PLACEHOLDER", 0, WS_EX_CLIENTEDGE, 200, 150 , L"0" },
};
// bileşenler (görsel olmayan) — tepside çizilir
static const wchar_t* kComponents[] = {
    L"Timer", L"BackgroundWorker", L"BindingSource", L"BindingNavigator", L"FileSystemWatcher",
    L"SerialPort", L"Process", L"DirectoryEntry", L"DirectorySearcher", L"EventLog",
    L"MessageQueue", L"PerformanceCounter", L"ServiceController", L"ErrorProvider", L"HelpProvider",
    L"ImageList", L"NotifyIcon", L"OpenFileDialog", L"SaveFileDialog", L"FolderBrowserDialog",
    L"FontDialog", L"ColorDialog", L"PrintDialog", L"PrintPreviewDialog", L"PageSetupDialog",
    L"DataSet", L"DataView", L"DataColumn", L"DataTable", L"System.Data.Odbc.OdbcConnection",
};

Win32Class MapClass(const std::wstring& type) {
    // bileşen mi?
    for (auto* c : kComponents)
        if (_wcsicmp(c, type.c_str()) == 0) return { L"", 0, 0, L"" };
    for (auto& e : kMap)
        if (_wcsicmp(e.type, type.c_str()) == 0) return { e.cls, e.style, e.exstyle, e.styleText };
    // katalogdaki win32_mapping'e bak (ör. bilinmeyen ama bilinen sınıf)
    const ElementDef* def = g_catalog.Find(type);
    if (def && !def->winClass.empty()) {
        std::wstring wc = def->winClass;
        // "BUTTON / BS..." gibi notlar gelirse temizle
        size_t sp = wc.find_first_of(L" /");
        if (sp != std::wstring::npos) wc = wc.substr(0, sp);
        if (wc == L"SetTimer") return { L"", 0, 0 };
        if (_wcsicmp(wc.c_str(), L"BUTTON") == 0) return { wc, WS_TABSTOP | BS_PUSHBUTTON, 0, L"BS_PUSHBUTTON" };
        if (_wcsicmp(wc.c_str(), L"EDIT") == 0) return { wc, WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, L"WS_BORDER | ES_AUTOHSCROLL" };
        if (_wcsicmp(wc.c_str(), L"STATIC") == 0) return { wc, SS_LEFT | SS_NOTIFY, 0, L"SS_LEFT | SS_NOTIFY" };
        return { wc, WS_TABSTOP | WS_BORDER, 0, L"WS_BORDER" };
    }
    return { L"M2GOD_PLACEHOLDER", 0, WS_EX_CLIENTEDGE, L"0" };
}

// --- özel sınıflar ---------------------------------------------------------
LRESULT CALLBACK PanelProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        Ctrl* c = g_model.FindByHwnd(h);
        COLORREF back = RGB(255, 255, 255);
        if (c) { COLORREF t; if (ParseColor(c->Get(L"BackColor", L"White"), t)) back = t; }
        HBRUSH br = CreateSolidBrush(back);
        FillRect(dc, &rc, br);
        DeleteObject(br);
        SetBkMode(dc, TRANSPARENT);
        if (c) {
            COLORREF fg = RGB(0, 0, 0);
            if (ParseColor(c->Get(L"ForeColor", L"Black"), fg)) SetTextColor(dc, fg);
            SelectObject(dc, g_fontUI);
            std::wstring txt = c->Get(L"Text");
            if (!txt.empty()) DrawTextW(dc, txt.c_str(), -1, &rc, DT_LEFT | DT_TOP);
        }
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

LRESULT CALLBACK PictureProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH br = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(dc, &rc, br); DeleteObject(br);
        // çapraz görsel (resim yok sembolü)
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
        HPEN op = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, rc.left, rc.top, nullptr); LineTo(dc, rc.right, rc.bottom);
        MoveToEx(dc, rc.right - 1, rc.top, nullptr); LineTo(dc, rc.left, rc.bottom - 1);
        SelectObject(dc, op); DeleteObject(pen);
        FrameRect(dc, &rc, (HBRUSH)GetStockObject(GRAY_BRUSH));
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

void RegisterCustomClasses(HINSTANCE hInst) {
    WNDCLASSW wc = { 0 };
    wc.hInstance = hInst; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpfnWndProc = PanelProc; wc.hbrBackground = nullptr; wc.lpszClassName = L"M2GOD_PANEL";
    RegisterClassW(&wc);
    wc.lpfnWndProc = PictureProc; wc.lpszClassName = L"M2GOD_PICTURE";
    RegisterClassW(&wc);
    wc.lpfnWndProc = DefWindowProcW; wc.lpszClassName = L"M2GOD_PLACEHOLDER";
    RegisterClassW(&wc);
}

// --- özellik uygulama -------------------------------------------------------
static void ApplyFontIfNeeded(Ctrl* c) {
    std::wstring fv = c->Get(L"Font", L"Segoe UI, 9pt");
    std::wstring fn; int pt; bool bo, it, un;
    ParseFontValue(fv, fn, pt, bo, it, un);
    static HFONT hPrev = nullptr;
    HFONT f = MakeFont(fn, pt, bo, it, un);
    if (c->hwnd) {
        HFONT old = (HFONT)SendMessageW(c->hwnd, WM_GETFONT, 0, 0);
        SendMessageW(c->hwnd, WM_SETFONT, (WPARAM)f, TRUE);
        if (old && old != g_fontUI) DeleteObject(old);
    }
    (void)hPrev;
}

void ApplyProp(Ctrl* c, const std::wstring& prop, const std::wstring& value) {
    if (!c || !c->hwnd) return;
    HWND h = c->hwnd;
    if (prop == L"Text") {
        if (c->type == L"Button" || c->type == L"Label" || c->type == L"CheckBox" || c->type == L"RadioButton" || c->type == L"GroupBox" || c->type == L"LinkLabel")
            SetWindowTextW(h, value.c_str());
        else if (c->type == L"TextBox" || c->type == L"MaskedTextBox" || c->type == L"NumericUpDown")
            SetWindowTextW(h, value.c_str());
    } else if (prop == L"Enabled") {
        EnableWindow(h, _wcsicmp(value.c_str(), L"true") == 0);
    } else if (prop == L"Visible") {
        ShowWindow(h, _wcsicmp(value.c_str(), L"true") == 0 ? SW_SHOWNOACTIVATE : SW_HIDE);
    } else if (prop == L"Location") {
        auto p = Split(value, L',');
        if (p.size() == 2) { c->x = _wtoi(Trim(p[0]).c_str()); c->y = _wtoi(Trim(p[1]).c_str());
            SetWindowPos(h, nullptr, c->x, c->y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE); }
    } else if (prop == L"Size") {
        auto p = Split(value, L',');
        if (p.size() == 2) { c->w = _wtoi(Trim(p[0]).c_str()); c->h = _wtoi(Trim(p[1]).c_str());
            SetWindowPos(h, nullptr, 0, 0, c->w, c->h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); }
    } else if (prop == L"Font") {
        ApplyFontIfNeeded(c);
    } else if (prop == L"ForeColor" || prop == L"BackColor") {
        InvalidateRect(h, nullptr, TRUE);
    } else if (prop == L"BorderStyle") {
        // basitçe yeniden çiz
        InvalidateRect(h, nullptr, TRUE);
    } else if (prop == L"ReadOnly") {
        if (c->type == L"TextBox") SendMessageW(h, EM_SETREADONLY, _wcsicmp(value.c_str(), L"true") == 0, 0);
    } else if (prop == L"PasswordChar") {
        if (c->type == L"TextBox" && value.size() == 1) SendMessageW(h, EM_SETPASSWORDCHAR, (WPARAM)value[0], 0);
        else if (c->type == L"TextBox") SendMessageW(h, EM_SETPASSWORDCHAR, 0, 0);
        InvalidateRect(h, nullptr, TRUE);
    } else if (prop == L"MultiLine" || prop == L"Multiline") {
        // yeniden oluşturma gerektirir — yüzey halleder (MWM_PROPS_APPLIED)
    }
}

void ApplyAll(Ctrl* c) {
    if (!c) return;
    for (auto& kv : c->props) ApplyProp(c, kv.first, kv.second);
    // geometriyi garanti et
    if (c->hwnd) SetWindowPos(c->hwnd, nullptr, c->x, c->y, c->w, c->h, SWP_NOZORDER | SWP_NOACTIVATE);
}

HWND CreateOnSurface(HWND surface, Ctrl* c) {
    Win32Class m = MapClass(c->type);
    if (m.cls.empty()) { c->nonVisual = true; return nullptr; }
    DWORD style = WS_CHILD | WS_CLIPSIBLINGS | m.style;
    // görünür/default prop'ları
    if (_wcsicmp(c->Get(L"Visible", L"True").c_str(), L"true") == 0) style |= WS_VISIBLE;
    if (_wcsicmp(c->Get(L"Enabled", L"True").c_str(), L"true") == 0) style |= 0; else style |= WS_DISABLED;
    // Tip bazlı stil geçersiz kılmaları
    if (c->type == L"TextBox" && _wcsicmp(c->Get(L"MultiLine", L"False").c_str(), L"true") == 0)
        style |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
    if (c->type == L"Label") {
        std::wstring ta = c->Get(L"TextAlign", L"TopLeft");
        style &= ~SS_CENTER; 
        if (_wcsicmp(ta.c_str(), L"MiddleCenter") == 0 || _wcsicmp(ta.c_str(), L"TopCenter") == 0) { style |= SS_CENTER; }
        else if (_wcsicmp(ta.c_str(), L"TopRight") == 0) style |= SS_RIGHT;
    }
    if (c->type == L"ComboBox") {
        int n = _wtoi(c->Get(L"DropDownStyle", L"DropDown").c_str());
        (void)n;
    }
    HWND h = CreateWindowExW(m.exstyle, m.cls.c_str(), L"",
                             style, c->x, c->y, c->w, c->h,
                             surface, (HMENU)(INT_PTR)c->cid, g_hInst, nullptr);
    if (!h) return nullptr;
    c->hwnd = h;
    // metni koy
    std::wstring txt = c->Get(L"Text");
    if (!txt.empty()) SetWindowTextW(h, txt.c_str());
    // font
    ApplyFontIfNeeded(c);
    // listview/treeview/combobox örnek içerik (tasarım görünümü)
    if (c->type == L"ComboBox") {
        for (int i = 1; i <= 3; i++) {
            wchar_t b[32]; swprintf(b, 32, L"Öğe %d", i);
            SendMessageW(h, CB_ADDSTRING, 0, (LPARAM)b);
        }
    } else if (c->type == L"ListBox" || c->type == L"CheckedListBox") {
        for (int i = 1; i <= 4; i++) {
            wchar_t b[32]; swprintf(b, 32, L"Öğe %d", i);
            SendMessageW(h, LB_ADDSTRING, 0, (LPARAM)b);
        }
    } else if (c->type == L"TabControl") {
        TCITEMW ti = { 0 }; ti.mask = TCIF_TEXT;
        wchar_t b1[] = L"tabPage1", b2[] = L"tabPage2";
        ti.pszText = b1; SendMessageW(h, TCM_INSERTITEMW, 0, (LPARAM)&ti);
        ti.pszText = b2; SendMessageW(h, TCM_INSERTITEMW, 1, (LPARAM)&ti);
    }
    // tasarım modu: fare mesajlarını yüzeye aktar (HTTRANSPARENT)
    surface::SubclassForDesign(h);
    // ilk boyamada CTLCOLORSTATIC erken geldiği için tam yeniden boya
    InvalidateRect(h, nullptr, TRUE);
    // form client alanına göre konumlandır (tek ekleme yolu için)
    surface::PositionOne(surface, c);
    UpdateWindow(h);
    return h;
}

} // namespace factory
