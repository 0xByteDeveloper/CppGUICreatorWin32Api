// props.cpp — Özellikler & Olaylar paneli gerçeklemesi
#include "props.h"
#include "app.h"
#include "catalog.h"
#include "model.h"
#include "factory.h"
#include "surface.h"
#include <commctrl.h>
#include <commdlg.h>

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
static int s_selRow = -1; // kendi takip ettiğimiz seçim (ListView state'ine güvenmiyoruz)

// ERTelenmiş commit: combo bildirimi (CBN_SELCHANGE/SELENDOK/KILLFOCUS) işlenirken
// DestroyWindow ÇAĞRILMAZ — Windows bunu davranış bozukluğu yapar ("comboboxa
// tıklayınca kayboluyor" şikayetinin kök nedeni). Commit, bildirim bitince kuyruktan gelir.
static const UINT MWM_PROPS_COMMIT = WM_APP + 21;
static UINT s_commitSeq = 0;   // bayat commit mesajlarını geçersiz kılmak için
static void PostCommit(HWND panel) { s_commitSeq++; PostMessageW(panel, MWM_PROPS_COMMIT, s_commitSeq, 0); }

// GÜVENLİ metin güncelleyici: ListView_SetItemText makrosu maskesiz (uninitialized)
// LV_ITEM gönderir; çöp mask LVIF_STATE biti içerirse rastgele LVIS_SELECTED uygulanır
// ("tüm satırlar seçili" hatasının kök nedeni). Burada maske açıkça LVIF_TEXT.
static void SetRowText(int row, int col, const std::wstring& t) {
    if (row < 0 || row >= (int)s_rows.size()) return;
    LVITEMW lv = { 0 };
    lv.mask = LVIF_TEXT;
    lv.iItem = row;
    lv.iSubItem = col;
    lv.pszText = (LPWSTR)t.c_str();
    SendMessageW(s_list, LVM_SETITEMTEXTW, (WPARAM)row, (LPARAM)&lv);
}

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
static void SubclassEditor(HWND h);   // satır içi edit/combo alt sınıfı (Enter/ESC)
static void SubclassComboInner(HWND combo);   // CBS_DROPDOWN iç edit'i (Enter/ESC)

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
    // FAZ 2.6: TabPages koleksiyon satırı → Koleksiyon Düzenleyici'yi aç
    // (VS "TabPage Collection Editor" karşılığı)
    if (_wcsicmp(r.name.c_str(), L"TabPages") == 0) {
        if (g_model.selected && _wcsicmp(g_model.selected->type.c_str(), L"TabControl") == 0)
            OpenTabPagesEditor(g_model.selected);
        return;
    }
    // FAZ 2.6.3: Items/Columns/Nodes koleksiyon satırları → Koleksiyon Düzenleyici
    // (VS "String Collection Editor" karşılığı)
    if (_wcsicmp(r.name.c_str(), L"Items") == 0 ||
        _wcsicmp(r.name.c_str(), L"Columns") == 0 ||
        _wcsicmp(r.name.c_str(), L"Nodes") == 0) {
        if (g_model.selected) OpenCollectionEditor(g_model.selected, r.name);
        return;
    }
    RECT rc;
    if (!ListView_GetSubItemRect(s_list, row, 1, LVIR_BOUNDS, &rc)) return;
    // GetSubItemRect liste-client koordinatı verir; edit panel'in çocuğu → panele çevir
    // (yoksa kutu satırın çok üstünde, "aşırı yukarda" belirir)
    MapWindowPoints(s_list, panel, (LPPOINT)&rc, 2);
    bool isColor = r.type.find(L"Color") != std::wstring::npos;
    bool isFont  = r.type.find(L"Font") != std::wstring::npos;
    if (r.isEvent || isColor || isFont ||
        _wcsicmp(r.type.c_str(), L"System.String") == 0 || r.type.empty() ||
        _wcsicmp(r.type.c_str(), L"System.Int32") == 0 ||
        r.type.find(L"Point") != std::wstring::npos || r.type.find(L"Size") != std::wstring::npos) {
        if (isColor) {
            // RENK: açılır liste + "Özel renk..." (ChooseColor)
            s_combo = CreateWindowExW(0, L"COMBOBOX", r.value.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN, rc.left, rc.top, rc.right - rc.left, 240,
                panel, (HMENU)1150, g_hInst, nullptr);
            const wchar_t* colors[] = { L"Black", L"White", L"Red", L"Lime", L"Blue", L"Yellow", L"Cyan", L"Magenta",
                                        L"Gray", L"Silver", L"Orange", L"Control", L"Window", L"Özel renk..." };
            for (auto* cn : colors) SendMessageW(s_combo, CB_ADDSTRING, 0, (LPARAM)cn);
            SendMessageW(s_combo, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
            SubclassEditor(s_combo);
            SubclassComboInner(s_combo);
            SetFocus(s_combo);
            s_editRow = row; s_editCol = 1;
            // FAZ 2.4: tek tık — hücreye tıklayınca açılır liste HEMEN açılır
            PostMessageW(s_combo, CB_SHOWDROPDOWN, TRUE, 0);
            return;
        }
        if (isFont) {
            // YAZI TİPİ: açılır liste + "Yazı tipi seç..." (ChooseFont)
            s_combo = CreateWindowExW(0, L"COMBOBOX", r.value.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN, rc.left, rc.top, rc.right - rc.left, 240,
                panel, (HMENU)1150, g_hInst, nullptr);
            const wchar_t* fonts[] = { L"Segoe UI, 9pt", L"Segoe UI, 10pt", L"Segoe UI, 9pt, style=Bold", L"Tahoma, 8pt",
                                        L"Arial, 10pt", L"Calibri, 11pt", L"Consolas, 10pt", L"Courier New, 10pt", L"Times New Roman, 10pt", L"Yazı tipi seç..." };
            for (auto* fn : fonts) SendMessageW(s_combo, CB_ADDSTRING, 0, (LPARAM)fn);
            SendMessageW(s_combo, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
            SubclassEditor(s_combo);
            SubclassComboInner(s_combo);
            SetFocus(s_combo);
            s_editRow = row; s_editCol = 1;
            // FAZ 2.4: tek tık — açılır liste hemen açılır
            PostMessageW(s_combo, CB_SHOWDROPDOWN, TRUE, 0);
            return;
        }
        s_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", r.value.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            panel, (HMENU)1151, g_hInst, nullptr);
        SendMessageW(s_edit, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
        SubclassEditor(s_edit);
        SetFocus(s_edit);
        // VS gibi: açılışta tüm metin seçili — yazmaya başlayınca eskisi değişir
        PostMessageW(s_edit, EM_SETSEL, 0, -1);
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
        SubclassEditor(s_combo);
        SetFocus(s_combo);
        s_editRow = row; s_editCol = 1;
        // FAZ 2.4: tek tık — enum/bool hücresine tıklayınca açılır liste HEMEN açılır
        // (kullanıcı artık ikinci kez komboya tıklamak zorunda kalmaz)
        PostMessageW(s_combo, CB_SHOWDROPDOWN, TRUE, 0);
    }
}

// satır içi düzenleyici alt sınıfı: ENTER işle, ESC vazgeç (VS davranışı)
static LRESULT CALLBACK EditorSubProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    WNDPROC old = (WNDPROC)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (m == WM_KEYDOWN && (w == VK_RETURN || w == VK_ESCAPE)) {
        EndEdit(s_panel, w == VK_RETURN);
        return 0;
    }
    if (m == WM_GETDLGCODE && w == VK_RETURN) return DLGC_WANTALLKEYS;
    return old ? CallWindowProcW(old, h, m, w, l) : DefWindowProcW(h, m, w, l);
}
static void SubclassEditor(HWND h) {
    WNDPROC old = (WNDPROC)SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)EditorSubProc);
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)old);
}
// CBS_DROPDOWN kombonun İÇ edit'ini de alt sınıfla — ENTER/ESC içeri düşmesin
static void SubclassComboInner(HWND combo) {
    HWND inner = FindWindowExW(combo, nullptr, L"Edit", nullptr);
    if (inner) SubclassEditor(inner);
}

static void EndEdit(HWND panel, bool commit) {
    if (s_editRow < 0) return;
    int row = s_editRow;
    Row rowInfo;
    if (row < (int)s_rows.size()) rowInfo = s_rows[row];
    std::wstring val;
    if (s_edit) {
        wchar_t buf[1024]; GetWindowTextW(s_edit, buf, 1024);
        val = buf;
    } else if (s_combo) {
        int sel = (int)SendMessageW(s_combo, CB_GETCURSEL, 0, 0);
        wchar_t buf[256] = L"";
        if (sel >= 0) SendMessageW(s_combo, CB_GETLBTEXT, sel, (LPARAM)buf);
        else GetWindowTextW(s_combo, buf, 256);
        val = buf;
    }
    DestroyEditors();
    if (!commit || row >= (int)s_rows.size()) return;
    // özel düzenleyiciler
    if (rowInfo.type.find(L"Color") != std::wstring::npos && val == L"Özel renk...") {
        static COLORREF cust[16] = { 0 };
        CHOOSECOLORW cc = { sizeof(cc) };
        cc.hwndOwner = panel;
        cc.lpCustColors = cust;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        COLORREF init = RGB(0, 0, 0);
        if (ParseColor(rowInfo.value, init)) cc.rgbResult = init;
        if (ChooseColorW(&cc)) val = ColorToHex(cc.rgbResult);
        else return;   // vazgeç — değer değişmez
    }
    if (rowInfo.type.find(L"Font") != std::wstring::npos && val == L"Yazı tipi seç...") {
        std::wstring fn; int pt; bool bo, it, un;
        ParseFontValue(rowInfo.value.empty() ? L"Segoe UI, 9pt" : rowInfo.value, fn, pt, bo, it, un);
        LOGFONTW lf = { 0 };
        lstrcpynW(lf.lfFaceName, fn.c_str(), LF_FACESIZE);
        HDC tdc = GetDC(panel);
        lf.lfHeight = -MulDiv(pt, GetDeviceCaps(tdc, LOGPIXELSY), 72);
        ReleaseDC(panel, tdc);
        lf.lfWeight = bo ? FW_BOLD : FW_NORMAL;
        lf.lfItalic = it; lf.lfUnderline = un;
        CHOOSEFONTW cf = { sizeof(cf) };
        cf.hwndOwner = panel;
        cf.lpLogFont = &lf;
        cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS;
        if (ChooseFontW(&cf)) {
            val = std::wstring(lf.lfFaceName) + L", " + std::to_wstring(-MulDiv(lf.lfHeight, 72, GetDeviceCaps(GetDC(panel), LOGPIXELSY))) + L"pt";
            if (lf.lfWeight >= FW_BOLD) val += L", style=Bold";
            if (lf.lfItalic) val += L", style=Italic";
            if (lf.lfUnderline) val += L", style=Underline";
        } else return;   // vazgeç
    }
    ApplyRowValue(s_rows[row], val);
    FillRows(); SyncList();
    if (s_selRow >= (int)s_rows.size()) s_selRow = row < (int)s_rows.size() ? row : -1;
}

// --- doldurma -------------------------------------------------------------------
// FAZ 2.6.3: koleksiyon değeri satır sayısı ('\n' ile ayrılmış, boş satırlar sayılmaz)
static int CountCollectionLines(const std::wstring& v) {
    int n = 0;
    for (auto& ln : Split(v, L'\n')) if (!Trim(ln).empty()) n++;
    return n;
}
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
            // FAZ 2.6: TabControl → TabPages koleksiyon satırı (VS: "..." düğmesi)
            if (_wcsicmp(c->type.c_str(), L"TabControl") == 0) {
                Row tp;
                tp.name = L"TabPages";
                wchar_t pb[80];
                swprintf(pb, 80, L"<%d sayfa>", (int)TabPagesOf(c).size());
                tp.value = pb;
                tp.type = L"System.Windows.Forms.TabControl.TabPageCollection";
                tp.category = L"Data";
                s_rows.push_back(tp);
            }
            // FAZ 2.6.3: Items/Columns/Nodes koleksiyon satırları (VS "..." düğmesi)
            // Config_info'da bu koleksiyonlar browsable=true, can_set=false — genel
            // döngüye girmedikleri için panelde YOKTULAR ("ListBox'ta items yok"
            // hatasının kökü). VS'de bunlar Koleksiyon Düzenleyicisi ile düzenlenir.
            if (_wcsicmp(c->type.c_str(), L"ListBox") == 0 ||
                _wcsicmp(c->type.c_str(), L"CheckedListBox") == 0 ||
                _wcsicmp(c->type.c_str(), L"ComboBox") == 0) {
                Row r;
                r.name = L"Items";
                r.type = L"System.Windows.Forms." + c->type + L"+ObjectCollection";
                int n = CountCollectionLines(c->Get(L"Items"));
                wchar_t nb[32]; swprintf(nb, 32, L"<%d öğe>", n);
                r.value = nb; r.category = L"Data";
                s_rows.push_back(r);
            } else if (_wcsicmp(c->type.c_str(), L"ListView") == 0) {
                Row rc;
                rc.name = L"Columns";
                rc.type = L"System.Windows.Forms.ListView+ColumnHeaderCollection";
                int nc = CountCollectionLines(c->Get(L"Columns"));
                wchar_t cb[32]; swprintf(cb, 32, L"<%d sütun>", nc);
                rc.value = cb; rc.category = L"Data";
                s_rows.push_back(rc);
                Row ri;
                ri.name = L"Items";
                ri.type = L"System.Windows.Forms.ListView+ListViewItemCollection";
                int ni = CountCollectionLines(c->Get(L"Items"));
                wchar_t ib[32]; swprintf(ib, 32, L"<%d satır>", ni);
                ri.value = ib; ri.category = L"Data";
                s_rows.push_back(ri);
            } else if (_wcsicmp(c->type.c_str(), L"TreeView") == 0) {
                Row rn;
                rn.name = L"Nodes";
                rn.type = L"System.Windows.Forms.TreeNodeCollection";
                int nn = CountCollectionLines(c->Get(L"Nodes"));
                wchar_t nb2[32]; swprintf(nb2, 32, L"<%d düğüm>", nn);
                rn.value = nb2; rn.category = L"Data";
                s_rows.push_back(rn);
            }
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
            SetRowText(idx, 1, s_rows[i].value);
    }
    // seçim durumunu AÇIKÇA kur: ekleme sonrası çöp LVIS durumları kalabilir
    ListView_SetItemState(s_list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (s_selRow >= 0 && s_selRow < (int)s_rows.size())
        ListView_SetItemState(s_list, s_selRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
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
        bool sel = (row == s_selRow);   // kendi takibimiz — uItemState'e güvenme
        HBRUSH br = CreateSolidBrush(sel ? RGB(204, 232, 255) : (row % 2 ? RGB(255, 255, 255) : RGB(248, 248, 248)));
        FillRect(dc, &rc, br); DeleteObject(br);
        // kolon 0: ad — NOT: GetSubItemRect(iSubItem=0) SATIRIN TAMAMINI döndürür;
        // kolon-0 genişliğini başlıktan al (uzun ad değerin üzerine taşmasın)
        int w0 = ListView_GetColumnWidth(s_list, 0);
        RECT c0 = rc; c0.right = c0.left + w0;
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
    // ertelenmiş commit: combo/edit bildirimi bittikten sonra güvenli uygulanır
    if (m == MWM_PROPS_COMMIT) {
        if (w == s_commitSeq) EndEdit(h, true);   // bayat mesajlar yok sayılır
        return 0;
    }
    switch (m) {
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)l;
        if (nm->idFrom == IDC_PROPS_LIST) {
            if (nm->code == NM_CUSTOMDRAW) return ListCustomDraw((NMLVCUSTOMDRAW*)l);
            if (nm->code == LVN_BEGINSCROLL) {
                // liste kaydırılırsa açık düzenleyici eski satırda kalır → kapat ve işle
                EndEdit(h, true);
                return 0;
            }
            if (nm->code == LVN_ITEMCHANGED) {
                NMLISTVIEW* nl = (NMLISTVIEW*)l;
                if (nl->uChanged & LVIF_STATE) {
                    int r = (int)nl->iItem;
                    bool nowSel = (nl->uNewState & LVIS_SELECTED) != 0;
                    if (nowSel && r >= 0 && r < (int)s_rows.size()) s_selRow = r;
                    else if (!nowSel && r == s_selRow && !(nl->uOldState & LVIS_SELECTED)) { /* yoksay */ }
                }
                return 0;
            }
            if (nm->code == NM_CLICK || nm->code == NM_DBLCLK) {
                NMITEMACTIVATE* ia = (NMITEMACTIVATE*)l;
                if (ia->iItem < 0 || ia->iItem >= (int)s_rows.size()) return 0;
                s_selRow = ia->iItem;
                if (ia->iItem >= 0 && ia->iSubItem == 1) {
                    // ÖNCE olay çift tıkı: açık düzenleyiciyi kapatıp handler üret
                    // (aynı satır koruması olay üretimini engellememeli)
                    if (nm->code == NM_DBLCLK && s_rows[ia->iItem].isEvent) {
                        Row& r = s_rows[ia->iItem];
                        // varsayılan handler adı üret
                        std::wstring def;
                        if (g_model.formSelected || !g_model.selected) def = L"Form1_" + r.name;
                        else def = g_model.HandlerNameOf(g_model.selected, r.name);
                        EndEdit(h, false);
                        if (r.value.empty()) { ApplyRowValue(r, def); FillRows(); SyncList(); }
                        wchar_t* heap = _wcsdup(r.name.c_str());
                        PostMessageW(g_hMain, MWM_EVENT_DBLCLICK, (WPARAM)(g_model.selected), (LPARAM)heap);
                        return 0;
                    }
                    if (s_editRow == ia->iItem && (s_edit || s_combo)) return 0; // aynı satır — düzenlemeye devam
                    BeginEdit(h, ia->iItem);
                } else EndEdit(h, true);
                return 0;
            }
        }
        // NOT: CBN_SELCHANGE / CBN_KILLFOCUS / EN_KILLFOCUS WM_COMMAND ile gelir!
        // (eski kod bunları WM_NOTIFY içinde arıyordu — hiç tetiklenmiyordu:
        //  dropdown seçimi ve tıklayarak ayrılma işlemi hiç uygulanmıyordu)
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(w), note = HIWORD(w);
        if (id == IDC_PROPS_TAB || id == IDC_PROPS_TAB + 1) {
            s_mode = (id == IDC_PROPS_TAB) ? 0 : 1;
            EndEdit(h, true);   // sekme değişince açık düzenleyiciyi işle (bırakma değil)
            FillRows(); SyncList();
            InvalidateRect(s_caption, nullptr, TRUE);
            return 0;
        }
        if (id == 1150) {
            // dropdown'dan seçim / kapanış / odak kaybı → ERTelenmiş commit
            // (bildirim içinde combo'yu yok etmek yerine kuyruğa bırakılır)
            if (note == CBN_SELCHANGE || note == CBN_SELENDOK) {
                PostCommit(h);
                return 0;
            }
            if (note == CBN_KILLFOCUS) {
                // açılır liste hâlâ açıksa bu odak geçişi normaldir — commit YOK
                if (s_combo && SendMessageW(s_combo, CB_GETDROPPEDSTATE, 0, 0)) return 0;
                PostCommit(h);
                return 0;
            }
            if (note == CBN_CLOSEUP || note == CBN_DROPDOWN) return 0;
        }
        if (id == 1151 && note == EN_KILLFOCUS) {
            PostCommit(h);   // metin düzenleyiciden tıklayarak ayrılma → ertelenmiş uygula
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
    s_selRow = -1; // satır kümesi değişti — eski seçim anlamsız
    FillRows(); SyncList();
    InvalidateRect(s_caption, nullptr, TRUE);
}

bool IsEditing() {
    return s_editRow >= 0 && (s_edit || s_combo);
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
            SetRowText(i, 1, b1);
        } else if (s_rows[i].name == L"Size") {
            s_rows[i].value = b2;
            SetRowText(i, 1, b2);
        }
    }
    InvalidateRect(s_list, nullptr, FALSE);
}

void ApplyPending(HWND panel) {
    (void)panel;
    EndEdit(s_panel, true);
}

// ==================== FAZ 2.6: TabPage Koleksiyon Düzenleyicisi ====================
// VS "TabPage Collection Editor" karşılığı: sol üye listesi (Ekle/Kaldır/Yukarı/
// Aşağı), sağda seçili sayfanın özellikleri (Ad, Metin, Arka Plan, Etkin, Araç
// İpucu). İptal → açılış anındaki model anlık görüntüsüne dönülür.
namespace tabed {

enum {
    IDE_LIST   = 3101,
    IDE_ADD    = 3102,
    IDE_DEL    = 3103,
    IDE_UP     = 3104,
    IDE_DOWN   = 3105,
    IDE_NAME   = 3106,
    IDE_TEXT   = 3107,
    IDE_BACK   = 3108,
    IDE_BACKBTN= 3109,
    IDE_ENAB   = 3110,
    IDE_TIP    = 3111,
    IDE_OK     = 3112,
    IDE_CANCEL = 3113,
};

static Ctrl* s_edTab = nullptr;
static HWND  s_dlg = nullptr;
static HWND  s_lb = nullptr, s_eName = nullptr, s_eText = nullptr, s_eBack = nullptr,
             s_cEnab = nullptr, s_eTip = nullptr;
static std::vector<Ctrl*> s_pages;
static int  s_cur = -1;
static bool s_done = false, s_ok = false;
static std::string s_snapshot;               // iptal için model anlık görüntüsü

static std::vector<Ctrl*> Pages() { return TabPagesOf(s_edTab); }

static void RefreshSurface() {
    HWND surf = GetDlgItem(g_hMain, IDC_SURFACE);
    surface::RecreateAll(surf);
    factory::UpdateTabPages(surf, s_edTab);
}

// açık alan düzenlemelerini seçili sayfaya uygula
static void CommitFields() {
    if (s_cur < 0 || s_cur >= (int)s_pages.size()) return;
    Ctrl* pg = s_pages[s_cur];
    if (!pg) return;
    wchar_t buf[512] = L"";
    auto apply = [&](HWND h, const wchar_t* prop, bool trim) {
        GetWindowTextW(h, buf, 512);
        std::wstring v = trim ? Trim(buf) : std::wstring(buf);
        Ctrl* c = g_model.selected;
        g_model.selected = pg;
        // ApplyRowValue benzeri: Name özel, diğerleri ApplyProp
        if (_wcsicmp(prop, L"Name") == 0) {
            if (!v.empty() && _wcsicmp(v.c_str(), pg->name.c_str()) != 0) {
                std::wstring nv = v;
                for (auto& ch : nv) if (!iswalnum(ch) && ch != L'_') ch = L'_';
                if (!nv.empty() && iswdigit(nv[0])) nv = L"_" + nv;
                g_model.Rename(pg, nv);
            }
        } else if (_wcsicmp(prop, L"Enabled") == 0) {
            pg->props[L"Enabled"] = v;
            factory::ApplyProp(pg, L"Enabled", v);
        } else {
            pg->props[prop] = v;
            factory::ApplyProp(pg, prop, v);
        }
        g_model.selected = c;
    };
    apply(s_eName, L"Name", true);
    apply(s_eText, L"Text", false);
    apply(s_eBack, L"BackColor", true);
    int en = (int)SendMessageW(s_cEnab, CB_GETCURSEL, 0, 0);
    if (en >= 0) {
        pg->props[L"Enabled"] = (en == 0) ? L"True" : L"False";
        factory::ApplyProp(pg, L"Enabled", pg->props[L"Enabled"]);
    }
    apply(s_eTip, L"ToolTipText", false);
    SyncTabText(s_edTab);
}

static void LoadSel() {
    if (s_cur < 0 || s_cur >= (int)s_pages.size()) {
        for (HWND h : { s_eName, s_eText, s_eBack, s_eTip }) { SetWindowTextW(h, L""); EnableWindow(h, FALSE); }
        EnableWindow(s_cEnab, FALSE);
        return;
    }
    Ctrl* pg = s_pages[s_cur];
    for (HWND h : { s_eName, s_eText, s_eBack, s_cEnab, s_eTip }) EnableWindow(h, TRUE);
    SetWindowTextW(s_eName, pg->name.c_str());
    SetWindowTextW(s_eText, pg->Get(L"Text").c_str());
    SetWindowTextW(s_eBack, pg->Get(L"BackColor", L"Control").c_str());
    SendMessageW(s_cEnab, CB_SETCURSEL,
                 _wcsicmp(pg->Get(L"Enabled", L"True").c_str(), L"true") == 0 ? 0 : 1, 0);
    SetWindowTextW(s_eTip, pg->Get(L"ToolTipText").c_str());
}

static void RebuildList(int keepSel) {
    s_pages = Pages();
    SendMessageW(s_lb, LB_RESETCONTENT, 0, 0);
    for (auto* p : s_pages) {
        std::wstring row = p->name + L"   [" + p->Get(L"Text", L"") + L"]";
        SendMessageW(s_lb, LB_ADDSTRING, 0, (LPARAM)row.c_str());
    }
    int sel = keepSel;
    if (sel >= (int)s_pages.size()) sel = (int)s_pages.size() - 1;
    if (sel < 0 && !s_pages.empty()) sel = 0;
    SendMessageW(s_lb, LB_SETCURSEL, sel >= 0 ? (WPARAM)sel : (WPARAM)-1, 0);
    s_cur = sel;
    LoadSel();
}

static void MovePage(int from, int dir) {
    int to = from + dir;
    if (from < 0 || to < 0 || from >= (int)s_pages.size() || to >= (int)s_pages.size()) return;
    // ctrls listesindeki slotları takas et (diğer kontroller yerinde kalır)
    std::vector<int> slots;
    for (auto* p : s_pages) {
        for (int i = 0; i < (int)g_model.ctrls.size(); i++)
            if (g_model.ctrls[i] == p) { slots.push_back(i); break; }
    }
    if (slots.size() != s_pages.size()) return;
    std::swap(g_model.ctrls[slots[from]], g_model.ctrls[slots[to]]);
    std::swap(s_pages[from], s_pages[to]);
    SyncTabText(s_edTab);
    RefreshSurface();
    RebuildList(to);
    InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, TRUE);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
}

static LRESULT CALLBACK TabEdProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_COMMAND: {
        int id = LOWORD(w), note = HIWORD(w);
        if (id == IDE_OK)   { CommitFields(); s_ok = true; s_done = true; return 0; }
        if (id == IDE_CANCEL) { s_ok = false; s_done = true; return 0; }
        if (id == IDE_LIST && note == LBN_SELCHANGE) {
            CommitFields();   // önce açık alanları yaz
            s_cur = (int)SendMessageW(s_lb, LB_GETCURSEL, 0, 0);
            LoadSel();
            return 0;
        }
        if (id == IDE_ADD) {
            CommitFields();
            Ctrl* pg = g_model.AddPage(s_edTab, L"");
            if (pg) {
                pg->props[L"Text"] = pg->name;
                SyncTabText(s_edTab);
                RefreshSurface();
                RebuildList((int)Pages().size() - 1);
                PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
            }
            return 0;
        }
        if (id == IDE_DEL) {
            if (s_cur >= 0 && s_cur < (int)s_pages.size()) {
                Ctrl* victim = s_pages[s_cur];
                g_model.RemovePage(s_edTab, victim);
                RefreshSurface();
                RebuildList(s_cur);
                PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
            }
            return 0;
        }
        if (id == IDE_UP)   { CommitFields(); MovePage(s_cur, -1); return 0; }
        if (id == IDE_DOWN) { CommitFields(); MovePage(s_cur, +1); return 0; }
        if (id == IDE_BACKBTN) {
            static COLORREF cust[16] = { 0 };
            CHOOSECOLORW cc = { sizeof(cc) };
            cc.hwndOwner = h;
            cc.lpCustColors = cust;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            COLORREF init = RGB(255, 255, 255);
            wchar_t b[64]; GetWindowTextW(s_eBack, b, 64);
            ParseColor(b, init);
            cc.rgbResult = init;
            if (ChooseColorW(&cc)) {
                std::wstring hex = ColorToHex(cc.rgbResult);
                SetWindowTextW(s_eBack, hex.c_str());
                CommitFields();
                InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, TRUE);
            }
            return 0;
        }
        // alan düzenlemeleri odak kaybında modele yazılır
        if ((id == IDE_NAME || id == IDE_TEXT || id == IDE_BACK || id == IDE_TIP) && note == EN_KILLFOCUS) {
            CommitFields();
            InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, TRUE);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        s_ok = false; s_done = true;
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static HWND MkCtrl(HWND parent, const wchar_t* cls, const wchar_t* txt, DWORD style,
                   int x, int y, int w, int h, int id) {
    HWND hc = CreateWindowExW(0, cls, txt, WS_CHILD | WS_VISIBLE | style,
                              x, y, w, h, parent, (HMENU)(INT_PTR)id, g_hInst, nullptr);
    SendMessageW(hc, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    return hc;
}

} // namespace tabed

void OpenTabPagesEditor(Ctrl* tab) {
    using namespace tabed;
    if (!tab || _wcsicmp(tab->type.c_str(), L"TabControl") != 0) return;
    s_edTab = tab;
    s_done = false; s_ok = false;
    s_snapshot = json::Stringify(g_model.ToJson(), false);
    g_model.PushUndo();

    RECT orc; GetWindowRect(g_hMain, &orc);
    int dw = 600, dh = 470;
    int dx = orc.left + ((orc.right - orc.left) - dw) / 2;
    int dy = orc.top + ((orc.bottom - orc.top) - dh) / 2;

    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = TabEdProc; wc.hInstance = g_hInst;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"M2GOD_TABED";
        RegisterClassW(&wc);
        reg = true;
    }
    s_dlg = CreateWindowExW(0, L"M2GOD_TABED", L"TabPage Koleksiyon Düzenleyicisi",
                            WS_POPUP | WS_CAPTION | WS_SYSMENU,
                            dx, dy, dw, dh, g_hMain, nullptr, g_hInst, nullptr);
    // sol: üye listesi
    MkCtrl(s_dlg, L"STATIC", L"Üyeler:", SS_LEFT, 14, 12, 200, 18, 0);
    s_lb = MkCtrl(s_dlg, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                  LBS_NOTIFY | LBS_HASSTRINGS | WS_BORDER, 14, 34, 250, 330, IDE_LIST);
    MkCtrl(s_dlg, L"BUTTON", L"Ekle", WS_TABSTOP | BS_PUSHBUTTON, 14, 374, 74, 28, IDE_ADD);
    MkCtrl(s_dlg, L"BUTTON", L"Kaldır", WS_TABSTOP | BS_PUSHBUTTON, 94, 374, 74, 28, IDE_DEL);
    MkCtrl(s_dlg, L"BUTTON", L"Yukarı", WS_TABSTOP | BS_PUSHBUTTON, 174, 374, 74, 28, IDE_UP);
    MkCtrl(s_dlg, L"BUTTON", L"Aşağı", WS_TABSTOP | BS_PUSHBUTTON, 254, 374, 74, 28, IDE_DOWN);
    // sağ: sayfa özellikleri
    MkCtrl(s_dlg, L"STATIC", L"TabPage özellikleri:", SS_LEFT, 290, 12, 250, 18, 0);
    MkCtrl(s_dlg, L"STATIC", L"(Ad) Name", SS_LEFT, 290, 44, 110, 18, 0);
    s_eName = MkCtrl(s_dlg, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 404, 40, 170, 24, IDE_NAME);
    MkCtrl(s_dlg, L"STATIC", L"Metin (Text)", SS_LEFT, 290, 74, 110, 18, 0);
    s_eText = MkCtrl(s_dlg, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 404, 70, 170, 24, IDE_TEXT);
    MkCtrl(s_dlg, L"STATIC", L"Arka Plan", SS_LEFT, 290, 104, 110, 18, 0);
    s_eBack = MkCtrl(s_dlg, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 404, 100, 130, 24, IDE_BACK);
    MkCtrl(s_dlg, L"BUTTON", L"... ", WS_TABSTOP | BS_PUSHBUTTON, 540, 100, 34, 24, IDE_BACKBTN);
    MkCtrl(s_dlg, L"STATIC", L"Etkin (Enabled)", SS_LEFT, 290, 134, 110, 18, 0);
    s_cEnab = MkCtrl(s_dlg, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 404, 130, 170, 120, IDE_ENAB);
    SendMessageW(s_cEnab, CB_ADDSTRING, 0, (LPARAM)L"True");
    SendMessageW(s_cEnab, CB_ADDSTRING, 0, (LPARAM)L"False");
    MkCtrl(s_dlg, L"STATIC", L"Araç İpucu", SS_LEFT, 290, 164, 110, 18, 0);
    s_eTip = MkCtrl(s_dlg, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 404, 160, 170, 24, IDE_TIP);
    MkCtrl(s_dlg, L"STATIC",
           L"Not: Sekme Ekle / Kaldır için TabControl seçiliyken sağ üstteki ok menüsünü de kullanabilirsiniz.",
           SS_LEFT, 290, 200, 284, 44, 0);
    // alt: Tamam / İptal
    MkCtrl(s_dlg, L"BUTTON", L"Tamam", WS_TABSTOP | BS_DEFPUSHBUTTON, 386, 396, 92, 30, IDE_OK);
    MkCtrl(s_dlg, L"BUTTON", L"İptal", WS_TABSTOP | BS_PUSHBUTTON, 486, 396, 92, 30, IDE_CANCEL);

    RebuildList(0);
    EnableWindow(g_hMain, FALSE);
    ShowWindow(s_dlg, SW_SHOW);
    SetForegroundWindow(s_dlg);
    SetFocus(s_lb);
    MSG msg;
    while (!s_done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) { s_ok = false; s_done = true; break; }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (IsWindow(s_dlg)) DestroyWindow(s_dlg);
    s_dlg = nullptr;
    EnableWindow(g_hMain, TRUE);
    SetForegroundWindow(g_hMain);
    // FAZ 2.6.3: diyaloğın kapladığı alan bayat kalmasın (siyah delik sorunu)
    RedrawWindow(g_hMain, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    if (!s_ok) {
        // iptal: açılış anındaki modele dön
        bool ok = false;
        json::Value v = json::Parse(s_snapshot, &ok);
        if (ok) {
            HWND surf = GetDlgItem(g_hMain, IDC_SURFACE);
            for (auto* c : g_model.ctrls) if (c->hwnd) { DestroyWindow(c->hwnd); c->hwnd = nullptr; }
            g_model.FromJson(v);
            surface::RecreateAll(surf);
        }
    }
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
    HWND pr = GetDlgItem(g_hMain, 1510);
    if (pr) Refresh(pr);
}

// ==================== FAZ 2.6.3: Koleksiyon Düzenleyicisi ====================
// VS "String Collection Editor" karşılığı: çok satırlı metin alanı, her satır
// bir öğe. Biçimler:
//   Items   (ListBox/CheckedListBox/ComboBox) : her satır bir öğe
//   Items   (ListView)                        : hücreler '|' ile ayrılır (Adı|Soyadı)
//   Columns (ListView)                        : her satır "Başlık, Genişlik"
//   Nodes   (TreeView)                        : girinti (Tab / 4 boşluk) = alt düğüm
namespace coled {

static bool s_done = false, s_ok = false;
static HWND s_dlg = nullptr, s_ed = nullptr;

static LRESULT CALLBACK ColEdProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id == IDOK)      { s_ok = true;  s_done = true; return 0; }
        if (id == IDCANCEL)  { s_ok = false; s_done = true; return 0; }
        break;
    }
    case WM_CLOSE:
        s_ok = false; s_done = true;
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

} // namespace coled

void OpenCollectionEditor(Ctrl* c, const std::wstring& propName) {
    using namespace coled;
    if (!c) return;
    // bu satırlar yalnız desteklenen tiplerde gösterilir; yine de koru
    if (propName == L"Items" &&
        _wcsicmp(c->type.c_str(), L"ListBox") != 0 && _wcsicmp(c->type.c_str(), L"CheckedListBox") != 0 &&
        _wcsicmp(c->type.c_str(), L"ComboBox") != 0 && _wcsicmp(c->type.c_str(), L"ListView") != 0) return;
    if (propName == L"Columns" && _wcsicmp(c->type.c_str(), L"ListView") != 0) return;
    if (propName == L"Nodes" && _wcsicmp(c->type.c_str(), L"TreeView") != 0) return;

    std::wstring title, hint;
    if (propName == L"Items") {
        if (_wcsicmp(c->type.c_str(), L"ListView") == 0) {
            title = c->name + L" — Öğeler (Items)";
            hint = L"Her satır bir liste satırıdır. Sütun hücrelerini '|' ile ayırın:\r\n  Ahmet|Yılmaz|İstanbul";
        } else {
            title = c->name + L" — Öğeler (Items)";
            hint = L"Her satır bir öğe olacak şekilde yazın:";
        }
    } else if (propName == L"Columns") {
        title = c->name + L" — Sütunlar (Columns)";
        hint = L"Her satır bir sütundur. Biçim: Başlık, Genişlik  (örnek: Ad, 140)";
    } else {
        title = c->name + L" — Düğümler (Nodes)";
        hint = L"Girinti (Tab veya 4 boşluk) düğümü bir alt seviyeye indirir.";
    }

    // mevcut değeri düzenlenebilir biçime getir (\n → \r\n)
    std::wstring cur = c->Get(propName.c_str());
    std::wstring shown;
    for (wchar_t ch : cur) { if (ch == L'\n') shown += L"\r\n"; else shown += ch; }

    RECT orc; GetWindowRect(g_hMain, &orc);
    int dw = 560, dh = 440;
    int dx = orc.left + ((orc.right - orc.left) - dw) / 2;
    int dy = orc.top + ((orc.bottom - orc.top) - dh) / 2;

    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = ColEdProc; wc.hInstance = g_hInst;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"M2GOD_COLED";
        RegisterClassW(&wc);
        reg = true;
    }
    s_done = false; s_ok = false;
    s_dlg = CreateWindowExW(0, L"M2GOD_COLED", title.c_str(),
                            WS_POPUP | WS_CAPTION | WS_SYSMENU,
                            dx, dy, dw, dh, g_hMain, nullptr, g_hInst, nullptr);
    HWND hHint = CreateWindowExW(0, L"STATIC", hint.c_str(), WS_CHILD | WS_VISIBLE,
                                 14, 12, dw - 28, 44, s_dlg, nullptr, g_hInst, nullptr);
    SendMessageW(hHint, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    s_ed = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", shown.c_str(),
                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
                           ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
                           14, 62, dw - 28, dh - 62 - 56, s_dlg, nullptr, g_hInst, nullptr);
    SendMessageW(s_ed, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    HWND hOk = CreateWindowExW(0, L"BUTTON", L"Tamam", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                               dw - 208, dh - 44, 92, 30, s_dlg, (HMENU)IDOK, g_hInst, nullptr);
    HWND hCancel = CreateWindowExW(0, L"BUTTON", L"İptal", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   dw - 106, dh - 44, 92, 30, s_dlg, (HMENU)IDCANCEL, g_hInst, nullptr);
    SendMessageW(hOk, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(hCancel, WM_SETFONT, (WPARAM)g_fontUI, TRUE);

    EnableWindow(g_hMain, FALSE);
    ShowWindow(s_dlg, SW_SHOW);
    SetForegroundWindow(s_dlg);
    SetFocus(s_ed);
    // VS davranışı: IsDialogMessage — Tab ile gezinme, Enter=Tamam (çok satırlı
    // alanda ES_WANTRETURN Enter'ı yeni satır yapar), ESC=İptal
    std::wstring result;
    bool hasResult = false;
    MSG msg;
    while (!s_done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (IsDialogMessageW(s_dlg, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    // değeri EDIT yok edilmeden ÖNCE al
    if (s_ok && s_ed && IsWindow(s_ed)) {
        std::vector<wchar_t> buf(65536);
        GetWindowTextW(s_ed, buf.data(), (int)buf.size());
        result = buf.data();
        hasResult = true;
    }
    if (IsWindow(s_dlg)) DestroyWindow(s_dlg);
    s_dlg = nullptr; s_ed = nullptr;
    EnableWindow(g_hMain, TRUE);
    SetForegroundWindow(g_hMain);
    // FAZ 2.6.3: diyaloğın kapladığı alan ANA pencerede bayat/siyah kalıyordu
    // (wine/Xvfb'de belirgin) — tam senkron repaint
    RedrawWindow(g_hMain, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);

    if (s_ok && hasResult) {
        // \r\n → \n normalizasyonu; boş satırlar atılır (VS davranışı)
        // NOT: Nodes satırlarının BAŞINDAKİ girinti anlamlıdır — soldan trim edilmez
        bool isNodes = (propName == L"Nodes");
        std::wstring val;
        std::wstring norm;
        for (wchar_t ch : result) { if (ch != L'\r') norm += ch; }
        for (auto& line : Split(norm, L'\n')) {
            std::wstring t = isNodes ? line : Trim(line);
            while (!t.empty() && (t.back() == L' ' || t.back() == L'\t')) t.pop_back();
            if (t.empty()) continue;
            if (!val.empty()) val += L"\n";
            val += t;
        }
        g_model.PushUndo();
        c->props[propName] = val;
        factory::ApplyProp(c, propName, val);
        InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, FALSE);
        PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
    }
    // panel satırlarını tazele ("<N öğe>" değeri güncellenir)
    FillRows(); SyncList();
}

} // namespace props
