// toolbox.cpp — araç kutusu paneli: kategoriler + elemanlar, yüzeye sürükle-bırak
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
static POINT s_dragPt;
static bool  s_dragging = false;
static std::wstring s_dragType;
static std::map<HTREEITEM, std::wstring> itemTypes;

static LRESULT CALLBACK PanelProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)l;
        if (nm->code == NM_DBLCLK && nm->idFrom == IDC_TOOLBOX_TREE) {
            TVHITTESTINFO ht = { 0 };
            DWORD mp = GetMessagePos();
            ht.pt.x = GET_X_LPARAM(mp); ht.pt.y = GET_Y_LPARAM(mp);
            ScreenToClient(s_tree, &ht.pt);
            HTREEITEM it = TreeView_HitTest(s_tree, &ht);
            if (it && itemTypes.count(it)) {
                const std::wstring& type = itemTypes[it];
                const ElementDef* def = g_catalog.Find(type);
                int x = 24 + (int)(g_model.ctrls.size() % 6) * 16;
                int y = 24 + (int)(g_model.ctrls.size() % 6) * 12;
                Ctrl* c = g_model.Add(type, x, y);
                if (c) {
                    factory::CreateOnSurface(GetDlgItem(g_hMain, IDC_SURFACE), c);
                    g_model.selected = c; g_model.formSelected = false;
                    surface::NotifySelection();
                    InvalidateRect(GetDlgItem(g_hMain, IDC_SURFACE), nullptr, FALSE);
                    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
                    Status(type + L" eklendi: " + c->name);
                }
            }
            return 0;
        }
        if (nm->code == TVN_BEGINDRAG && nm->idFrom == IDC_TOOLBOX_TREE) {
            NMTREEVIEW* nt = (NMTREEVIEW*)l;
            if (itemTypes.count(nt->itemNew.hItem)) {
                s_dragType = itemTypes[nt->itemNew.hItem];
                s_dragging = true;
                s_dragPt = nt->ptDrag;
                SetCapture(h);
            }
            return 0;
        }
        break;
    }
    case WM_MOUSEMOVE: {
        if (s_dragging) {
            POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            ClientToScreen(h, &pt);
            // hayalet çizimi: basit — ekran kenarında çapraz imleç + durum çubuğu bilgisi
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
                int fx, fy; surface::ScreenToForm(surf, sp.x, sp.y, &fx, &fy);
                // snap
                fx = ((fx + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE;
                fy = ((fy + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE;
                Ctrl* c = g_model.Add(s_dragType, fx, fy);
                if (c) {
                    factory::CreateOnSurface(surf, c);
                    g_model.selected = c; g_model.formSelected = false;
                    surface::NotifySelection();
                    InvalidateRect(surf, nullptr, FALSE);
                    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
                    Status(s_dragType + L" bırakıldı: " + c->name);
                }
            }
            Status(L"Hazır");
        }
        break;
    }
    case WM_SIZE: {
        RECT rc; GetClientRect(h, &rc);
        MoveWindow(s_caption, 0, 0, rc.right, 26, TRUE);
        MoveWindow(s_tree, 0, 26, rc.right, rc.bottom - 26, TRUE);
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
    s_caption = CreateWindowExW(0, L"STATIC", L"Araç Kutusu", WS_CHILD | WS_VISIBLE,
                                0, 0, TOOLBOX_W, 26, h, nullptr, g_hInst, nullptr);
    s_tree = CreateWindowExW(0, WC_TREEVIEWW, L"",
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
                             0, 26, TOOLBOX_W, 200, h, (HMENU)IDC_TOOLBOX_TREE, g_hInst, nullptr);
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
    for (auto& cat : cats) {
        auto it = g_catalog.catElements.find(cat);
        if (it == g_catalog.catElements.end() || it->second.empty()) continue;
        ti.hParent = nullptr;
        ti.item.pszText = (LPWSTR)cat.c_str();
        HTREEITEM parent = TreeView_InsertItem(s_tree, &ti);
        for (auto& name : it->second) {
            const ElementDef* def = g_catalog.Find(name);
            if (!def || !def->toolboxVisible) continue;
            ti.hParent = parent;
            ti.item.pszText = (LPWSTR)name.c_str();
            HTREEITEM child = TreeView_InsertItem(s_tree, &ti);
            itemTypes[child] = name;
        }
        TreeView_Expand(s_tree, parent, TVE_EXPAND);
    }
}

} // namespace toolbox
