// ============================================================================
//  WelcomePage.cpp — Welcome tab: hero, running-VS chips, recent projects
// ============================================================================
#include "UI.h"
#include "Theme.h"

using namespace Gdiplus;
#include <shlwapi.h>

namespace WelcomeNS
{
    enum Hit { HIT_NONE, HIT_NEW, HIT_SCAN, HIT_ROW, HIT_VSCHIP, HIT_REFRESH };

    struct Row { RECT rc; int index; bool isVS; int vsIndex; };

    struct Self
    {
        HWND hwnd = nullptr;
        HoverTrack hover;
        std::vector<Row> rows;
        int hotRow = -1, hotVs = -1;
        bool hotNew = false, hotScan = false, hotRefresh = false;
        RECT rcNew{}, rcScan{}, rcRefresh{};
        int scroll = 0;      // wheel offset for the list section

        Self(HWND h) : hwnd(h) {}

        // ---------------------------------------------------------------
        LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l);
        void PaintHero(Graphics& g, const RECT& rc, float scale);
        void PaintList(Graphics& g, const RECT& rc, float scale);
    };

    LRESULT Self::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        switch (m) {
            case WM_SIZE:
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_MOUSEWHEEL: {
                scroll = std::max(0, scroll - (GET_WHEEL_DELTA_WPARAM(w) > 0 ? 48 : -48));
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            case WM_MOUSEMOVE: {
                hover.On(h, m, l);
                int x = GetXL(l), y = GetYL(l);
                hotNew = PtInRc(&rcNew, x, y) != 0;
                hotScan = PtInRc(&rcScan, x, y) != 0;
                hotRefresh = PtInRc(&rcRefresh, x, y) != 0;
                hotRow = hotVs = -1;
                for (size_t i = 0; i < rows.size(); i++) {
                    if (PtInRc(&rows[i].rc, x, y)) {
                        if (rows[i].isVS) hotVs = rows[i].vsIndex;
                        else hotRow = rows[i].index;
                    }
                }
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            case WM_MOUSELEAVE:
                hover.On(h, m, l);
                hotNew = hotScan = hotRefresh = false;
                hotRow = hotVs = -1;
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_LBUTTONDOWN: {
                int x = GetXL(l), y = GetYL(l);
                if (PtInRc(&rcNew, x, y)) {
                    g_mw->CreateNewProjectFlow();
                    return 0;
                }
                if (PtInRc(&rcScan, x, y)) {
                    g_mw->RefreshVS(true);
                    return 0;
                }
                if (PtInRc(&rcRefresh, x, y)) {
                    g_mw->RefreshRecents();
                    g_mw->RefreshVS(true);
                    return 0;
                }
                for (size_t i = 0; i < rows.size(); i++) {
                    if (PtInRc(&rows[i].rc, x, y)) {
                        if (rows[i].isVS) {
                            int vi = rows[i].vsIndex;
                            if (vi >= 0 && vi < (int)g_mw->vs.size()) {
                                const VSInstance& v = g_mw->vs[vi];
                                std::wstring name = !v.solutionName.empty() ? v.solutionName
                                            : Fmt(L"devenv_%lu", (unsigned long)v.pid);
                                g_mw->OpenProjectTab(name);
                            }
                        } else {
                            int ri = rows[i].index;
                            if (ri >= 0 && ri < (int)g_mw->recents.size()) {
                                g_mw->OpenProjectTab(g_mw->recents[ri].name);
                            }
                        }
                        return 0;
                    }
                }
                return 0;
            }
            case WM_PAINT: {
                BufferedPaint bp(h, TC(C_BG));
                Graphics& g = *bp.g;
                float scale = ScaleF(h);
                rows.clear();
                RECT rc = bp.rc;
                // hero occupies top ~200 dip
                int heroH = ScaleI(h, 210);
                RECT heroRc = { 0, 0, rc.right, heroH };
                PaintHero(g, heroRc, scale);
                RECT listRc = { 0, heroH, rc.right, rc.bottom };
                PaintList(g, listRc, scale);
                return 0;
            }
        }
        return DefWindowProcW(h, m, w, l);
    }

    void Self::PaintHero(Graphics& g, const RECT& rc, float scale)
    {
        float pad = 40 * scale;
        // big title
        ThemeText(g, L"Welcome", Theme_Font(F_BIGTITLE), TC(C_TEXT),
                  RectF(pad, 26 * scale, 560 * scale, 60 * scale),
                  StringAlignmentNear, StringAlignmentNear);
        ThemeText(g, L"Win32 API tabanlı görsel GUI oluşturucu — menüden seçin, "
                     L"\"+\" ile instance oluşturun, olaylara çift tıklayın.",
                  Theme_Font(F_BODY), TC(C_TEXT2),
                  RectF(pad, 92 * scale, 620 * scale, 40 * scale),
                  StringAlignmentNear, StringAlignmentNear);
        // primary button: Yeni Proje
        float bw = 190 * scale, bh = 44 * scale;
        rcNew = { (int)pad, (int)(140 * scale), (int)(pad + bw), (int)(140 * scale + bh) };
        DrawPill(g, pad, 140 * scale, bw, bh, L"＋ Yeni Proje",
                 Theme_Font(F_BODY_SB),
                 hotNew ? TC(C_ACCENT_HOVER) : TC(C_ACCENT), TC(C_WHITE), hotNew);
        // secondary: VS'yi Tara
        float sx = pad + bw + 14 * scale;
        float sw = 150 * scale;
        rcScan = { (int)sx, (int)(140 * scale), (int)(sx + sw), (int)(140 * scale + bh) };
        DrawPill(g, sx, 140 * scale, sw, bh, L"VS'yi Tara",
                 Theme_Font(F_BODY_SB), TC(C_CARD), TC(C_TEXT), hotScan);
        StrokeRound(g, sx, 140 * scale, sw, bh, bh / 2, TC(C_BORDER), 1.2f);
        // refresh (small, right of list header below? keep here for simplicity)
        float rx = sx + sw + 14 * scale;
        float rw = 44 * scale;
        rcRefresh = { (int)rx, (int)(140 * scale), (int)(rx + rw), (int)(140 * scale + bh) };
        DrawPill(g, rx, 140 * scale, rw, bh, L"↻", Theme_Font(F_BODY_SB),
                 TC(C_CARD), hotRefresh ? TC(C_ACCENT) : TC(C_TEXT2), hotRefresh);
        StrokeRound(g, rx, 140 * scale, rw, bh, bh / 2, TC(C_BORDER), 1.2f);
    }

    void Self::PaintList(Graphics& g, const RECT& rc, float scale)
    {
        float pad = 40 * scale;
        float y = 10 * scale;
        // ---- section: running visual studio ----
        ThemeText(g, L"Çalışan Visual Studio (devenv.exe)", Theme_Font(F_H2), TC(C_TEXT),
                  RectF(pad, y, 480 * scale, 26 * scale),
                  StringAlignmentNear, StringAlignmentNear);
        y += 32 * scale;
        int vsCount = g_mw ? (int)g_mw->vs.size() : 0;
        if (vsCount == 0) {
            ThemeText(g, L"Çalışan devenv.exe bulunamadı — Visual Studio 2019'u açın "
                         L"veya \"VS'yi Tara\"ya basın.",
                      Theme_Font(F_SMALL), TC(C_TEXT3),
                      RectF(pad, y, 640 * scale, 22 * scale),
                      StringAlignmentNear, StringAlignmentCenter);
            y += 34 * scale;
        } else {
            float cw = std::min(300 * scale, ((float)rc.right - pad * 2 - 12 * scale) / vsCount - 8 * scale);
            float x = pad;
            for (int i = 0; i < vsCount; i++) {
                const VSInstance& v = g_mw->vs[i];
                std::wstring label = !v.solutionName.empty() ? v.solutionName
                                    : Fmt(L"devenv_%lu", (unsigned long)v.pid);
                bool hot = (hotVs == i);
                float ch = 34 * scale;
                Row r; r.isVS = true; r.vsIndex = i; r.index = -1;
                r.rc = { (int)x, (int)y, (int)(x + cw), (int)(y + ch) };
                rows.push_back(r);
                DrawPill(g, x, y, cw, ch, L"", Theme_Font(F_BODY),
                         hot ? TC(C_SEL_SOFT) : TC(C_CARD), TC(C_TEXT), hot);
                StrokeRound(g, x, y, cw, ch, ch / 2, TC(C_BORDER), 1.0f);
                DrawDot(g, x + 16 * scale, y + ch / 2, 4.5f * scale, TC(C_OK));
                ThemeText(g, label, Theme_Font(F_BODY_SB), TC(C_TEXT),
                          RectF(x + 28 * scale, y, cw - 40 * scale, ch),
                          StringAlignmentNear, StringAlignmentCenter);
                x += cw + 10 * scale;
                if (x > (float)rc.right - 60 * scale) break;
            }
            y += 44 * scale;
        }

        // ---- section: recent projects (newest first) ----
        ThemeText(g, L"Son Projeler", Theme_Font(F_H2), TC(C_TEXT),
                  RectF(pad, y, 300 * scale, 26 * scale),
                  StringAlignmentNear, StringAlignmentNear);
        ThemeText(g, L"(en yeni önce)", Theme_Font(F_SMALL), TC(C_TEXT3),
                  RectF(pad + 150 * scale, y + 4 * scale, 200 * scale, 22 * scale),
                  StringAlignmentNear, StringAlignmentCenter);
        y += 34 * scale;

        std::vector<ProjectRecord> emptyRecents;
        auto& recents = g_mw ? g_mw->recents : emptyRecents;
        if (recents.empty()) {
            // dashed empty card
            float eh = 70 * scale;
            Pen pen(TC(C_BORDER), 1.4f);
            pen.SetDashStyle(DashStyleDash);
            auto p = RoundPath(pad, y, (float)rc.right - pad * 2, eh, 14 * scale);
            g.DrawPath(&pen, p.get());
            ThemeText(g, L"Henüz proje yok — \"＋ Yeni Proje\" ile başlayın veya "
                         L"VS2019'da bir çözüm açın.",
                      Theme_Font(F_BODY), TC(C_TEXT3),
                      RectF(pad, y, (float)rc.right - pad * 2, eh),
                      StringAlignmentCenter, StringAlignmentCenter);
            return;
        }
        float rowH = 64 * scale, gap = 10 * scale;
        float rw = (float)rc.right - pad * 2;
        for (int i = 0; i < (int)recents.size(); i++) {
            const ProjectRecord& p = recents[i];
            float ry = y + i * (rowH + gap) - scroll;
            if (ry + rowH < (float)rc.top || ry > (float)rc.bottom) {
                Row r; r.isVS = false; r.index = i; r.vsIndex = -1;
                r.rc = { 0, 0, 0, 0 };
                rows.push_back(r);
                continue;
            }
            bool hot = (hotRow == i);
            Row r; r.isVS = false; r.index = i; r.vsIndex = -1;
            r.rc = { (int)pad, (int)ry, (int)(pad + rw), (int)(ry + rowH) };
            rows.push_back(r);
            DrawShadow(g, pad, ry, rw, rowH, 14 * scale, hot ? 3 : 2);
            FillRound(g, pad, ry, rw, rowH, 14 * scale, TC(C_CARD));
            StrokeRound(g, pad, ry, rw, rowH, 14 * scale, TC(C_BORDER), 1.0f);
            // avatar circle with initial
            float ar = 18 * scale;
            float acx = pad + 30 * scale, acy = ry + rowH / 2;
            Color av = hot ? TC(C_ACCENT) : TC(C_ACCENT_SOFT);
            DrawDot(g, acx, acy, ar, av);
            std::wstring initial = p.name.empty() ? L"?" : UpperW(p.name.substr(0, 1));
            ThemeText(g, initial, Theme_Font(F_BODY_SB),
                      hot ? TC(C_WHITE) : TC(C_ACCENT),
                      RectF(acx - ar, acy - ar, ar * 2, ar * 2),
                      StringAlignmentCenter, StringAlignmentCenter);
            // name + meta
            std::wstring meta;
            if (p.fromVS) meta += L"● devenv bağlı   ";
            if (!p.sourceDir.empty()) meta += p.sourceDir + L"   ";
            meta += L"son açılış: " + RelTime(p.lastOpened);
            ThemeText(g, p.name, Theme_Font(F_BODY_SB), TC(C_TEXT),
                      RectF(pad + 60 * scale, ry + 10 * scale, rw - 220 * scale, 24 * scale),
                      StringAlignmentNear, StringAlignmentNear);
            ThemeText(g, meta, Theme_Font(F_SMALL), TC(C_TEXT3),
                      RectF(pad + 60 * scale, ry + 34 * scale, rw - 220 * scale, 20 * scale),
                      StringAlignmentNear, StringAlignmentNear);
            // open pill
            float ow = 74 * scale, oh = 28 * scale;
            float ox = pad + rw - ow - 14 * scale;
            float oy = ry + (rowH - oh) / 2;
            DrawPill(g, ox, oy, ow, oh, L"Aç", Theme_Font(F_BODY_SB),
                     hot ? TC(C_ACCENT) : TC(C_GRAY_SOFT),
                     hot ? TC(C_WHITE) : TC(C_TEXT), hot);
        }
    }

    static WNDPROC s_proc = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        if (m == WM_NCCREATE) {
            auto* cs = (CREATESTRUCTW*)l;
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return DefWindowProcW(h, m, w, l);
        }
        auto* self = (Self*)GetWindowLongPtrW(h, GWLP_USERDATA);
        return self ? self->Proc(h, m, w, l) : DefWindowProcW(h, m, w, l);
    };

    ATOM RegisterClass1(HINSTANCE inst)
    {
        WNDCLASSW wc;
        if (GetClassInfoW(inst, L"CGC_Welcome", &wc)) return 0;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = s_proc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
        wc.lpszClassName = L"CGC_Welcome";
        wc.hbrBackground = nullptr;
        return RegisterClassW(&wc);
    }

    HWND Create(HWND parent)
    {
        Self* s = new Self(nullptr);
        return CreateWindowExW(0, L"CGC_Welcome", nullptr, WS_CHILD | WS_VISIBLE,
                               0, 0, 100, 100, parent, nullptr,
                               (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), s);
    }

    void Refresh(HWND h)
    {
        InvalidateRect(h, nullptr, FALSE);
    }
}
