// toolbox.cpp — araç kutusu: arama kutusu + kategori ağacı + sürükle-bırak + tıkla-yerleştir
#include "toolbox.h"
#include "app.h"
#include "catalog.h"
#include "model.h"
#include "factory.h"
#include "surface.h"
#include <commctrl.h>

namespace toolbox {

static HWND s_tree = nullptr;
static HWND s_caption = nullptr;
static HWND s_search = nullptr;
static HWND s_panelH = nullptr;                 // panel (armed göstergesi için)
static HWND s_panelHwnd() { return s_panelH; }
static POINT s_dragPt;
static bool  s_dragging = false;
static std::wstring s_dragType;
static std::wstring s_armedType;               // tıkla-yerleştir modu (VS davranışı)
static std::map<HTREEITEM, std::wstring> itemTypes;
static std::wstring s_filter;

// --- armed (tıkla-yerleştir) API ------------------------------------------------
std::wstring ArmedType() { return s_armedType; }
void Disarm() {
    if (!s_armedType.empty()) {
        s_armedType.clear();
        // ağaç vurgusunu temizle (VS: yerleştirme bitince araç vurgusu kalkar)
        if (s_tree) {
            PostMessageW(s_tree, TVM_SELECTITEM, TVGN_CARET, 0);
            InvalidateRect(s_tree, nullptr, FALSE);
        }
        InvalidateRect(s_panelHwnd(), nullptr, FALSE);
        Status(L"Hazır");
    }
}
void Arm(const std::wstring& type) {
    s_armedType = type;
    Status(type + L" — formda bir yere tıklayın (ESC = iptal)");
    InvalidateRect(s_panelHwnd(), nullptr, FALSE);
}

static LRESULT CALLBACK PanelProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_COMMAND: {
        if (LOWORD(w) == IDC_TOOLBOX_SEARCH && HIWORD(w) == EN_CHANGE) {
            wchar_t buf[128] = L"";
            GetWindowTextW(s_search, buf, 128);
            s_filter = Lower(buf);
            Refresh(h);
            return 0;
        }
        break;
    }
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)l;
        if (nm->idFrom != IDC_TOOLBOX_TREE) break;
        if (nm->code == NM_CLICK || nm->code == NM_DBLCLK) {
            // tek tık: tıkla-yerleştir modunu aç (VS2019 davranışı)
            TVHITTESTINFO ht = { 0 };
            DWORD mp = GetMessagePos();
            ht.pt.x = GET_X_LPARAM(mp); ht.pt.y = GET_Y_LPARAM(mp);
            ScreenToClient(s_tree, &ht.pt);
            HTREEITEM it = TreeView_HitTest(s_tree, &ht);
            if (it && itemTypes.count(it)) {
                const std::wstring& type = itemTypes[it];
                if (nm->code == NM_CLICK) {
                    Arm(type);   // imleç formda artık artı (cross) olur
                    // araç vurgusu: kategori DEĞİL, tıklanan araç seçilsin (VS gibi)
                    TreeView_SelectItem(s_tree, it);
                } else {
                    // çift tık: geçerli konuma ekle (basamaklı)
                    Disarm();
                    HWND surf = GetDlgItem(g_hMain, IDC_SURFACE);
                    RECT cr; GetClientRect(surf, &cr);
                    int cx = cr.right / 2 - 50, cy = cr.bottom / 2 - 15;
                    surface::PlaceNew(surf, type, cx, cy);
                }
                return 1;
            }
            return 1;   // kategori tıkları varsayılan işlemi yapmasın (yanlış vurgu engeli)
        }
        if (nm->code == TVN_SELCHANGED) {
            // YANLIŞ VURGU DÜZELTMESİ: seçim bir ARAÇ değilse (kategori vs.)
            // vurguyu temizle; araçsa tıkla-yerleştir modunu aç
            NMTREEVIEW* nt = (NMTREEVIEW*)l;
            HTREEITEM it = nt->itemNew.hItem;
            if (!it || !itemTypes.count(it)) {
                if (TreeView_GetSelection(s_tree))
                    PostMessageW(s_tree, TVM_SELECTITEM, TVGN_CARET, 0);
            } else if (s_armedType.empty() || s_armedType != itemTypes[it]) {
                // klavye/varsayılan yolla seçim araca geldi — arm et
                Arm(itemTypes[it]);
            }
            return 0;
        }
        if (nm->code == TVN_BEGINDRAG) {
            NMTREEVIEW* nt = (NMTREEVIEW*)l;
            if (itemTypes.count(nt->itemNew.hItem)) {
                s_dragType = itemTypes[nt->itemNew.hItem];
                s_dragging = true;
                Disarm();
                s_dragPt = nt->ptDrag;
                SetCapture(h);
            }
            return 0;
        }
        break;
    }
    case WM_MOUSEMOVE: {
        if (s_dragging) {
            Status(L"Sürükle: " + s_dragType + L" — tasarım yüzeyine bırak");
        }
        break;
    }
    case WM_LBUTTONUP: {
        if (s_dragging) {
            ReleaseCapture();
            s_dragging = false;
            POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            ClientToScreen(h, &pt);
            HWND surf = GetDlgItem(g_hMain, IDC_SURFACE);
            RECT sr; GetWindowRect(surf, &sr);
            if (PtInRect(&sr, pt)) {
                POINT sp = pt; ScreenToClient(surf, &sp);
                surface::PlaceNew(surf, s_dragType, sp.x, sp.y);   // konteyner algılama dahil
            }
            Status(L"Hazır");
        }
        break;
    }
    case WM_SIZE: {
        RECT rc; GetClientRect(h, &rc);
        MoveWindow(s_caption, 0, 0, rc.right, 26, TRUE);
        MoveWindow(s_search, 4, 30, rc.right - 8, 24, TRUE);
        MoveWindow(s_tree, 0, 58, rc.right, rc.bottom - 58, TRUE);
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
        RECT tr = cap; tr.left += 10;
        DrawTextW(dc, L"Araç Kutusu", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        // armed göstergesi
        if (!s_armedType.empty()) {
            SetTextColor(dc, RGB(255, 230, 120));
            SelectObject(dc, g_fontUI);
            RECT ar = { 10, rc.bottom - 24, rc.right, rc.bottom - 4 };
            DrawTextW(dc, s_armedType.c_str(), -1, &ar, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
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
        wc.lpszClassName = L"M2GOD_TOOLBOX";
        RegisterClassW(&wc);
        reg = true;
    }
    HWND h = CreateWindowExW(0, L"M2GOD_TOOLBOX", L"", WS_CHILD | WS_VISIBLE,
                             0, 0, TOOLBOX_W, 200, parent, (HMENU)1500, g_hInst, nullptr);
    s_panelH = h;
    s_caption = CreateWindowExW(0, L"STATIC", L"Araç Kutusu", WS_CHILD | WS_VISIBLE,
                                0, 0, TOOLBOX_W, 26, h, nullptr, g_hInst, nullptr);
    s_search = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                               4, 30, TOOLBOX_W - 8, 24, h, (HMENU)IDC_TOOLBOX_SEARCH, g_hInst, nullptr);
    SendMessageW(s_search, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(s_search, EM_SETCUEBANNER, TRUE, (LPARAM)L"Araç ara...");   // fil ipucu
    s_tree = CreateWindowExW(0, WC_TREEVIEWW, L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
                             0, 58, TOOLBOX_W, 200, h, (HMENU)IDC_TOOLBOX_TREE, g_hInst, nullptr);
    SendMessageW(s_tree, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(s_caption, WM_SETFONT, (WPARAM)g_fontUIBold, TRUE);
    return h;
}

void Refresh(HWND tb) {
    (void)tb;
    TreeView_DeleteAllItems(s_tree);
    itemTypes.clear();
    // kategori sırası + diğerleri
    std::vector<std::wstring> cats = g_catalog.categories;
    std::map<std::wstring, bool> seen;
    for (auto& c : cats) seen[c] = true;
    for (auto& kv : g_catalog.catElements)
        if (!seen.count(kv.first)) cats.push_back(kv.first);
    TVINSERTSTRUCTW ti = { 0 };
    ti.hInsertAfter = TVI_LAST;
    ti.item.mask = TVIF_TEXT;
    bool filtering = !s_filter.empty();
    for (auto& cat : cats) {
        auto it = g_catalog.catElements.find(cat);
        if (it == g_catalog.catElements.end() || it->second.empty()) continue;
        // arama filtresi: eşleşen elemanı olan kategoriler
        HTREEITEM parent = nullptr;
        for (auto& name : it->second) {
            const ElementDef* def = g_catalog.Find(name);
            if (!def || !def->toolboxVisible) continue;
            if (filtering && Lower(name).find(s_filter) == std::wstring::npos) continue;
            if (!parent) {
                ti.hParent = nullptr;
                ti.item.pszText = (LPWSTR)cat.c_str();
                parent = TreeView_InsertItem(s_tree, &ti);
            }
            ti.hParent = parent;
            ti.item.pszText = (LPWSTR)name.c_str();
            HTREEITEM child = TreeView_InsertItem(s_tree, &ti);
            itemTypes[child] = name;
        }
        if (parent && !filtering) TreeView_Expand(s_tree, parent, TVE_EXPAND);
        if (parent && filtering) TreeView_Expand(s_tree, parent, TVE_EXPAND);
    }
    if (filtering) {
        // filtre boşsa bilgi satırı
        if (itemTypes.empty()) {
            ti.hParent = nullptr;
            wchar_t msg[160];
            swprintf(msg, 160, L"\"%ls\" için sonuç yok", s_filter.c_str());
            ti.item.pszText = msg;
            TreeView_InsertItem(s_tree, &ti);
        }
    }
}

} // namespace toolbox
