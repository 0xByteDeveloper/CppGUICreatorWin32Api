// surface.cpp — tasarım yüzeyi gerçeklemesi (Bölüm 1)
#include "surface.h"
#include "app.h"
#include "factory.h"
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
static POINT    s_start;            // fare başlangıç (ekran)
static RECT     s_startRect;        // kontrol başlangıç rect (form koord)
static bool     s_started = false;  // eşik geçildi mi (undo için)
static bool     s_gridSnap = true;

// handle dikdörtgenleri (ekran koord), seçim rect'e göre
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
// kontrollerin bulunduğu client alan (başlık altı)
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
        InvalidateRect(surf, nullptr, FALSE);
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
    // form adı
    RECT tr = t; tr.left += 10;
    SelectObject(dc, g_fontUI);
    SetBkMode(dc, TRANSPARENT);
    std::wstring text = g_model.formProps.count(L"Text") ? g_model.formProps[L"Text"] : L"Form1";
    SetTextColor(dc, RGB(0, 0, 0));
    DrawTextW(dc, text.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    // sahte pencere düğmeleri
    for (int i = 0; i < 3; i++) {
        RECT r = { f.right - 66 + i * 22, f.top + 8, f.right - 48 + i * 22, f.top + 24 };
        HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(dc, &r, wb); DeleteObject(wb);
        FrameRect(dc, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
    }
}

static void DrawGrid(HDC dc, const RECT& client) {
    HPEN dot = CreatePen(PS_SOLID, 1, RGB(222, 222, 222));
    HPEN op = (HPEN)SelectObject(dc, dot);
    int gx = GRID_SIZE * 2;
    for (int x = gx - (s_scrollX % gx); x < client.right; x += gx) {
        if (x < client.left) continue;
        for (int y = gx - (s_scrollY % gx); y < client.bottom; y += gx) {
            if (y < client.top) continue;
            SetPixel(dc, x, y, RGB(208, 208, 208));
        }
    }
    SelectObject(dc, op); DeleteObject(dot);
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
    RECT cr = ClientAreaRect(surf);
    RECT r;
    if (sel->nonVisual) {
        RECT tr = TrayRect(surf);
        // tepsideki yerine göre hesapla (basit: x konumu tekrar hesapla)
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
        r = { cr.left + sel->x - SEL_MARGIN, cr.top + sel->y - SEL_MARGIN,
              cr.left + sel->x + sel->w + SEL_MARGIN, cr.top + sel->y + sel->h + SEL_MARGIN };
    }
    // kesikli çerçeve
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
    // arka plan
    HBRUSH bg = CreateSolidBrush(RGB(229, 229, 229));
    FillRect(mem, &rc, bg); DeleteObject(bg);
    RECT f = FormRect(surf);
    // gölge
    RECT sh = f; OffsetRect(&sh, 4, 4);
    HBRUSH shb = CreateSolidBrush(RGB(205, 205, 205));
    FillRect(mem, &sh, shb); DeleteObject(shb);
    // form çerçevesi + içi
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
    RECT cr = ClientAreaRect(surf);
    RECT r = { cr.left + sel->x - SEL_MARGIN, cr.top + sel->y - SEL_MARGIN,
               cr.left + sel->x + sel->w + SEL_MARGIN, cr.top + sel->y + sel->h + SEL_MARGIN };
    for (int i = 0; i < 8; i++) {
        RECT hr = HandleRect(r, i);
        InflateRect(&hr, 3, 3);
        if (PtInRect(&hr, { sx, sy })) return i;
    }
    return -1;
}

static Ctrl* HitControl(HWND surf, int sx, int sy) {
    RECT cr = ClientAreaRect(surf);
    for (int i = (int)g_model.ctrls.size() - 1; i >= 0; i--) {
        Ctrl* c = g_model.ctrls[i];
        if (c->nonVisual) continue;
        RECT r = { cr.left + c->x, cr.top + c->y, cr.left + c->x + c->w, cr.top + c->y + c->h };
        if (PtInRect(&r, { sx, sy })) return c;
    }
    return nullptr;
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

static void ApplyDrag(HWND surf, int sx, int sy) {
    if (!s_drag) return;
    RECT cr = ClientAreaRect(surf);
    int dx = sx - s_start.x, dy = sy - s_start.y;
    if (s_dragMode == 1) {
        int nx = s_startRect.left + dx, ny = s_startRect.top + dy;
        if (s_gridSnap) { nx = ((nx + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; ny = ((ny + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; }
        nx = max(0, min(nx, g_model.formW - 8)); ny = max(0, min(ny, g_model.formH - 8));
        s_drag->x = nx; s_drag->y = ny;
        SetWindowPos(s_drag->hwnd, nullptr, cr.left + nx, cr.top + ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        wchar_t b[64]; swprintf(b, 64, L"%d, %d", nx, ny);
        s_drag->props[L"Location"] = b;
    } else {
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
        SetWindowPos(s_drag->hwnd, nullptr, cr.left + x0, cr.top + y0, nw, nh, SWP_NOZORDER | SWP_NOACTIVATE);
        wchar_t b1[64], b2[64];
        swprintf(b1, 64, L"%d, %d", x0, y0); swprintf(b2, 64, L"%d, %d", nw, nh);
        s_drag->props[L"Location"] = b1; s_drag->props[L"Size"] = b2;
    }
    InvalidateRect(surf, nullptr, FALSE);
    PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);
}

static void NotifySelChanged() {
    PostMessageW(g_hMain, MWM_SEL_CHANGED, (WPARAM)g_model.selected, 0);
}

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
    case WM_LBUTTONDOWN: {
        SetFocus(surf);
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int hIdx = HitHandle(surf, pt.x, pt.y);
        if (hIdx >= 0 && g_model.selected) {
            g_model.PushUndo();
            s_drag = g_model.selected; s_dragMode = 2; s_handle = hIdx;
            s_start = pt; s_started = false;
            RECT cr = ClientAreaRect(surf);
            s_startRect = { s_drag->x, s_drag->y, s_drag->x + s_drag->w, s_drag->y + s_drag->h };
            SetCapture(surf);
            return 0;
        }
        Ctrl* c = HitControl(surf, pt.x, pt.y);
        if (!c) c = HitTray(surf, pt.x, pt.y);
        if (c) {
            if (g_model.selected != c) {
                g_model.selected = c; g_model.formSelected = false;
                NotifySelChanged(); InvalidateRect(surf, nullptr, FALSE);
            }
            g_model.PushUndo();
            s_drag = c; s_dragMode = 1; s_start = pt; s_started = false;
            s_startRect = { c->x, c->y, c->x + c->w, c->y + c->h };
            SetCapture(surf);
        } else {
            if (!g_model.formSelected) {
                g_model.selected = nullptr; g_model.formSelected = true;
                NotifySelChanged(); InvalidateRect(surf, nullptr, FALSE);
            }
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (s_drag && (wp & MK_LBUTTON)) {
            POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            if (!s_started) {
                int ddx = abs(pt.x - s_start.x), ddy = abs(pt.y - s_start.y);
                if (ddx > 2 || ddy > 2) s_started = true;
            }
            if (s_started) ApplyDrag(surf, pt.x, pt.y);
            return 0;
        }
        // imleç
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int h = HitHandle(surf, pt.x, pt.y);
        LPWSTR cur = IDC_ARROW;
        if (h == 0 || h == 4) cur = IDC_SIZENWSE;
        else if (h == 2 || h == 6) cur = IDC_SIZENESW;
        else if (h == 3 || h == 7) cur = IDC_SIZEWE;
        else if (h == 1 || h == 5) cur = IDC_SIZENS;
        else if (HitControl(surf, pt.x, pt.y) || HitTray(surf, pt.x, pt.y)) cur = IDC_SIZEALL;
        SetCursor(LoadCursor(nullptr, cur));
        // durum çubuğu: fare form koordinatı
        int fx, fy; ScreenToForm(surf, pt.x, pt.y, &fx, &fy);
        wchar_t b[96];
        if (g_model.selected && !g_model.formSelected) {
            Ctrl* c = g_model.selected;
            swprintf(b, 96, L"Konum: %d, %d   Boyut: %d × %d   [%s]", fx, fy, c->w, c->h, c->name.c_str());
        } else swprintf(b, 96, L"Konum: %d, %d", fx, fy);
        Status(b);
        return 0;
    }
    case WM_LBUTTONUP: {
        if (s_drag) {
            ReleaseCapture();
            s_drag = nullptr; s_dragMode = 0;
            InvalidateRect(surf, nullptr, FALSE);
            PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
        }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        Ctrl* c = HitControl(surf, pt.x, pt.y);
        bool isForm = false;
        if (!c) c = HitTray(surf, pt.x, pt.y);
        if (!c && g_model.formSelected) isForm = true;
        if (c || isForm) {
            std::wstring ev = isForm ? std::wstring(L"Load") : g_model.DefaultEventOf(c);
            wchar_t* heap = _wcsdup(ev.c_str());
            PostMessageW(g_hMain, MWM_EVENT_DBLCLICK, (WPARAM)c, (LPARAM)heap);
        }
        return 0;
    }
    case WM_KEYDOWN: {
        Ctrl* c = g_model.selected;
        bool shift = GetKeyState(VK_SHIFT) & 0x8000;
        bool ctrl = GetKeyState(VK_CONTROL) & 0x8000;
        int step = s_gridSnap ? GRID_SIZE : 1;
        if (wp == VK_ESCAPE) {
            g_model.selected = nullptr; g_model.formSelected = true;
            NotifySelChanged(); InvalidateRect(surf, nullptr, FALSE); return 0;
        }
        if (!c) {
            if (wp == 'V' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, 2103, 0); return 0; } // yapıştır
            if (wp == 'V' && ctrl == FALSE) return 0;
            return 0;
        }
        if (wp == VK_DELETE) { PostMessageW(g_hMain, WM_COMMAND, 2104, 0); return 0; }
        if (wp == 'Z' && ctrl) { g_model.Undo(); RecreateAll(surf); NotifySelChanged(); InvalidateRect(surf, nullptr, TRUE); PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); return 0; }
        if (wp == 'Y' && ctrl) { g_model.Redo(); RecreateAll(surf); NotifySelChanged(); InvalidateRect(surf, nullptr, TRUE); PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); return 0; }
        if (wp == 'C' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, 2102, 0); return 0; }
        if (wp == 'X' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, 2101, 0); return 0; }
        if (wp == 'V' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, 2103, 0); return 0; }
        if (c->nonVisual) return 0;
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
        RECT cr = ClientAreaRect(surf);
        SetWindowPos(c->hwnd, nullptr, cr.left + c->x, cr.top + c->y, c->w, c->h, SWP_NOZORDER | SWP_NOACTIVATE);
        wchar_t b1[64], b2[64];
        swprintf(b1, 64, L"%d, %d", c->x, c->y); swprintf(b2, 64, L"%d, %d", c->w, c->h);
        c->props[L"Location"] = b1; c->props[L"Size"] = b2;
        InvalidateRect(surf, nullptr, FALSE);
        PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        // CreateWindowExW sırasında da gelir: önce id ile ara (hwnd henüz modelde olmayabilir)
        Ctrl* c = g_model.FindByCid((UINT)GetDlgCtrlID((HWND)lp));
        if (!c) c = g_model.FindByHwnd((HWND)lp);
        if (!c) break;
        HDC dc = (HDC)wp;
        COLORREF fg, bg;
        if (!ParseColor(c->Get(L"ForeColor", L"ControlText"), fg)) fg = RGB(0, 0, 0);
        if (!ParseColor(c->Get(L"BackColor", L"Control"), bg)) bg = GetSysColor(COLOR_BTNFACE);
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
    HWND h = CreateWindowExW(0, L"M2GOD_SURFACE", L"", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN,
                             0, 0, 0, 0, parent, (HMENU)IDC_SURFACE, g_hInst, nullptr);
    return h;
}

void SubclassForDesign(HWND h) {
    SetWindowSubclass(h, [](HWND hh, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) -> LRESULT {
        if (m == WM_NCHITTEST) return HTTRANSPARENT;
        return DefSubclassProc(hh, m, w, l);
    }, 1, 0);
}

void LayoutAll(HWND surf) {
    RECT cr = ClientAreaRect(surf);
    for (auto* c : g_model.ctrls) {
        if (!c->hwnd) continue;
        SetWindowPos(c->hwnd, nullptr, cr.left + c->x, cr.top + c->y, c->w, c->h,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    UpdateScroll(surf);
}

void PositionOne(HWND surf, Ctrl* c) {
    if (!c || !c->hwnd) return;
    RECT cr = ClientAreaRect(surf);
    SetWindowPos(c->hwnd, nullptr, cr.left + c->x, cr.top + c->y, c->w, c->h,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void RecreateAll(HWND surf) {
    for (auto* c : g_model.ctrls) {
        if (c->hwnd) { DestroyWindow(c->hwnd); c->hwnd = nullptr; }
    }
    // z-sırası: arkadan öne
    for (auto* c : g_model.ctrls) {
        if (c->nonVisual) continue;
        factory::CreateOnSurface(surf, c);
    }
    LayoutAll(surf);
    InvalidateRect(surf, nullptr, TRUE);
}

void RefreshSelection(HWND surf) {
    InvalidateRect(surf, nullptr, FALSE);
}

void NotifySelection() { NotifySelChanged(); }

} // namespace surface

