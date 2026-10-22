// props.cpp — Özellikler & Olaylar paneli gerçeklemesi
#include "props.h"
#include "app.h"
#include "catalog.h"
#include "model.h"
#include "factory.h"
#include "surface.h"
#include <commctrl.h>

namespace props {

struct Row {
    std::wstring name;      // özellik/olay adı
    std::wstring value;     // mevcut değer / handler
    std::wstring type;      // "System.Boolean" ...
    std::wstring enumType;  // enum kısa adı
    bool readOnly = false;
    bool isEvent = false;
    std::wstring category;
};

static HWND s_panel = nullptr, s_tab = nullptr, s_tab2 = nullptr, s_list = nullptr, s_caption = nullptr;
static HWND s_edit = nullptr, s_combo = nullptr;   // satır içi düzenleyiciler
static int  s_editRow = -1, s_editCol = -1;
static std::vector<Row> s_rows;
static int s_mode = 0; // 0: özellikler, 1: olaylar

static bool IsEnumProp(const Row& r, std::vector<std::pair<std::wstring, int>>* vals) {
    if (r.enumType.empty()) return false;
    auto* v = g_catalog.EnumValues(r.enumType);
    if (!v) return false;
    if (vals) *vals = *v;
    return true;
}

static void EndEdit(HWND panel, bool commit);
static void FillRows();
static void SyncList();

// --- değer uygulama -----------------------------------------------------------
static void ApplyRowValue(Row& r, const std::wstring& val) {
    std::wstring v = Trim(val);
    if (r.isEvent) {
        if (g_model.formSelected || !g_model.selected) {
            if (v.empty()) g_model.formEvents.erase(r.name);
            else g_model.formEvents[r.name] = v;
        } else if (g_model.selected) {
            Ctrl* c = g_model.selected;
            if (v.empty()) c->events.erase(r.name);
            else c->events[r.name] = v;
        }
        PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
        return;
    }
    if (g_model.formSelected || !g_model.selected) {
        g_model.formProps[r.name] = v;
        if (r.name == L"ClientSize") {
            auto p = Split(v, L',');
            if (p.size() == 2) {
                g_model.formW = max(100, _wtoi(Trim(p[0]).c_str()));
                g_model.formH = max(100, _wtoi(Trim(p[1]).c_str()));
            }
            InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, TRUE);
        } else if (r.name == L"BackColor" || r.name == L"Text") {
            InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, TRUE);
        }
        PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
        return;
    }
    Ctrl* c = g_model.selected;
    if (r.name == L"Name") {
        std::wstring old = c->name;
        if (!v.empty() && _wcsicmp(v.c_str(), c->name.c_str()) != 0) {
            std::wstring nv = v;
            // geçerli tanımlayıcı yap
            for (auto& ch : nv) if (!iswalnum(ch) && ch != L'_') ch = L'_';
            if (iswdigit(nv[0])) nv = L"_" + nv;
            g_model.Rename(c, nv);
        }
        PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
        return;
    }
    if (r.name == L"Location" || r.name == L"Size") {
        factory::ApplyProp(c, r.name, v);
        InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, FALSE);
        PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
        return;
    }
    c->props[r.name] = v;
    factory::ApplyProp(c, r.name, v);
    // yeniden oluşturma gerektiren özellikler
    if (r.name == L"MultiLine" || r.name == L"Multiline") {
        HWND surf = GetDlgItem(g_hMain, IDC_SURFACE);
        surface::RecreateAll(surf);
    }
    InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, FALSE);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
}

// --- satır içi düzenleyiciler ---------------------------------------------------
static void DestroyEditors() {
    if (s_edit) { DestroyWindow(s_edit); s_edit = nullptr; }
    if (s_combo) { DestroyWindow(s_combo); s_combo = nullptr; }
    s_editRow = -1; s_editCol = -1;
}

static void BeginEdit(HWND panel, int row) {
    EndEdit(panel, true);
    if (row < 0 || row >= (int)s_rows.size()) return;
    Row& r = s_rows[row];
    if (r.readOnly) return;
    RECT rc;
    ListView_GetSubItemRect(s_list, row, 1, LVIR_BOUNDS, &rc);
    if (r.isEvent || _wcsicmp(r.type.c_str(), L"System.String") == 0 || r.type.empty() ||
        _wcsicmp(r.type.c_str(), L"System.Int32") == 0 ||
        r.type.find(L"Point") != std::wstring::npos || r.type.find(L"Size") != std::wstring::npos ||
        r.type.find(L"Color") != std::wstring::npos || r.type.find(L"Font") != std::wstring::npos) {
        // color/font için dropdown da izinli — CBS_DROPDOWN
        bool dropdown = r.type.find(L"Color") != std::wstring::npos || r.type.find(L"Font") != std::wstring::npos || r.isEvent == false && false;
        if (r.type.find(L"Color") != std::wstring::npos) {
            s_combo = CreateWindowExW(0, L"COMBOBOX", r.value.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN, rc.left, rc.top, rc.right - rc.left, 220,
                panel, (HMENU)1150, g_hInst, nullptr);
            const wchar_t* colors[] = { L"Black", L"White", L"Red", L"Lime", L"Blue", L"Yellow", L"Cyan", L"Magenta", L"Gray", L"Silver", L"Orange", L"Control", L"Window" };
            for (auto* cn : colors) SendMessageW(s_combo, CB_ADDSTRING, 0, (LPARAM)cn);
            SendMessageW(s_combo, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
            SetFocus(s_combo);
            s_editRow = row; s_editCol = 1;
            return;
        }
        s_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", r.value.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            panel, (HMENU)1151, g_hInst, nullptr);
        SendMessageW(s_edit, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
        SetFocus(s_edit);
        s_editRow = row; s_editCol = 1;
        return;
    }
    // enum / bool → combobox
    std::vector<std::pair<std::wstring, int>> vals;
    bool isEnum = IsEnumProp(r, &vals);
    bool isBool = _wcsicmp(r.type.c_str(), L"System.Boolean") == 0;
    if (isEnum || isBool) {
        s_combo = CreateWindowExW(0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, rc.left, rc.top, rc.right - rc.left, 260,
            panel, (HMENU)1150, g_hInst, nullptr);
        std::vector<std::wstring> opts;
        if (isBool) opts = { L"True", L"False" };
        else for (auto& vp : vals) opts.push_back(vp.first);
        for (auto& o : opts) SendMessageW(s_combo, CB_ADDSTRING, 0, (LPARAM)o.c_str());
        int cur = 0;
        for (size_t i = 0; i < opts.size(); i++) if (_wcsicmp(opts[i].c_str(), r.value.c_str()) == 0) { cur = (int)i; break; }
        SendMessageW(s_combo, CB_SETCURSEL, cur, 0);
        SendMessageW(s_combo, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
        SetFocus(s_combo);
        s_editRow = row; s_editCol = 1;
    }
}

static void EndEdit(HWND panel, bool commit) {
    if (s_editRow < 0) return;
    int row = s_editRow;
    std::wstring val;
    if (s_edit) {
        wchar_t buf[1024]; GetWindowTextW(s_edit, buf, 1024);
        val = buf;
    } else if (s_combo) {
        int sel = (int)SendMessageW(s_combo, CB_GETCURSEL, 0, 0);
        wchar_t buf[256] = L"";
        if (s_edit) {}
        if (sel >= 0) SendMessageW(s_combo, CB_GETLBTEXT, sel, (LPARAM)buf);
        else GetWindowTextW(s_combo, buf, 256);
        val = buf;
    }
    DestroyEditors();
    if (commit && row < (int)s_rows.size()) {
        ApplyRowValue(s_rows[row], val);
        FillRows(); SyncList();
    }
}

// --- doldurma -------------------------------------------------------------------
static void AddPropRow(const ElementDef* def, const wchar_t* name, const std::wstring& curVal, bool readOnly) {
    Row r;
    r.name = name;
    r.value = curVal;
    r.isEvent = false;
    if (def) {
        const PropDef* p = def->FindProp(name);
        if (p) { r.type = p->type; r.enumType = Catalog::EnumShortName(p->enumType); r.category = p->category; }
    }
    r.readOnly = readOnly;
    s_rows.push_back(r);
}
static void FillRows() {
    s_rows.clear();
    if (g_model.formSelected || !g_model.selected) {
        const ElementDef* def = g_catalog.Find(L"Form");
        wchar_t b[64];
        // öncelikli temel satırlar
        swprintf(b, 64, L"%d, %d", g_model.formW, g_model.formH);
        AddPropRow(def, L"Name", g_model.formProps.count(L"Name") ? g_model.formProps[L"Name"] : L"Form1", false);
        if (s_mode == 0) {
        AddPropRow(def, L"Text", g_model.formProps.count(L"Text") ? g_model.formProps[L"Text"] : L"Form1", false);
        AddPropRow(def, L"ClientSize", b, false);
        AddPropRow(def, L"BackColor", g_model.formProps.count(L"BackColor") ? g_model.formProps[L"BackColor"] : L"Control", false);
        AddPropRow(def, L"Font", g_model.formProps.count(L"Font") ? g_model.formProps[L"Font"] : L"Segoe UI, 9pt", false);
        AddPropRow(def, L"StartPosition", g_model.formProps.count(L"StartPosition") ? g_model.formProps[L"StartPosition"] : L"WindowsDefaultLocation", false);
        AddPropRow(def, L"MaximizeBox", g_model.formProps.count(L"MaximizeBox") ? g_model.formProps[L"MaximizeBox"] : L"True", false);
        AddPropRow(def, L"MinimizeBox", g_model.formProps.count(L"MinimizeBox") ? g_model.formProps[L"MinimizeBox"] : L"True", false);
        AddPropRow(def, L"FormBorderStyle", g_model.formProps.count(L"FormBorderStyle") ? g_model.formProps[L"FormBorderStyle"] : L"Sizable", false);
        }
        if (s_mode == 1) {
            for (auto& e : def->events) {
                if (!e.browsable) continue;
                Row r; r.name = e.name; r.isEvent = true;
                r.value = g_model.formEvents.count(e.name) ? g_model.formEvents[e.name] : L"";
                s_rows.push_back(r);
            }
        }
    } else {
        Ctrl* c = g_model.selected;
        const ElementDef* def = g_catalog.Find(c->type);
        wchar_t b1[64], b2[64];
        swprintf(b1, 64, L"%d, %d", c->x, c->y);
        swprintf(b2, 64, L"%d, %d", c->w, c->h);
        AddPropRow(def, L"Name", c->name, false);
        if (s_mode == 0) {
        AddPropRow(def, L"Text", c->Get(L"Text"), false);
        AddPropRow(def, L"Location", b1, false);
        AddPropRow(def, L"Size", b2, false);
        AddPropRow(def, L"Enabled", c->Get(L"Enabled", L"True"), false);
        AddPropRow(def, L"Visible", c->Get(L"Visible", L"True"), false);
        AddPropRow(def, L"Font", c->Get(L"Font", L"Segoe UI, 9pt"), false);
        AddPropRow(def, L"ForeColor", c->Get(L"ForeColor", L"ControlText"), false);
        AddPropRow(def, L"BackColor", c->Get(L"BackColor", L"Control"), false);
            // katalogun kalan browsable+canSet özellikleri
            if (def) {
                for (auto& p : def->props) {
                    if (!p.browsable || !p.canSet) continue;
                    std::wstring n = p.name;
                    if (n == L"Name" || n == L"Text" || n == L"Location" || n == L"Size" ||
                        n == L"Enabled" || n == L"Visible" || n == L"Font" || n == L"ForeColor" || n == L"BackColor")
                        continue;
                    AddPropRow(def, p.name.c_str(), c->Get(p.name.c_str(), p.def.c_str()), false);
                }
            }
        } else {
            if (def) {
                for (auto& e : def->events) {
                    if (!e.browsable) continue;
                    Row r; r.name = e.name; r.isEvent = true;
                    r.value = c->events.count(e.name) ? c->events[e.name] : L"";
                    s_rows.push_back(r);
                }
            }
        }
    }
}

// --- custom draw ----------------------------------------------------------------
static void SyncList() {
    ListView_DeleteAllItems(s_list);
    LVITEMW it = { 0 };
    it.mask = LVIF_TEXT | LVIF_PARAM;
    for (int i = 0; i < (int)s_rows.size(); i++) {
        it.iItem = i;
        it.lParam = (LPARAM)i;
        it.pszText = (LPWSTR)s_rows[i].name.c_str();
        int idx = ListView_InsertItem(s_list, &it);
        if (idx >= 0)
            ListView_SetItemText(s_list, idx, 1, (LPWSTR)s_rows[i].value.c_str());
    }
    InvalidateRect(s_list, nullptr, FALSE);
    // başlık: seçim bilgisi
    std::wstring cap;
    if (g_model.formSelected || !g_model.selected) cap = L"Form1  [System.Windows.Forms.Form]";
    else cap = g_model.selected->name + L"  [System.Windows.Forms." + g_model.selected->type + L"]";
    InvalidateRect(s_caption, nullptr, TRUE);
}

static LRESULT ListCustomDraw(NMLVCUSTOMDRAW* ncd) {
    switch (ncd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT: {
        int row = (int)ncd->nmcd.dwItemSpec;
        if (row < 0 || row >= (int)s_rows.size()) return CDRF_DODEFAULT;
        // satır zemini
        HDC dc = ncd->nmcd.hdc;
        RECT rc; ListView_GetItemRect(s_list, row, &rc, LVIR_BOUNDS);
        RECT rc1; ListView_GetSubItemRect(s_list, row, 1, LVIR_BOUNDS, &rc1);
        // kategori çizgisi: kategori değişince kalın ayraç (basit)
        Row& r = s_rows[row];
        bool sel = (ncd->nmcd.uItemState & CDIS_SELECTED) != 0;
        HBRUSH br = CreateSolidBrush(sel ? RGB(204, 232, 255) : (row % 2 ? RGB(255, 255, 255) : RGB(248, 248, 248)));
        FillRect(dc, &rc, br); DeleteObject(br);
        // kolon 0: ad
        RECT c0; ListView_GetSubItemRect(s_list, row, 0, LVIR_BOUNDS, &c0);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(60, 60, 60));
        RECT t0 = c0; t0.left += 6; t0.right = c0.right - 4;
        DrawTextW(dc, r.name.c_str(), -1, &t0, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        // kolon 1: değer
        SetTextColor(dc, r.isEvent ? (r.value.empty() ? RGB(150, 150, 150) : RGB(0, 90, 158)) : RGB(0, 0, 0));
        RECT t1 = rc1; t1.left += 6;
        std::wstring disp = r.value.empty() && r.isEvent ? L"(çift tıkla)" : r.value;
        DrawTextW(dc, disp.c_str(), -1, &t1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        // ayırıcı çizgi
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(235, 235, 235));
        HPEN op = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, rc.left, rc.bottom - 1, nullptr); LineTo(dc, rc.right, rc.bottom - 1);
        SelectObject(dc, op); DeleteObject(pen);
        return CDRF_SKIPDEFAULT;
    }
    }
    return CDRF_DODEFAULT;
}

// --- panel prosedürü -------------------------------------------------------------
static LRESULT CALLBACK PanelProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)l;
        if (nm->idFrom == IDC_PROPS_LIST) {
            if (nm->code == NM_CUSTOMDRAW) return ListCustomDraw((NMLVCUSTOMDRAW*)l);
            if (nm->code == NM_CLICK || nm->code == NM_DBLCLK) {
                NMITEMACTIVATE* ia = (NMITEMACTIVATE*)l;
                if (ia->iItem < 0 || ia->iItem >= (int)s_rows.size()) return 0;
                if (ia->iItem >= 0 && ia->iSubItem == 1) {
                    if (nm->code == NM_DBLCLK && s_rows[ia->iItem].isEvent) {
                        Row& r = s_rows[ia->iItem];
                        // varsayılan handler adı üret
                        std::wstring def;
                        if (g_model.formSelected || !g_model.selected) def = L"Form1_" + r.name;
                        else def = g_model.HandlerNameOf(g_model.selected, r.name);
                        if (r.value.empty()) { ApplyRowValue(r, def); FillRows(); SyncList(); }
                        wchar_t* heap = _wcsdup(r.name.c_str());
                        PostMessageW(g_hMain, MWM_EVENT_DBLCLICK, (WPARAM)(g_model.selected), (LPARAM)heap);
                        return 0;
                    }
                    BeginEdit(h, ia->iItem);
                } else EndEdit(h, true);
                return 0;
            }
        }
        if (nm->idFrom == 1150 && nm->code == CBN_SELCHANGE) { EndEdit(h, true); return 0; }
        if (nm->idFrom == 1150 && nm->code == CBN_KILLFOCUS) { EndEdit(h, true); return 0; }
        if (nm->idFrom == 1151 && nm->code == EN_KILLFOCUS) { EndEdit(h, true); return 0; }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id == IDC_PROPS_TAB || id == IDC_PROPS_TAB + 1) {
            s_mode = (id == IDC_PROPS_TAB) ? 0 : 1;
            EndEdit(h, false);
            FillRows(); SyncList();
            InvalidateRect(s_caption, nullptr, TRUE);
            return 0;
        }
        break;
    }
    case WM_VSCROLL: EndEdit(h, true); break;
    case WM_SIZE: {
        RECT rc; GetClientRect(h, &rc);
        MoveWindow(s_caption, 0, 0, rc.right, 26, TRUE);
        int bw = (rc.right - 8) / 2;
        MoveWindow(s_tab, 2, 28, bw, 26, TRUE);
        MoveWindow(s_tab2, 6 + bw, 28, bw, 26, TRUE);
        MoveWindow(s_list, 4, 58, rc.right - 8, rc.bottom - 62, TRUE);
        RECT lv; GetClientRect(s_list, &lv);
        ListView_SetColumnWidth(s_list, 0, (lv.right - 8) * 45 / 100);
        ListView_SetColumnWidth(s_list, 1, (lv.right - 8) * 55 / 100);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH br = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(dc, &rc, br); DeleteObject(br);
        RECT cap = { 0, 0, rc.right, 26 };
        HBRUSH capb = CreateSolidBrush(RGB(0, 120, 215));
        FillRect(dc, &cap, capb); DeleteObject(capb);
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255, 255, 255));
        SelectObject(dc, g_fontUIBold);
        std::wstring t = s_mode == 0 ? L"Özellikler" : L"Olaylar";
        RECT tr = cap; tr.left += 10;
        DrawTextW(dc, t.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        // seçim etiketi
        SelectObject(dc, g_fontUI);
        std::wstring cap2;
        if (g_model.formSelected || !g_model.selected) {
            std::wstring fn = g_model.formProps.count(L"Name") ? g_model.formProps[L"Name"] : L"Form1";
            cap2 = fn + L" — System.Windows.Forms.Form";
        }
        else cap2 = g_model.selected->name + L" — " + g_model.selected->type;
        RECT er = { 0, 28, rc.right, 50 };
        SetTextColor(dc, RGB(40, 40, 40));
        DrawTextW(dc, cap2.c_str(), -1, &er, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        EndPaint(h, &ps);
        return 0;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

HWND Create(HWND parent) {
    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = PanelProc; wc.hInstance = g_hInst; wc.hbrBackground = nullptr;
        wc.lpszClassName = L"M2GOD_PROPS";
        RegisterClassW(&wc);
        reg = true;
    }
    HWND h = CreateWindowExW(0, L"M2GOD_PROPS", L"", WS_CHILD | WS_VISIBLE,
                             0, 0, PROPS_W, 200, parent, (HMENU)1510, g_hInst, nullptr);
    s_panel = h;
    s_caption = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, PROPS_W, 26, h, nullptr, g_hInst, nullptr);
    // sekme yerine güvenilir toggle butonlar
    s_tab = CreateWindowExW(0, L"BUTTON", L"Özellikler", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            2, 28, 145, 26, h, (HMENU)IDC_PROPS_TAB, g_hInst, nullptr);
    s_tab2 = CreateWindowExW(0, L"BUTTON", L"Olaylar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                             149, 28, 145, 26, h, (HMENU)(IDC_PROPS_TAB + 1), g_hInst, nullptr);
    SendMessageW(s_tab, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(s_tab2, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    s_list = CreateWindowExW(0, WC_LISTVIEWW, L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                             4, 58, PROPS_W - 8, 150, h, (HMENU)IDC_PROPS_LIST, g_hInst, nullptr);
    ListView_SetExtendedListViewStyle(s_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    SendMessageW(s_list, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    LVCOLUMNW col = { 0 }; col.mask = LVCF_TEXT | LVCF_WIDTH;
    wchar_t c1[] = L"Özellik", c2[] = L"Değer";
    col.pszText = c1; col.cx = 130; ListView_InsertColumn(s_list, 0, &col);
    col.pszText = c2; col.cx = 160; ListView_InsertColumn(s_list, 1, &col);
    SendMessageW(s_caption, WM_SETFONT, (WPARAM)g_fontUIBold, TRUE);
    FillRows(); SyncList();
    return h;
}

void Refresh(HWND panel) {
    (void)panel;
    EndEdit(s_panel, false);
    FillRows(); SyncList();
    InvalidateRect(s_caption, nullptr, TRUE);
}

// SÜRÜKLEME SIRASINDA hafif güncelleme: liste yeniden kurulmaz (flicker yok),
// sadece Location/Size satırlarının metni güncellenir
void UpdateLiveGeometry() {
    if (g_model.formSelected || !g_model.selected) return;
    Ctrl* c = g_model.selected;
    wchar_t b1[64], b2[64];
    swprintf(b1, 64, L"%d, %d", c->x, c->y);
    swprintf(b2, 64, L"%d, %d", c->w, c->h);
    int n = (int)s_rows.size();
    for (int i = 0; i < n; i++) {
        if (s_rows[i].isEvent) continue;
        if (s_rows[i].name == L"Location") {
            s_rows[i].value = b1;
            ListView_SetItemText(s_list, i, 1, b1);
        } else if (s_rows[i].name == L"Size") {
            s_rows[i].value = b2;
            ListView_SetItemText(s_list, i, 1, b2);
        }
    }
    InvalidateRect(s_list, nullptr, FALSE);
}

void ApplyPending(HWND panel) {
    (void)panel;
    EndEdit(s_panel, true);
}

} // namespace props
