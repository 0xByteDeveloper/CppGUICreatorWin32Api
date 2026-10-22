// surface.cpp — tasarım yüzeyi (parent-child, tıkla-yerleştir, bağlam menüsü)
#include "surface.h"
#include "app.h"
#include "factory.h"
#include "toolbox.h"
#include <commctrl.h>

namespace surface {

// --- durum -------------------------------------------------------------------
static int  s_scrollX = 0, s_scrollY = 0;
static const int PAD = 12;          // form ile yüzey kenarı arası
static const int TITLE_H = 32;      // sahte başlık çubuğu
static const int TRAY_H = 34;       // bileşen tepsisi
static Ctrl*    s_drag = nullptr;   // taşınan/boyutlanan
static int      s_dragMode = 0;     // 0 yok, 1 taşı, 2 boyutla
static int      s_handle = -1;      // 0..7
static POINT    s_start;            // fare başlangıç (yüzey client)
static RECT     s_startRect;        // kontrol başlangıç rect (parent görelisi)
static bool     s_started = false;  // eşik geçildi mi (undo için)
static bool     s_gridSnap = true;
static POINT    s_lastCursor = { 0, 0 };  // son imleç (yüzey client) — bırakma noktası
static HWND     s_surf = nullptr;         // yüzey (drag katmanı için)
static HWND     s_dragLayer = nullptr;    // SÜRÜKLEME GÖRSELİ KATMANI (tüm çocukların üstünde)

// --- sürükleme görsel katmanı -------------------------------------------------
// Gerçek HWND'ler sürükleme sırasında HİZLETİLMEZ (hayalet piksel/kendini
// kopyalamış gibi görünen bug'ların kök nedeni); yerine katman penceresi
// kontrollerin ÜSTÜNDE hayalet + bırakma hedefi çizer.
// Katman SetWindowRgn ile şekillendirilir: yalnız hayalet + hedef çerçevesi
// görünür, geri kalanı tamamen şeffaf ve tıklanabilir (Wine/Windows uyumlu).
static RECT CtrlRectOnSurface(const Ctrl* c, HWND surf);
static Ctrl* HitAt(HWND surf, int sx, int sy, Ctrl* exclude);
static Ctrl* DropParentAt(HWND surf, int sx, int sy, Ctrl* dragged);
static void ContainerChildArea(Ctrl* cont, int& x0, int& y0, int& x1, int& y1);
static RECT ClientAreaRect(HWND surf);
static LRESULT CALLBACK DragLayerProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_NCHITTEST: return HTTRANSPARENT;   // fare alttaki yüzeye/ kontrollere gitsin
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        Ctrl* d = s_drag;
        if (d && s_dragMode == 1 && s_started) {
            // hayalet kontrol (bölge zaten yalnız bu alanları kapsıyor)
            RECT g = CtrlRectOnSurface(d, s_surf);
            HBRUSH fb = CreateSolidBrush(RGB(204, 232, 255));
            FillRect(dc, &g, fb); DeleteObject(fb);
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
            HPEN op = (HPEN)SelectObject(dc, pen);
            HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
            Rectangle(dc, g.left, g.top, g.right, g.bottom);
            // seçim kesikli çerçeve
            HPEN dpen = CreatePen(PS_DOT, 1, RGB(0, 90, 158));
            HPEN op2 = (HPEN)SelectObject(dc, dpen);
            Rectangle(dc, g.left - SEL_MARGIN, g.top - SEL_MARGIN, g.right + SEL_MARGIN, g.bottom + SEL_MARGIN);
            SelectObject(dc, op2); DeleteObject(dpen);
            SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);
            SetBkMode(dc, TRANSPARENT);
            SelectObject(dc, g_fontUI);
            SetTextColor(dc, RGB(0, 60, 110));
            RECT tr = g; tr.left += 6;
            std::wstring label = d->name + L" (" + d->type + L")";
            DrawTextW(dc, label.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            // bırakma hedefi çerçevesi (bölge yalnız şeritleri kapsar)
            Ctrl* tgt = DropParentAt(s_surf, s_lastCursor.x, s_lastCursor.y, d);
            if (tgt && tgt != d) {
                int x0, y0, x1, y1; ContainerChildArea(tgt, x0, y0, x1, y1);
                POINT ap = g_model.AbsPos(tgt);
                RECT cr = ClientAreaRect(s_surf);
                RECT t = { cr.left + ap.x + x0, cr.top + ap.y + y0, cr.left + ap.x + x1, cr.top + ap.y + y1 };
                HPEN tpen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
                HPEN otp = (HPEN)SelectObject(dc, tpen);
                HBRUSH obt = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
                Rectangle(dc, t.left, t.top, t.right, t.bottom);
                SelectObject(dc, otp); SelectObject(dc, obt); DeleteObject(tpen);
            }
        }
        EndPaint(h, &ps);
        return 0;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

// katmanın görünür bölgesi = hayalet (+kenar) + hedef çerçeve şeritleri
static void DragLayerUpdateRegion(HWND surf) {
    if (!s_dragLayer || !IsWindow(s_dragLayer)) return;
    if (!(s_drag && s_dragMode == 1 && s_started)) { SetWindowRgn(s_dragLayer, nullptr, TRUE); return; }
    RECT g = CtrlRectOnSurface(s_drag, surf);
    InflateRect(&g, SEL_MARGIN + 1, SEL_MARGIN + 1);
    HRGN rgn = CreateRectRgnIndirect(&g);
    Ctrl* tgt = DropParentAt(surf, s_lastCursor.x, s_lastCursor.y, s_drag);
    if (tgt && tgt != s_drag) {
        int x0, y0, x1, y1; ContainerChildArea(tgt, x0, y0, x1, y1);
        POINT ap = g_model.AbsPos(tgt);
        RECT cr = ClientAreaRect(surf);
        RECT t = { cr.left + ap.x + x0, cr.top + ap.y + y0, cr.left + ap.x + x1, cr.top + ap.y + y1 };
        HRGN ss[4] = {
            CreateRectRgn(t.left - 1, t.top - 1, t.right + 1, t.top + 1),
            CreateRectRgn(t.left - 1, t.bottom - 1, t.right + 1, t.bottom + 1),
            CreateRectRgn(t.left - 1, t.top, t.left + 1, t.bottom),
            CreateRectRgn(t.right - 1, t.top, t.right + 1, t.bottom)
        };
        for (auto s : ss) { CombineRgn(rgn, rgn, s, RGN_OR); DeleteObject(s); }
    }
    SetWindowRgn(s_dragLayer, rgn, TRUE);
}

static void DragLayerShow(HWND surf) {
    if (!s_dragLayer) return;
    RECT rc; GetClientRect(surf, &rc);
    MoveWindow(s_dragLayer, 0, 0, rc.right, rc.bottom, FALSE);
    SetWindowPos(s_dragLayer, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    DragLayerUpdateRegion(surf);
    ShowWindow(s_dragLayer, SW_SHOWNA);
    InvalidateRect(s_dragLayer, nullptr, TRUE);
}
static void DragLayerHide() {
    if (s_dragLayer && IsWindow(s_dragLayer)) ShowWindow(s_dragLayer, SW_HIDE);
}
static void DragLayerInvalidate() {
    if (s_dragLayer && IsWindow(s_dragLayer)) {
        DragLayerUpdateRegion(s_surf);
        InvalidateRect(s_dragLayer, nullptr, TRUE);
    }
}

// --- koordinat yardımcıları ----------------------------------------------------
static void RepaintAll(HWND surf);   // ileri bildirim (DoScroll da kullanır)
static RECT ClientAreaRect(HWND surf) {
    RECT f = FormRect(surf);
    RECT c = { f.left, f.top + TITLE_H, f.right, f.bottom - TRAY_H };
    return c;
}
static RECT TrayRect(HWND surf) {
    RECT f = FormRect(surf);
    return { f.left, f.bottom - TRAY_H, f.right, f.bottom };
}
void ScreenToForm(HWND surf, int sx, int sy, int* fx, int* fy) {
    RECT c = ClientAreaRect(surf);
    *fx = sx - c.left;
    *fy = sy - c.top;
}
// kontrolün form koordinatındaki rect'i (parent zinciri toplanır)
static RECT AbsRect(const Ctrl* c) {
    POINT p = g_model.AbsPos(c);
    return { p.x, p.y, p.x + c->w, p.y + c->h };
}
// kontrolün yüzey client koordinatındaki rect'i
static RECT CtrlRectOnSurface(const Ctrl* c, HWND surf) {
    RECT cr = ClientAreaRect(surf);
    POINT p = g_model.AbsPos(c);
    return { cr.left + p.x, cr.top + p.y, cr.left + p.x + c->w, cr.top + p.y + c->h };
}
static bool VisibleCtrl(Ctrl* c) {
    return !c->nonVisual && _wcsicmp(c->Get(L"Visible", L"True").c_str(), L"true") == 0;
}

// handle dikdörtgenleri (yüzey koord), seçim rect'e göre
static RECT HandleRect(const RECT& sel, int idx) {
    int cx = (sel.left + sel.right) / 2, cy = (sel.top + sel.bottom) / 2;
    POINT p[8] = {
        { sel.left, sel.top }, { cx, sel.top }, { sel.right, sel.top },
        { sel.right, cy }, { sel.right, sel.bottom }, { cx, sel.bottom },
        { sel.left, sel.bottom }, { sel.left, cy }
    };
    int hs = 7;
    RECT r = { p[idx].x - hs / 2, p[idx].y - hs / 2, p[idx].x - hs / 2 + hs, p[idx].y - hs / 2 + hs };
    return r;
}

RECT FormRect(HWND surf) {
    RECT rc; GetClientRect(surf, &rc);
    RECT f = { PAD - s_scrollX, PAD - s_scrollY,
               PAD - s_scrollX + g_model.formW, PAD - s_scrollY + TITLE_H + g_model.formH };
    return f;
}

// VS kuralı: konteyner üstüne bırakılırsa içine; düz kontrol üstüne bırakılırsa
// o kontrolün bulunduğu katmana girer.
// TabControl: yalnızca SAYFA ALANI (sekme şeridi değil) içine alır — şeride
// bırakılanlar dış katmana gider (yanlışlıkla yutma sorununu önler).
static void ContainerChildArea(Ctrl* cont, int& x0, int& y0, int& x1, int& y1);
static Ctrl* DropParentAt(HWND surf, int sx, int sy, Ctrl* dragged) {
    Ctrl* t = HitAt(surf, sx, sy, dragged);
    Ctrl* p = nullptr;
    if (t) {
        if (IsContainerType(t->type)) {
            if (_wcsicmp(t->type.c_str(), L"TabControl") == 0) {
                int x0, y0, x1, y1; ContainerChildArea(t, x0, y0, x1, y1);
                int fx, fy; ScreenToForm(surf, sx, sy, &fx, &fy);
                POINT ap = g_model.AbsPos(t);
                int lx = fx - ap.x, ly = fy - ap.y;
                if (lx < x0 || ly < y0 || lx > x1 || ly > y1) p = t->parent;
                else p = t;
            } else p = t;
        }
        else p = t->parent;
    }
    if (p && (p == dragged || g_model.IsAncestorOf(dragged, p))) p = dragged->parent;
    return p;
}

// --- hit-test (parent-child duyarlı) ---------------------------------------------
// lx,ly: parent'in sol üstüne göre (parent=null → form koordinatı)
static Ctrl* HitInEx(Ctrl* parent, int lx, int ly, Ctrl* exclude) {
    auto kids = g_model.ChildrenOf(parent);
    for (int i = (int)kids.size() - 1; i >= 0; i--) {
        Ctrl* k = kids[i];
        if (k == exclude) continue;
        if (!VisibleCtrl(k)) continue;
        RECT r = { k->x, k->y, k->x + k->w, k->y + k->h };
        if (!PtInRect(&r, { lx, ly })) continue;
        Ctrl* sub = HitInEx(k, lx - k->x, ly - k->y, exclude);
        return sub ? sub : k;      // çocuklar her zaman parent'ın üstündedir
    }
    return nullptr;
}
static Ctrl* HitAt(HWND surf, int sx, int sy, Ctrl* exclude = nullptr) {
    int fx, fy; ScreenToForm(surf, sx, sy, &fx, &fy);
    return HitInEx(nullptr, fx, fy, exclude);
}
static Ctrl* HitControl(HWND surf, int sx, int sy) { return HitAt(surf, sx, sy); }

// konteynerin "çocuk alanı": GroupBox başlığı, TabControl sekme şeridi hariç
static void ContainerChildArea(Ctrl* cont, int& x0, int& y0, int& x1, int& y1) {
    x0 = 0; y0 = 0; x1 = cont->w; y1 = cont->h;
    if (_wcsicmp(cont->type.c_str(), L"GroupBox") == 0) { y0 = 20; x0 = 4; x1 -= 4; y1 -= 4; }
    else if (_wcsicmp(cont->type.c_str(), L"TabControl") == 0) {
        y0 = 30; x0 = 4; x1 -= 4; y1 -= 4;
        if (cont->hwnd) {
            RECT dr = { 0, 0, cont->w, cont->h };
            SendMessageW(cont->hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&dr);
            x0 = max(0, (int)dr.left); y0 = max(0, (int)dr.top);
            x1 = min(cont->w, (int)dr.right); y1 = min(cont->h, (int)dr.bottom);
        }
    }
}
static void ClampIntoContainer(Ctrl* cont, int& px, int& py, int w, int h) {
    int x0, y0, x1, y1;
    ContainerChildArea(cont, x0, y0, x1, y1);
    px = max(x0, min(px, max(x0, x1 - w)));
    py = max(y0, min(py, max(y0, y1 - h)));
}

// --- scrollbar yönetimi -------------------------------------------------------
static void UpdateScroll(HWND surf) {
    RECT rc; GetClientRect(surf, &rc);
    int viewW = rc.right, viewH = rc.bottom;
    int contentW = PAD * 2 + g_model.formW;
    int contentH = PAD * 2 + TITLE_H + g_model.formH;
    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0; si.nMax = contentW;
    si.nPage = (UINT)max(1, viewW); si.nPos = s_scrollX;
    SetScrollInfo(surf, SB_HORZ, &si, TRUE);
    si.nMax = contentH; si.nPage = (UINT)max(1, viewH); si.nPos = s_scrollY;
    SetScrollInfo(surf, SB_VERT, &si, TRUE);
    s_scrollX = min(s_scrollX, max(0, contentW - (int)viewW));
    s_scrollY = min(s_scrollY, max(0, contentH - (int)viewH));
}

static void DoScroll(HWND surf, int dx, int dy) {
    int odx = s_scrollX, ody = s_scrollY;
    RECT rc; GetClientRect(surf, &rc);
    int maxSX = max(0, PAD * 2 + g_model.formW - (int)rc.right);
    int maxSY = max(0, PAD * 2 + TITLE_H + g_model.formH - (int)rc.bottom);
    s_scrollX = min(max(0, s_scrollX + dx), maxSX);
    s_scrollY = min(max(0, s_scrollY + dy), maxSY);
    if (odx != s_scrollX || ody != s_scrollY) {
        ScrollWindow(surf, odx - s_scrollX, ody - s_scrollY, nullptr, nullptr);
        UpdateScroll(surf);
        RepaintAll(surf);   // kaydırmada da çocuklar dahil tam temizlik (hayalet olmasın)
    }
}

// --- çizim --------------------------------------------------------------------
static void DrawTitleBar(HDC dc, const RECT& f) {
    RECT t = { f.left, f.top, f.right, f.top + TITLE_H };
    HBRUSH br = CreateSolidBrush(RGB(247, 247, 247));
    FillRect(dc, &t, br); DeleteObject(br);
    RECT b = { f.left, f.top + TITLE_H - 1, f.right, f.top + TITLE_H };
    HBRUSH br2 = CreateSolidBrush(RGB(213, 213, 213));
    FillRect(dc, &b, br2); DeleteObject(br2);
    RECT tr = t; tr.left += 10;
    SelectObject(dc, g_fontUI);
    SetBkMode(dc, TRANSPARENT);
    std::wstring text = g_model.formProps.count(L"Text") ? g_model.formProps[L"Text"] : L"Form1";
    SetTextColor(dc, RGB(0, 0, 0));
    DrawTextW(dc, text.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    for (int i = 0; i < 3; i++) {
        RECT r = { f.right - 66 + i * 22, f.top + 8, f.right - 48 + i * 22, f.top + 24 };
        HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(dc, &r, wb); DeleteObject(wb);
        FrameRect(dc, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
    }
}

static void DrawGrid(HDC dc, const RECT& client) {
    if (!g_gridVisible) return;
    int gx = GRID_SIZE * 2;
    for (int x = gx - (s_scrollX % gx); x < client.right; x += gx) {
        if (x < client.left) continue;
        for (int y = gx - (s_scrollY % gx); y < client.bottom; y += gx) {
            if (y < client.top) continue;
            SetPixel(dc, x, y, RGB(208, 208, 208));
        }
    }
}

static void DrawComponentIcon(HDC dc, int x, int y, const std::wstring& type) {
    HBRUSH br = CreateSolidBrush(RGB(0, 120, 215));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 90, 158));
    HPEN op = (HPEN)SelectObject(dc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    if (type == L"Timer") {
        Ellipse(dc, x, y, x + 16, y + 16);
        SelectObject(dc, br);
        MoveToEx(dc, x + 8, y + 8, nullptr); LineTo(dc, x + 8, y + 3);
        MoveToEx(dc, x + 8, y + 8, nullptr); LineTo(dc, x + 12, y + 10);
    } else {
        Rectangle(dc, x, y, x + 16, y + 16);
        SelectObject(dc, br);
        MoveToEx(dc, x + 3, y + 8, nullptr); LineTo(dc, x + 13, y + 8);
        MoveToEx(dc, x + 8, y + 3, nullptr); LineTo(dc, x + 8, y + 13);
    }
    SelectObject(dc, op); SelectObject(dc, ob);
    DeleteObject(pen); DeleteObject(br);
}

static void DrawTray(HDC dc, HWND surf) {
    RECT tr = TrayRect(surf);
    RECT sep = { tr.left, tr.top, tr.right, tr.top + 1 };
    HBRUSH br = CreateSolidBrush(RGB(200, 200, 200));
    FillRect(dc, &sep, br); DeleteObject(br);
    int x = tr.left + 12;
    SelectObject(dc, g_fontUI);
    SetBkMode(dc, TRANSPARENT);
    for (auto* c : g_model.ctrls) {
        if (!c->nonVisual) continue;
        SIZE sz; GetTextExtentPoint32W(dc, c->name.c_str(), (int)c->name.size(), &sz);
        int w = 16 + 6 + sz.cx + 12;
        RECT item = { x, tr.top + 8, x + w, tr.top + TRAY_H - 8 };
        if (g_model.selected == c) {
            HPEN pen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
            HPEN op = (HPEN)SelectObject(dc, pen);
            HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
            Rectangle(dc, item.left, item.top, item.right, item.bottom);
            SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);
        }
        DrawComponentIcon(dc, item.left + 6, item.top + (item.bottom - item.top - 16) / 2, c->type);
        RECT tr2 = item; tr2.left += 24;
        SetTextColor(dc, RGB(30, 30, 30));
        DrawTextW(dc, c->name.c_str(), -1, &tr2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        x += w + 8;
    }
    if (x == tr.left + 12) {
        RECT tr3 = { tr.left + 14, tr.top, tr.right, tr.bottom };
        SetTextColor(dc, RGB(140, 140, 140));
        DrawTextW(dc, L"(Bileşen tepsisi — Timer vb. burada görünür)", -1, &tr3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

static void DrawSelection(HWND dcH, HDC dc) {
    Ctrl* sel = g_model.selected;
    if (!sel) return;
    HWND surf = dcH;
    RECT r;
    if (sel->nonVisual) {
        RECT tr = TrayRect(surf);
        int x = tr.left + 12;
        HDC tdc = GetDC(surf);
        SelectObject(tdc, g_fontUI);
        for (auto* c : g_model.ctrls) {
            if (!c->nonVisual) continue;
            SIZE sz; GetTextExtentPoint32W(tdc, c->name.c_str(), (int)c->name.size(), &sz);
            int w = 16 + 6 + sz.cx + 12;
            if (c == sel) { r = { x, tr.top + 8, x + w, tr.top + TRAY_H - 8 }; break; }
            x += w + 8;
        }
        ReleaseDC(surf, tdc);
    } else {
        RECT cs = CtrlRectOnSurface(sel, surf);
        r = { cs.left - SEL_MARGIN, cs.top - SEL_MARGIN, cs.right + SEL_MARGIN, cs.bottom + SEL_MARGIN };
    }
    HPEN pen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
    HPEN op = (HPEN)SelectObject(dc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, r.left, r.top, r.right, r.bottom);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
    if (!sel->nonVisual) {
        for (int i = 0; i < 8; i++) {
            RECT hr = HandleRect(r, i);
            HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(dc, &hr, wb); DeleteObject(wb);
            FrameRect(dc, &hr, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }
    }
}

static void Paint(HWND surf) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(surf, &ps);
    RECT rc; GetClientRect(surf, &rc);
    HDC mem = CreateCompatibleDC(dc);
    HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
    HBITMAP ob = (HBITMAP)SelectObject(mem, bmp);
    HBRUSH bg = CreateSolidBrush(RGB(229, 229, 229));
    FillRect(mem, &rc, bg); DeleteObject(bg);
    RECT f = FormRect(surf);
    RECT sh = f; OffsetRect(&sh, 4, 4);
    HBRUSH shb = CreateSolidBrush(RGB(205, 205, 205));
    FillRect(mem, &sh, shb); DeleteObject(shb);
    FrameRect(mem, &f, (HBRUSH)GetStockObject(BLACK_BRUSH));
    COLORREF formBack = RGB(255, 255, 255);
    std::wstring fb = g_model.formProps.count(L"BackColor") ? g_model.formProps[L"BackColor"] : L"Control";
    COLORREF t; if (ParseColor(fb, t)) formBack = t;
    RECT client = ClientAreaRect(surf);
    HBRUSH fb2 = CreateSolidBrush(formBack);
    FillRect(mem, &client, fb2); DeleteObject(fb2);
    DrawGrid(mem, client);
    DrawTitleBar(mem, f);
    DrawTray(mem, surf);
    // overlay sürükleme sırasında seçim çerçevesini katman çizer (çift çizgi olmasın)
    if (!(s_drag && s_dragMode == 1 && s_started))
        DrawSelection(surf, mem);
    BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, ob);
    DeleteObject(bmp); DeleteDC(mem);
    EndPaint(surf, &ps);
}

// ===PART2===

// --- etkileşim ----------------------------------------------------------------
static int HitHandle(HWND surf, int sx, int sy) {
    Ctrl* sel = g_model.selected;
    if (!sel || sel->nonVisual) return -1;
    RECT cs = CtrlRectOnSurface(sel, surf);
    RECT r = { cs.left - SEL_MARGIN, cs.top - SEL_MARGIN, cs.right + SEL_MARGIN, cs.bottom + SEL_MARGIN };
    for (int i = 0; i < 8; i++) {
        RECT hr = HandleRect(r, i);
        InflateRect(&hr, 3, 3);
        if (PtInRect(&hr, { sx, sy })) return i;
    }
    return -1;
}

static Ctrl* HitTray(HWND surf, int sx, int sy) {
    RECT tr = TrayRect(surf);
    if (!PtInRect(&tr, { sx, sy })) return nullptr;
    HDC tdc = GetDC(surf);
    SelectObject(tdc, g_fontUI);
    int x = tr.left + 12;
    for (auto* c : g_model.ctrls) {
        if (!c->nonVisual) continue;
        SIZE sz; GetTextExtentPoint32W(tdc, c->name.c_str(), (int)c->name.size(), &sz);
        int w = 16 + 6 + sz.cx + 12;
        RECT item = { x, tr.top + 8, x + w, tr.top + TRAY_H - 8 };
        if (PtInRect(&item, { sx, sy })) { ReleaseDC(surf, tdc); return c; }
        x += w + 8;
    }
    ReleaseDC(surf, tdc);
    return nullptr;
}

static void NotifySelChanged() {
    PostMessageW(g_hMain, MWM_SEL_CHANGED, (WPARAM)g_model.selected, 0);
}

// eski seçim kenarları/hayalet pikseller kalmasın: tüm çocuklarla tam yeniden çiz
static void RepaintAll(HWND surf) {
    RedrawWindow(surf, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

// aktif sürükleme kontrolünü güvenle iptal et (silme/undo sırasında çağrılır)
void OnModelDelete(Ctrl* c) {
    if (!s_drag) return;
    if (s_drag == c || g_model.IsAncestorOf(c, s_drag) || g_model.IsAncestorOf(s_drag, c)) {
        DragLayerHide();
        // gerçek HWND gizliydi (overlay sürükleme) — silinmeden önce göster
        if (c->hwnd && IsWindow(c->hwnd)) ShowWindow(c->hwnd, SW_SHOWNA);
        s_drag = nullptr; s_dragMode = 0; s_handle = -1;
    }
}

// araç kutusundan yeni kontrol yerleştir (bırakma VE tıkla-yerleştir ortak yolu)
void PlaceNew(HWND surf, const std::wstring& type, int sx, int sy) {
    const ElementDef* def = g_catalog.Find(type);
    if (!def) return;
    int fx, fy; ScreenToForm(surf, sx, sy, &fx, &fy);
    Ctrl* parent = DropParentAt(surf, sx, sy, nullptr);
    int px = fx, py = fy;
    if (parent) {
        POINT ap = g_model.AbsPos(parent);
        px = fx - ap.x; py = fy - ap.y;
    }
    factory::Win32Class m = factory::MapClass(type);
    int w = m.dw > 0 ? m.dw : 100, h = m.dh > 0 ? m.dh : 30;
    if (s_gridSnap) { px = ((px + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; py = ((py + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; }
    if (parent) ClampIntoContainer(parent, px, py, w, h);
    else { px = max(0, min(px, g_model.formW - 8)); py = max(0, min(py, g_model.formH - 8)); }
    g_model.PushUndo();
    Ctrl* c = g_model.Add(type, px, py, parent);
    if (!c) return;
    c->w = w; c->h = h;
    wchar_t b1[64], b2[64];
    swprintf(b1, 64, L"%d, %d", c->x, c->y);
    swprintf(b2, 64, L"%d, %d", c->w, c->h);
    c->props[L"Location"] = b1; c->props[L"Size"] = b2;
    factory::CreateOnSurface(surf, c);
    g_model.selected = c; g_model.formSelected = false;
    NotifySelChanged();
    RepaintAll(surf);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
    std::wstring where = parent ? (L" → " + parent->name + L" içine") : L"";
    Status(type + L" eklendi: " + c->name + where);
}

// sürükleme/boyutlandırmayı uygula — koordinatlar PARENT görelisi
// TAŞIMA (mode 1): gerçek HWND HİZLETİLMEZ (gizli), hayalet katmanda çizilir.
//   → konteyner kenarına çarpma yok, konteynerden akıcı çıkış var,
//   → grup kutusu içinde hayalet/kendini kopyalamış görüntü olmaz.
// BOYUTLANDIRMA (mode 2): HWND canlı boyutlanır + her hamlede tam temizlik.
static void ApplyDrag(HWND surf, int sx, int sy) {
    if (!s_drag) return;
    s_lastCursor = { sx, sy };
    int dx = sx - s_start.x, dy = sy - s_start.y;
    if (s_dragMode == 1) {
        int nx = s_startRect.left + dx, ny = s_startRect.top + dy;
        if (s_gridSnap) { nx = ((nx + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; ny = ((ny + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; }
        // KONTAYNER CLAMP'İ YOK — kontrol konteynerin dışına serbestçe taşınabilir;
        // hedef katman bırakma anında imleç konumundan belirlenir (VS davranışı)
        s_drag->x = nx; s_drag->y = ny;
        wchar_t b[64]; swprintf(b, 64, L"%d, %d", nx, ny);
        s_drag->props[L"Location"] = b;
        DragLayerInvalidate();          // hayalet + hedef vurgusu yeniden çiz
        PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);
        return;
    }
    // --- boyutlandırma (mode 2) ---
    int nw = s_startRect.right - s_startRect.left, nh = s_startRect.bottom - s_startRect.top;
    int x0 = s_startRect.left, y0 = s_startRect.top;
    switch (s_handle) {
    case 0: x0 += dx; y0 += dy; nw -= dx; nh -= dy; break;
    case 1: y0 += dy; nh -= dy; break;
    case 2: x0 += dx; y0 += dy; nw -= dx; nh -= dy; break;
    case 3: nw += dx; break;
    case 4: nw += dx; nh += dy; break;
    case 5: nh += dy; break;
    case 6: x0 += dx; nw -= dx; nh += dy; break;
    case 7: x0 += dx; nw -= dx; break;
    }
    if (s_gridSnap) { nw = max(GRID_SIZE, ((nw + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE); nh = max(GRID_SIZE, ((nh + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE); }
    nw = max(8, nw); nh = max(8, nh);
    s_drag->x = x0; s_drag->y = y0; s_drag->w = nw; s_drag->h = nh;
    wchar_t b1[64], b2[64];
    swprintf(b1, 64, L"%d, %d", x0, y0); swprintf(b2, 64, L"%d, %d", nw, nh);
    s_drag->props[L"Location"] = b1; s_drag->props[L"Size"] = b2;
    PositionOne(surf, s_drag);
    RepaintAll(surf);   // canlı boyutlandırmada hayalet pikselleri anında temizle
    PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);   // hafif güncelleme (flicker yok)
}

// sürükleme bitince: imlecin üstündeki konteynere al (VS davranışı)
static void FinishDragReparent(HWND surf, Ctrl* dragged) {
    // gerçek kontrolü geri göster (overlay sürükleme sırasında gizliydi)
    if (dragged->hwnd && IsWindow(dragged->hwnd)) ShowWindow(dragged->hwnd, SW_SHOWNA);
    DragLayerHide();
    if (!dragged) { PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); return; }
    Ctrl* newParent = DropParentAt(surf, s_lastCursor.x, s_lastCursor.y, dragged);
    if (newParent != dragged->parent) {
        int fx, fy; ScreenToForm(surf, s_lastCursor.x, s_lastCursor.y, &fx, &fy);
        // kontrolün sol üstünü koru (imleç konumu değil)
        RECT ar = AbsRect(dragged);
        int px = ar.left, py = ar.top;
        if (newParent) {
            POINT ap = g_model.AbsPos(newParent);
            px -= ap.x; py -= ap.y;
            ClampIntoContainer(newParent, px, py, dragged->w, dragged->h);
        } else {
            // forma bırakıldı: form sınırları içine kıstır
            px = max(0, min(px, max(0, g_model.formW - dragged->w)));
            py = max(0, min(py, max(0, g_model.formH - dragged->h)));
        }
        g_model.Reparent(dragged, newParent, px, py);
        NotifySelChanged();   // props panelini tazele (parent-görelisi değerler)
        std::wstring where = newParent ? (newParent->name + L" içine alındı") : L"forma alındı";
        Status(dragged->name + L" " + where);
    }
    // HER DURUMDA: HWND'yi model konumuna getir (overlay sürüklemede HWND
    // hareket etmediği için aynı-katman bırakmalarda da zorunlu)
    PositionOne(surf, dragged);
    RepaintAll(surf);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
}

// seçili kontrolün varsayılan olay adı
static std::wstring EvNameOf(Ctrl* c) {
    return c ? g_model.DefaultEventOf(c) : std::wstring(L"Load");
}

// sağ tık bağlam menüsü — Sil her koşulda erişilebilir
static void ShowContextMenu(HWND surf, int sx, int sy) {
    Ctrl* c = HitControl(surf, sx, sy);
    if (!c) c = HitTray(surf, sx, sy);
    if (c) {
        g_model.selected = c; g_model.formSelected = false;
        NotifySelChanged(); InvalidateRect(surf, nullptr, FALSE);
    }
    bool hasSel = g_model.selected != nullptr;
    HMENU m = CreatePopupMenu();
    if (hasSel) {
        std::wstring label = EvNameOf(g_model.selected) + L" işleyicisi oluştur";
        AppendMenuW(m, MF_STRING, 9001, label.c_str());
        AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    }
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_CUT, L"Kes\tCtrl+X");
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_COPY, L"Kopyala\tCtrl+C");
    AppendMenuW(m, MF_STRING, M2CMD_PASTE, L"Yapıştır\tCtrl+V");
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_DELETE, L"Sil\tDel");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_FRONT, L"Öne Getir\tCtrl+T");
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_BACK, L"Arkaya Gönder\tCtrl+B");
    POINT pt = { sx, sy };
    ClientToScreen(surf, &pt);
    SetForegroundWindow(g_hMain);
    int cmd = TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, surf, nullptr);
    DestroyMenu(m);
    if (cmd == 9001 && g_model.selected) {
        PostMessageW(g_hMain, MWM_EVENT_DBLCLICK, (WPARAM)g_model.selected, (LPARAM)_wcsdup(EvNameOf(g_model.selected).c_str()));
    } else if (cmd) {
        PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
    }
}

// ===PART3===

LRESULT CALLBACK SurfaceProc(HWND surf, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: Paint(surf); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SIZE: UpdateScroll(surf); InvalidateRect(surf, nullptr, FALSE); return 0;
    case WM_HSCROLL: {
        int req = LOWORD(wp);
        if (req == SB_LINERIGHT || req == SB_PAGERIGHT) DoScroll(surf, req == SB_LINERIGHT ? 40 : 160, 0);
        else if (req == SB_LINELEFT || req == SB_PAGELEFT) DoScroll(surf, req == SB_LINELEFT ? -40 : -160, 0);
        else if (req == SB_THUMBTRACK) DoScroll(surf, (int)HIWORD(wp) - s_scrollX, 0);
        return 0;
    }
    case WM_VSCROLL: {
        int req = LOWORD(wp);
        if (req == SB_LINEDOWN || req == SB_PAGEDOWN) DoScroll(surf, 0, req == SB_LINEDOWN ? 40 : 160);
        else if (req == SB_LINEUP || req == SB_PAGEUP) DoScroll(surf, 0, req == SB_LINEUP ? -40 : -160);
        else if (req == SB_THUMBTRACK) DoScroll(surf, 0, (int)HIWORD(wp) - s_scrollY);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -120 : 120;
        if (GetKeyState(VK_SHIFT) & 0x8000) DoScroll(surf, d, 0); else DoScroll(surf, 0, d);
        return 0;
    }
    case WM_SETCURSOR: {
        if (LOWORD(lp) == HTCLIENT) {
            if (!toolbox::ArmedType().empty()) {
                SetCursor(LoadCursor(nullptr, IDC_CROSS));   // VS: yerleştirme imleci
                return TRUE;
            }
        }
        break;   // alttaki WM_MOUSEMOVE imleç mantığına bırak
    }
    case WM_LBUTTONDOWN: {
        SetFocus(surf);
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        s_lastCursor = pt;
        // 1) araç kutusundan tıkla-yerleştir modu
        std::wstring armed = toolbox::ArmedType();
        if (!armed.empty()) {
            toolbox::Disarm();
            PlaceNew(surf, armed, pt.x, pt.y);
            Status(L"Hazır");
            return 0;
        }
        // 2) tutamaç (boyutlandırma)
        int hIdx = HitHandle(surf, pt.x, pt.y);
        if (hIdx >= 0 && g_model.selected) {
            g_model.PushUndo();
            s_drag = g_model.selected; s_dragMode = 2; s_handle = hIdx;
            s_start = pt; s_started = false;
            s_startRect = { s_drag->x, s_drag->y, s_drag->x + s_drag->w, s_drag->y + s_drag->h };
            SetCapture(surf);
            return 0;
        }
        // 3) kontrol seç + taşı (parent-child duyarlı: üstteki kontrol kazanır)
        Ctrl* c = HitControl(surf, pt.x, pt.y);
        if (!c) c = HitTray(surf, pt.x, pt.y);
        if (c) {
            if (g_model.selected != c) {
                g_model.selected = c; g_model.formSelected = false;
                NotifySelChanged(); RepaintAll(surf);
            }
            g_model.PushUndo();
            s_drag = c; s_dragMode = 1; s_start = pt; s_started = false;
            s_startRect = { c->x, c->y, c->x + c->w, c->y + c->h };
            SetCapture(surf);
        } else {
            if (!g_model.formSelected) {
                g_model.selected = nullptr; g_model.formSelected = true;
                NotifySelChanged(); RepaintAll(surf);
            }
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        s_lastCursor = pt;
        if (s_drag && (wp & MK_LBUTTON)) {
            if (!s_started) {
                int ddx = abs(pt.x - s_start.x), ddy = abs(pt.y - s_start.y);
                if (ddx > 2 || ddy > 2) {
                    s_started = true;
                    // OVERLAY SÜRÜKLEME başlıyor: gerçek kontrol gizlenir, hayalet katman gösterilir
                    if (s_dragMode == 1 && s_drag->hwnd && IsWindow(s_drag->hwnd) && !s_drag->nonVisual)
                        ShowWindow(s_drag->hwnd, SW_HIDE);
                    if (s_dragMode == 1 && !s_drag->nonVisual) DragLayerShow(surf);
                    InvalidateRect(surf, nullptr, FALSE);
                }
            }
            if (s_started) ApplyDrag(surf, pt.x, pt.y);
            return 0;
        }
        // imleç
        LPWSTR cur = IDC_ARROW;
        std::wstring armed = toolbox::ArmedType();
        if (armed.empty()) {
            int h = HitHandle(surf, pt.x, pt.y);
            if (h == 0 || h == 4) cur = IDC_SIZENWSE;
            else if (h == 2 || h == 6) cur = IDC_SIZENESW;
            else if (h == 3 || h == 7) cur = IDC_SIZEWE;
            else if (h == 1 || h == 5) cur = IDC_SIZENS;
            else if (HitControl(surf, pt.x, pt.y) || HitTray(surf, pt.x, pt.y)) cur = IDC_SIZEALL;
        }
        SetCursor(LoadCursor(nullptr, cur));
        // durum çubuğu
        int fx, fy; ScreenToForm(surf, pt.x, pt.y, &fx, &fy);
        wchar_t b[128];
        if (!armed.empty()) swprintf(b, 128, L"%ls yerleştirmek için tıkla — ESC iptal", armed.c_str());
        else if (g_model.selected && !g_model.formSelected) {
            Ctrl* c = g_model.selected;
            swprintf(b, 128, L"Konum: %d, %d   Boyut: %d × %d   [%ls]", fx, fy, c->w, c->h, c->name.c_str());
        } else swprintf(b, 128, L"Konum: %d, %d", fx, fy);
        Status(b);
        return 0;
    }
    case WM_LBUTTONUP: {
        if (s_drag) {
            Ctrl* dragged = s_drag;          // reparent için yakala
            int mode = s_dragMode;
            bool started = s_started;
            ReleaseCapture();
            s_drag = nullptr; s_dragMode = 0; s_started = false;
            if (mode == 2) {
                RepaintAll(surf);
                PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
            } else {
                // hayalet katmanı kapat, gerçek kontrolü geri getir, tam temizlik
                DragLayerHide();
                if (started && dragged->hwnd && IsWindow(dragged->hwnd) && !dragged->nonVisual)
                    ShowWindow(dragged->hwnd, SW_SHOWNA);
                RepaintAll(surf);
                if (started) FinishDragReparent(surf, dragged);
            }
        }
        return 0;
    }
    case WM_CAPTURECHANGED: {
        // sürükleme dışarıdan iptal edildiyse (silme vb.) görünümü toparla
        if (s_drag) {
            Ctrl* d = s_drag;
            DragLayerHide();
            if (d->hwnd && IsWindow(d->hwnd) && !d->nonVisual) ShowWindow(d->hwnd, SW_SHOWNA);
            s_drag = nullptr; s_dragMode = 0; s_started = false;
            RepaintAll(surf);
        }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        Ctrl* c = HitControl(surf, pt.x, pt.y);
        bool isForm = false;
        if (!c) c = HitTray(surf, pt.x, pt.y);
        if (!c) isForm = true;   // boş alan: form çift tıkı (Load)
        if (c || isForm) {
            std::wstring ev = isForm ? std::wstring(L"Load") : g_model.DefaultEventOf(c);
            wchar_t* heap = _wcsdup(ev.c_str());
            PostMessageW(g_hMain, MWM_EVENT_DBLCLICK, (WPARAM)c, (LPARAM)heap);
        }
        return 0;
    }
    case WM_RBUTTONUP:
        return DefWindowProcW(surf, msg, wp, lp);   // DefWindowProc WM_CONTEXTMENU üretir
    case WM_CONTEXTMENU: {
        // imleç yüzeyin üzerinde mi?
        HWND from = (HWND)wp;
        if (from != surf && from != nullptr) break;
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (lp == -1) { pt.x = pt.y = 0; ClientToScreen(surf, &pt); }
        else ScreenToClient(surf, &pt);
        ShowContextMenu(surf, pt.x, pt.y);
        return 0;
    }
    case WM_KEYDOWN: {
        Ctrl* c = g_model.selected;
        bool shift = GetKeyState(VK_SHIFT) & 0x8000;
        bool ctrl = GetKeyState(VK_CONTROL) & 0x8000;
        int step = s_gridSnap ? GRID_SIZE : 1;
        if (wp == VK_ESCAPE) {
            // 1) aktif sürüklemeyi iptal et: başlangıç konumuna dön
            if (s_drag) {
                Ctrl* d = s_drag;
                DragLayerHide();
                s_drag = nullptr; s_dragMode = 0; s_started = false;
                d->x = s_startRect.left; d->y = s_startRect.top;
                wchar_t b[64]; swprintf(b, 64, L"%d, %d", d->x, d->y);
                d->props[L"Location"] = b;
                if (d->hwnd && IsWindow(d->hwnd)) { PositionOne(surf, d); ShowWindow(d->hwnd, SW_SHOWNA); }
                RepaintAll(surf);
                NotifySelChanged();
                Status(L"Taşıma iptal edildi");
                return 0;
            }
            if (!toolbox::ArmedType().empty()) { toolbox::Disarm(); Status(L"Yerleştirme iptal edildi"); return 0; }
            g_model.selected = nullptr; g_model.formSelected = true;
            NotifySelChanged(); InvalidateRect(surf, nullptr, FALSE); return 0;
        }
        if (wp == VK_DELETE) { PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_DELETE, 0), 0); return 0; }
        if (wp == 'Z' && ctrl) { g_model.Undo(); RecreateAll(surf); NotifySelChanged(); InvalidateRect(surf, nullptr, TRUE); PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); return 0; }
        if (wp == 'Y' && ctrl) { g_model.Redo(); RecreateAll(surf); NotifySelChanged(); InvalidateRect(surf, nullptr, TRUE); PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); return 0; }
        if (wp == 'C' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_COPY, 0), 0); return 0; }
        if (wp == 'X' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_CUT, 0), 0); return 0; }
        if (wp == 'V' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_PASTE, 0), 0); return 0; }
        if (!c || c->nonVisual) return 0;
        int dx = 0, dy = 0, dw = 0, dh = 0;
        switch (wp) {
        case VK_LEFT:  if (shift) dw = -step; else dx = -step; break;
        case VK_RIGHT: if (shift) dw = step;  else dx = step;  break;
        case VK_UP:    if (shift) dh = -step; else dy = -step; break;
        case VK_DOWN:  if (shift) dh = step;  else dy = step;  break;
        default: return 0;
        }
        g_model.PushUndo();
        if (s_gridSnap && dx == 0 && dy == 0) { dw = ((dw + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; dh = ((dh + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; }
        c->x = max(0, c->x + dx); c->y = max(0, c->y + dy);
        c->w = max(8, c->w + dw); c->h = max(8, c->h + dh);
        if (c->parent) {
            int x0, y0, x1, y1;
            ContainerChildArea(c->parent, x0, y0, x1, y1);
            c->x = max(x0, min(c->x, max(x0, x1 - c->w)));
            c->y = max(y0, min(c->y, max(y0, y1 - c->h)));
        }
        PositionOne(surf, c);
        wchar_t b1[64], b2[64];
        swprintf(b1, 64, L"%d, %d", c->x, c->y); swprintf(b2, 64, L"%d, %d", c->w, c->h);
        c->props[L"Location"] = b1; c->props[L"Size"] = b2;
        RepaintAll(surf);   // konteyner içinde ok tuşuyla taşımada da hayalet kalmasın
        PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        Ctrl* cc = g_model.FindByCid((UINT)GetDlgCtrlID((HWND)lp));
        if (!cc) cc = g_model.FindByHwnd((HWND)lp);
        if (!cc) break;
        HDC dc = (HDC)wp;
        COLORREF fg, bg;
        if (!ParseColor(cc->Get(L"ForeColor", L"ControlText"), fg)) fg = RGB(0, 0, 0);
        if (!ParseColor(cc->Get(L"BackColor", L"Control"), bg)) bg = GetSysColor(COLOR_BTNFACE);
        SetTextColor(dc, fg);
        SetBkColor(dc, bg);
        SetBkMode(dc, OPAQUE);
        static std::map<HWND, std::pair<COLORREF, HBRUSH>> brushCache;
        auto it = brushCache.find((HWND)lp);
        if (it != brushCache.end() && it->second.first == bg) return (LRESULT)it->second.second;
        if (it != brushCache.end()) DeleteObject(it->second.second);
        HBRUSH br = CreateSolidBrush(bg);
        brushCache[(HWND)lp] = { bg, br };
        return (LRESULT)br;
    }
    case WM_DESTROY:
        for (auto* c : g_model.ctrls) if (c->hwnd) { DestroyWindow(c->hwnd); c->hwnd = nullptr; }
        return 0;
    }
    return DefWindowProcW(surf, msg, wp, lp);
}

// ===PART4===

HWND Create(HWND parent) {
    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = SurfaceProc;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"M2GOD_SURFACE";
        wc.style = CS_DBLCLKS;
        RegisterClassW(&wc);
        reg = true;
    }
    // NOT: WS_CLIPCHILDREN BİLİNÇLİ YOK — grup kutusu gibi şeffaf kontrollerin
    // iç yüzeyi parent boyasıyla temizlensin (hayalet piksellerin kök nedeni)
    HWND h = CreateWindowExW(0, L"M2GOD_SURFACE", L"", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
                             0, 0, 0, 0, parent, (HMENU)IDC_SURFACE, g_hInst, nullptr);
    s_surf = h;
    // sürükleme görsel katmanı (bölgeli child — kontrollerin üstünde, şekli dışında şeffaf)
    static bool regL = false;
    if (!regL) {
        WNDCLASSW wc2 = { 0 };
        wc2.lpfnWndProc = DragLayerProc;
        wc2.hInstance = g_hInst;
        wc2.hbrBackground = nullptr;
        wc2.lpszClassName = L"M2GOD_DRAGLAYER";
        RegisterClassW(&wc2);
        regL = true;
    }
    s_dragLayer = CreateWindowExW(WS_EX_TRANSPARENT, L"M2GOD_DRAGLAYER", L"",
                                  WS_CHILD, 0, 0, 0, 0, h, nullptr, g_hInst, nullptr);
    return h;
}

void SubclassForDesign(HWND h) {
    // fare mesajlarını tasarım yüzeyine aktar (HTTRANSPARENT zincirlenir:
    // konteyner çocuğu → konteyner → yüzey)
    SetWindowSubclass(h, [](HWND hh, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) -> LRESULT {
        if (m == WM_NCHITTEST) return HTTRANSPARENT;
        return DefSubclassProc(hh, m, w, l);
    }, 1, 0);
}

// kontrolleri parent zincirine göre oluştur (önce konteyner, sonra çocukları)
static void CreateBranch(HWND surf, Ctrl* c) {
    if (c->nonVisual) return;
    factory::CreateOnSurface(surf, c);
    for (auto* k : g_model.ChildrenOf(c)) CreateBranch(surf, k);
}

void LayoutAll(HWND surf) {
    for (auto* c : g_model.ctrls) PositionOne(surf, c);
    UpdateScroll(surf);
}

void PositionOne(HWND surf, Ctrl* c) {
    if (!c || !c->hwnd) return;
    if (c->parent && c->parent->hwnd) {
        // konteyner çocuğu: konteyner client koordinatı
        SetWindowPos(c->hwnd, nullptr, c->x, c->y, c->w, c->h, SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        RECT cr = ClientAreaRect(surf);
        SetWindowPos(c->hwnd, nullptr, cr.left + c->x, cr.top + c->y, c->w, c->h, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void RecreateAll(HWND surf) {
    for (auto* c : g_model.ctrls) {
        if (c->hwnd) { DestroyWindow(c->hwnd); c->hwnd = nullptr; }
    }
    // z-sırası kurallı: önce parent'lar; aynı parent içinde oluşturma sırası (son = üstte)
    for (auto* c : g_model.ChildrenOf(nullptr)) CreateBranch(surf, c);
    LayoutAll(surf);
    RepaintAll(surf);
}

void RefreshSelection(HWND surf) {
    InvalidateRect(surf, nullptr, FALSE);
}

void NotifySelection() { NotifySelChanged(); }

} // namespace surface
