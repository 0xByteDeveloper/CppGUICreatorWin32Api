// ============================================================================
//  Bars.cpp — TopBar (menu selector), OptionsStrip, TabBar, StatusStrip
// ============================================================================
#include "UI.h"
#include "Theme.h"

using namespace Gdiplus;

// ===========================================================================
// shared widget plumbing
// ===========================================================================
#define WM_SETSTATUS (AM_BASE + 1)

template <class T>
static LRESULT WidgetProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)l;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(h, m, w, l);
    }
    auto* self = (T*)GetWindowLongPtrW(h, GWLP_USERDATA);
    return self ? self->Proc(h, m, w, l) : DefWindowProcW(h, m, w, l);
}

static ATOM RegOnce(const wchar_t* cls, WNDPROC proc, HINSTANCE inst)
{
    WNDCLASSW wc;
    if (GetClassInfoW(inst, cls, &wc)) return 0;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.lpszClassName = cls;
    wc.hbrBackground = nullptr;
    return RegisterClassW(&wc);
}

// standalone Graphics for measuring text outside WM_PAINT
struct MeasureCtx
{
    HWND hwnd; HDC hdc; Graphics* g;
    MeasureCtx(HWND h) : hwnd(h) {
        hdc = GetDC(h);
        g = new Graphics(hdc);
        g->SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    }
    ~MeasureCtx() { delete g; if (hwnd) ReleaseDC(hwnd, hdc); }
    Graphics* operator->() { return g; }
    Graphics& G() { return *g; }
};

// ===========================================================================
// TopBar
// ===========================================================================
namespace TopBarNS
{
    struct Item { std::wstring text; RECT rc; };

    struct Self
    {
        HWND hwnd = nullptr;
        std::vector<Item> items;
        int active = 0;
        int vsCount = -1;        // -1 unknown
        HoverTrack hover;
        int hotItem = -1;
        bool chipHot = false;
        long chipX = -100000;

        Self(HWND h) : hwnd(h) {}

        void Layout()
        {
            items.clear();
            const wchar_t* names[MENU_COUNT] =
                { L"Welcome", L"Projeler", L"Araç Kutusu", L"Görünüm", L"Bağlantı", L"Yardım" };
            float scale = ScaleF(hwnd);
            MeasureCtx mc(hwnd);
            float x = 190 * scale;
            RECT rc; GetClientRect(hwnd, &rc);
            float mid = (rc.bottom - rc.top) * 0.5f;
            (void)mid;
            for (int i = 0; i < MENU_COUNT; i++) {
                Gdiplus::RectF m = MeasureTextW(mc.G(), names[i], Theme_Font(F_BODY_SB));
                float w = m.Width + 26 * scale;
                Item it; it.text = names[i];
                it.rc = { (int)x, 0, (int)(x + w), 0 };
                items.push_back(it);
                x += w + 6 * scale;
            }
        }

        LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l);
    };

    LRESULT Self::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        switch (m) {
            case WM_SIZE:
                Layout();
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_MOUSEMOVE: {
                hover.On(h, m, l);
                int x = GetXL(l);
                hotItem = -1;
                for (size_t i = 0; i < items.size(); i++)
                    if (PtInRc(&items[i].rc, x, 1)) hotItem = (int)i;
                RECT rc; GetClientRect(h, &rc);
                chipHot = (x >= (int)chipX && x < rc.right - 8);
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            case WM_MOUSELEAVE:
                hover.On(h, m, l);
                hotItem = -1; chipHot = false;
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_LBUTTONDOWN: {
                int x = GetXL(l);
                for (size_t i = 0; i < items.size(); i++) {
                    if (PtInRc(&items[i].rc, x, 1)) {
                        g_mw->SetMenu((int)i);
                        return 0;
                    }
                }
                if (chipHot) g_mw->OnTopBarVSChip();
                return 0;
            }
            case WM_PAINT: {
                BufferedPaint bp(h, TC(C_BG));
                Graphics& g = *bp.g;
                float scale = ScaleF(h);
                FillRectC(g, 0, (float)bp.H() - 1, (float)bp.W(), 1, TC(C_BORDER));
                // app title
                ThemeText(g, APP_NAME, Theme_Font(F_H2), TC(C_TEXT),
                          RectF(16 * scale, 0, 170 * scale, (float)bp.H()),
                          StringAlignmentNear, StringAlignmentCenter);
                for (size_t i = 0; i < items.size(); i++) {
                    float pw = (float)(items[i].rc.right - items[i].rc.left);
                    float x = (float)items[i].rc.left;
                    float py = (bp.H() - 30 * scale) / 2.0f;
                    bool isHot = ((int)i == hotItem);
                    bool isActive = ((int)i == active);
                    if (isActive)
                        DrawPill(g, x, py, pw, 30 * scale, items[i].text,
                                 Theme_Font(F_BODY_SB), TC(C_CARD), TC(C_TEXT), isHot);
                    else if (isHot)
                        DrawPill(g, x, py, pw, 30 * scale, items[i].text,
                                 Theme_Font(F_BODY_SB), TC(C_HOVER_SOFT), TC(C_TEXT), true);
                    else
                        DrawPill(g, x, py, pw, 30 * scale, items[i].text,
                                 Theme_Font(F_BODY_SB), Color(0, 0, 0, 0), TC(C_TEXT2), false);
                }
                // VS status chip (right)
                std::wstring chip;
                Color dot = TC(C_TEXT3);
                if (vsCount < 0) { chip = L"VS: ?"; }
                else if (vsCount == 0) { chip = L"VS yok"; }
                else { chip = Fmt(L"devenv: %d", vsCount); dot = TC(C_OK); }
                RectF mch = MeasureTextW(g, chip, Theme_Font(F_SMALL_SB));
                float cw = mch.Width + 36 * scale;
                float cx = bp.W() - cw - 12 * scale;
                chipX = (long)cx;
                float cy = (bp.H() - 26 * scale) / 2.0f;
                DrawPill(g, cx, cy, cw, 26 * scale, L"", Theme_Font(F_SMALL_SB),
                         TC(chipHot ? C_HOVER_SOFT : C_CARD), TC(C_TEXT2), false);
                DrawDot(g, cx + 14 * scale, cy + 13 * scale, 4.5f * scale, dot);
                ThemeText(g, chip, Theme_Font(F_SMALL_SB), TC(C_TEXT2),
                          RectF(cx + 24 * scale, cy, cw - 26 * scale, 26 * scale),
                          StringAlignmentNear, StringAlignmentCenter);
                return 0;
            }
        }
        return DefWindowProcW(h, m, w, l);
    }

    static WNDPROC s_proc = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        return WidgetProc<Self>(h, m, w, l);
    };

    ATOM RegisterClass1(HINSTANCE inst) { return RegOnce(L"CGC_TopBar", s_proc, inst); }

    HWND Create(HWND parent)
    {
        Self* s = new Self(nullptr);
        return CreateWindowExW(0, L"CGC_TopBar", nullptr, WS_CHILD | WS_VISIBLE,
                               0, 0, 100, 40, parent, nullptr,
                               (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), s);
    }

    void SetActiveMenu(HWND h, int idx)
    {
        auto* s = (Self*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (s) { s->active = idx; InvalidateRect(h, nullptr, FALSE); }
    }
    void SetVSCount(HWND h, int count)
    {
        auto* s = (Self*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (s) { s->vsCount = count; InvalidateRect(h, nullptr, FALSE); }
    }
}

// ===========================================================================
// OptionsStrip
// ===========================================================================
namespace OptionsNS
{
    struct Pill { RECT rc; bool isPlus; bool hot; };

    struct Self
    {
        HWND hwnd = nullptr;
        std::vector<OptionItem> opts;
        int selected = -1;
        std::vector<Pill> pills;
        int scroll = 0;               // x-offset
        HoverTrack hover;

        Self(HWND h) : hwnd(h) {}

        void BuildPills()
        {
            pills.clear();
            float scale = ScaleF(hwnd);
            MeasureCtx mc(hwnd);
            float x = 12 * scale;
            for (size_t i = 0; i < opts.size(); i++) {
                RectF m = MeasureTextW(mc.G(), opts[i].text, Theme_Font(F_BODY));
                float pw = m.Width + 24 * scale;
                Pill p; p.isPlus = opts[i].isPlus; p.hot = false;
                p.rc = { (int)x, 0, (int)(x + pw), 0 };
                pills.push_back(p);
                x += pw + 8 * scale;
            }
            Pill p; p.isPlus = true; p.hot = false;       // trailing "+"
            p.rc = { (int)x, 0, (int)(x + 30 * scale), 0 };
            pills.push_back(p);
        }

        void Set(const std::vector<OptionItem>& o, int sel)
        {
            opts = o; selected = sel; scroll = 0;
            BuildPills();
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l);
    };

    LRESULT Self::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        switch (m) {
            case WM_SIZE:
                BuildPills();
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_MOUSEMOVE: {
                hover.On(h, m, l);
                int x = GetXL(l) + scroll;
                for (auto& p : pills) p.hot = PtInRc(&p.rc, x, 1);
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            case WM_MOUSELEAVE:
                hover.On(h, m, l);
                for (auto& p : pills) p.hot = false;
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_MOUSEWHEEL: {
                RECT rc; GetClientRect(h, &rc);
                int avail = pills.empty() ? 0
                          : (int)pills.back().rc.right - (int)rc.right + 20;
                int maxScroll = std::max(0, avail);
                if (maxScroll > 0) {
                    scroll = std::max(0, std::min(maxScroll,
                        scroll - (GET_WHEEL_DELTA_WPARAM(w) > 0 ? 60 : -60)));
                    InvalidateRect(h, nullptr, FALSE);
                }
                return 0;
            }
            case WM_LBUTTONDOWN: {
                int x = GetXL(l) + scroll;
                for (size_t i = 0; i < pills.size(); i++) {
                    if (PtInRc(&pills[i].rc, x, 1)) {
                        if (pills[i].isPlus) g_mw->OnOptionClick(-1);   // "+"
                        else g_mw->OnOptionClick((int)i);
                        return 0;
                    }
                }
                return 0;
            }
            case WM_PAINT: {
                BufferedPaint bp(h, TC(C_CARD));
                Graphics& g = *bp.g;
                float scale = ScaleF(h);
                FillRectC(g, 0, (float)bp.H() - 1, (float)bp.W(), 1, TC(C_BORDER));
                g.SetClip(RectF(0, 0, (float)bp.W(), (float)bp.H()));
                for (size_t i = 0; i < pills.size(); i++) {
                    auto& p = pills[i];
                    float px = (float)p.rc.left - scroll;
                    if (px > bp.W() || px + (float)(p.rc.right - p.rc.left) < 0) continue;
                    float py = (bp.H() - 28 * scale) / 2.0f;
                    float pw = (float)(p.rc.right - p.rc.left);
                    if (p.isPlus) {
                        bool trailing = (i == pills.size() - 1);
                        bool hot = p.hot;
                        Color fill = TC(C_CARD);
                        Color glyph = TC(C_ACCENT);
                        if (trailing) { fill = hot ? TC(C_ACCENT_HOVER) : TC(C_ACCENT); glyph = TC(C_WHITE); }
                        else if (hot) fill = TC(C_HOVER_SOFT);
                        DrawPill(g, px, py, pw, 28 * scale, L"", Theme_Font(F_BODY_SB),
                                 fill, TC(C_WHITE), hot);
                        DrawPlusGlyph(g, px + pw / 2, py + 14 * scale, 7 * scale, glyph, 1.8f);
                    } else {
                        bool sel = ((int)i == selected);
                        DrawPill(g, px, py, pw, 28 * scale, opts[i].text,
                                 Theme_Font(sel ? F_BODY_SB : F_BODY),
                                 sel ? TC(C_ACCENT_SOFT) : (p.hot ? TC(C_HOVER_SOFT) : TC(C_GRAY_SOFT)),
                                 sel ? TC(C_ACCENT) : TC(C_TEXT), p.hot, sel);
                    }
                }
                return 0;
            }
        }
        return DefWindowProcW(h, m, w, l);
    }

    static WNDPROC s_proc = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        return WidgetProc<Self>(h, m, w, l);
    };

    ATOM RegisterClass1(HINSTANCE inst) { return RegOnce(L"CGC_Options", s_proc, inst); }

    HWND Create(HWND parent)
    {
        Self* s = new Self(nullptr);
        return CreateWindowExW(0, L"CGC_Options", nullptr, WS_CHILD | WS_VISIBLE,
                               0, 0, 100, 36, parent, nullptr,
                               (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), s);
    }

    void SetOptions(HWND h, const std::vector<OptionItem>& opts, int selected)
    {
        auto* s = (Self*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (s) s->Set(opts, selected);
    }
}

// ===========================================================================
// TabBar
// ===========================================================================
namespace TabBarNS
{
    struct TabRect { RECT rc; RECT closeRc; };

    struct Self
    {
        HWND hwnd = nullptr;
        int active = -1;
        std::vector<TabRect> trs;
        int hotTab = -1, hotClose = -1;
        HoverTrack hover;

        Self(HWND h) : hwnd(h) {}

        void Set(const std::vector<MainWindow::Tab>& tabs, int act)
        {
            active = act;
            trs.clear();
            float scale = ScaleF(hwnd);
            MeasureCtx mc(hwnd);
            float x = 8 * scale;
            for (size_t i = 0; i < tabs.size(); i++) {
                std::wstring title = tabs[i].welcome ? std::wstring(WELCOME_TAB) : tabs[i].project;
                RectF m = MeasureTextW(mc.G(), title, Theme_Font(F_BODY_SB));
                float w = std::max(110 * scale, std::min(250 * scale, m.Width + 52 * scale));
                TabRect tr;
                tr.rc = { (int)x, 0, (int)(x + w), 0 };
                float closeW = 18 * scale;
                tr.closeRc = { (int)(x + w - 8 * scale - closeW), 0,
                               (int)(x + w - 8 * scale), 0 };
                trs.push_back(tr);
                x += w + 4 * scale;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l);
    };

    LRESULT Self::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        switch (m) {
            case WM_MOUSEMOVE: {
                hover.On(h, m, l);
                int x = GetXL(l);
                hotTab = hotClose = -1;
                for (size_t i = 0; i < trs.size(); i++) {
                    if (PtInRc(&trs[i].rc, x, 1)) {
                        hotTab = (int)i;
                        if (PtInRc(&trs[i].closeRc, x, 1)) hotClose = (int)i;
                    }
                }
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            case WM_MOUSELEAVE:
                hover.On(h, m, l);
                hotTab = hotClose = -1;
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_LBUTTONDOWN: {
                int x = GetXL(l);
                for (size_t i = 0; i < trs.size(); i++) {
                    if (PtInRc(&trs[i].closeRc, x, 1)) {
                        g_mw->CloseTab((int)i);
                        return 0;
                    }
                    if (PtInRc(&trs[i].rc, x, 1)) {
                        g_mw->ActivateTab((int)i);
                        return 0;
                    }
                }
                return 0;
            }
            case WM_PAINT: {
                BufferedPaint bp(h, TC(C_TABBAR));
                Graphics& g = *bp.g;
                float scale = ScaleF(h);
                FillRectC(g, 0, (float)bp.H() - 1, (float)bp.W(), 1, TC(C_BORDER));
                float th = 30 * scale;
                float ty = (float)bp.H() - th - 4 * scale;
                float r = 9 * scale;
                for (size_t i = 0; i < trs.size(); i++) {
                    auto& tr = trs[i];
                    float tx = (float)tr.rc.left;
                    float tw = (float)(tr.rc.right - tr.rc.left);
                    bool isAct = ((int)i == active);
                    bool isHot = ((int)i == hotTab);
                    std::wstring title = (g_mw && i < g_mw->tabs.size())
                        ? (g_mw->tabs[i].welcome ? std::wstring(WELCOME_TAB) : g_mw->tabs[i].project)
                        : std::wstring(L"?");
                    if (isAct) {
                        // active tab: white card welded to the content seam below
                        FillRound(g, tx, ty, tw, th + 5 * scale, r, TC(C_TAB_ACTIVE));
                        FillRectC(g, tx + 2, ty + th, tw - 4, 5 * scale, TC(C_TAB_ACTIVE));
                    } else if (isHot) {
                        FillRound(g, tx, ty, tw, th, r, TC(C_HOVER_SOFT));
                    }
                    float pad = 12 * scale;
                    ThemeText(g, title, Theme_Font(F_BODY_SB),
                              isAct ? TC(C_TEXT) : TC(C_TEXT2),
                              RectF(tx + pad, ty, tw - pad - 22 * scale, th),
                              StringAlignmentNear, StringAlignmentCenter);
                    float ccx = tx + tw - 8 * scale - 9 * scale;
                    float ccy = ty + th / 2;
                    bool hotC = (hotClose == (int)i);
                    DrawCloseGlyph(g, ccx, ccy, 7 * scale,
                                   hotC ? TC(C_TEXT) : (isAct || isHot ? TC(C_TEXT2) : TC(C_TEXT3)),
                                   hotC);
                }
                return 0;
            }
        }
        return DefWindowProcW(h, m, w, l);
    }

    static WNDPROC s_proc = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        return WidgetProc<Self>(h, m, w, l);
    };

    ATOM RegisterClass1(HINSTANCE inst) { return RegOnce(L"CGC_TabBar", s_proc, inst); }

    HWND Create(HWND parent)
    {
        Self* s = new Self(nullptr);
        return CreateWindowExW(0, L"CGC_TabBar", nullptr, WS_CHILD | WS_VISIBLE,
                               0, 0, 100, 32, parent, nullptr,
                               (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), s);
    }

    void SetTabs(HWND h, const std::vector<MainWindow::Tab>& tabs, int active)
    {
        auto* s = (Self*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (s) s->Set(tabs, active);
    }
}

// ===========================================================================
// StatusStrip
// ===========================================================================
namespace StatusNS
{
    struct Self
    {
        HWND hwnd = nullptr;
        std::wstring text = L"Hazır";
        Self(HWND h) : hwnd(h) {}

        LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l)
        {
            switch (m) {
                case WM_PAINT: {
                    BufferedPaint bp(h, TC(C_BG));
                    Graphics& g = *bp.g;
                    float scale = ScaleF(h);
                    FillRectC(g, 0, 0, (float)bp.W(), 1, TC(C_BORDER));
                    ThemeText(g, text, Theme_Font(F_SMALL), TC(C_TEXT2),
                              RectF(12 * scale, 1, (float)bp.W() - 440 * scale, (float)bp.H() - 1),
                              StringAlignmentNear, StringAlignmentCenter);
                    ThemeText(g, std::wstring(APP_NAME) + L" " + APP_VERSION_W +
                                  L"  •  pure Win32 + GDI+  •  " + APP_COMPANY,
                              Theme_Font(F_TINY), TC(C_TEXT3),
                              RectF((float)bp.W() - 430 * scale, 1, 420 * scale, (float)bp.H() - 1),
                              StringAlignmentFar, StringAlignmentCenter);
                    return 0;
                }
                case WM_SETSTATUS:
                    text = (const wchar_t*)l;
                    InvalidateRect(h, nullptr, FALSE);
                    return 0;
            }
            return DefWindowProcW(h, m, w, l);
        }
    };

    static WNDPROC s_proc = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        return WidgetProc<Self>(h, m, w, l);
    };

    ATOM RegisterClass1(HINSTANCE inst) { return RegOnce(L"CGC_Status", s_proc, inst); }

    HWND Create(HWND parent)
    {
        Self* s = new Self(nullptr);
        return CreateWindowExW(0, L"CGC_Status", nullptr, WS_CHILD | WS_VISIBLE,
                               0, 0, 100, 24, parent, nullptr,
                               (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), s);
    }

    void SetText(HWND h, const std::wstring& s)
    {
        SendMessageW(h, WM_SETSTATUS, 0, (LPARAM)s.c_str());
    }
}
