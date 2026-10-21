// ============================================================================
//  Designer.cpp — canvas rendering + interaction (DesignerNS::Canvas)
// ============================================================================
#include "DesignerInternal.h"

namespace DesignerNS
{
    std::map<HWND, Host*>& Hosts()
    {
        static std::map<HWND, Host*> m;
        return m;
    }

    // =========================================================================
    // Canvas geometry
    // =========================================================================
    RECT Canvas::FormRect() const
    {
        RECT rc; GetClientRect(hwnd, &rc);
        float s = S();
        int w = (int)(form.w * s), hh = (int)(form.h * s);
        int x = std::max<int>((int)(24 * s), (rc.right - w) / 2);
        int y = std::max<int>((int)(20 * s), (rc.bottom - hh) / 2);
        RECT r = { x, y, x + w, y + hh };
        return r;
    }

    RECT Canvas::CtrlRect(const ControlInstance& c)
    {
        float s = S();
        RECT r;
        r.left   = formRc.left + (int)(c.x * s);
        r.top    = formRc.top  + (int)(c.y * s);
        r.right  = r.left + (int)(c.w * s);
        r.bottom = r.top  + (int)(c.h * s);
        return r;
    }

    int Canvas::HitCtrl(POINT pt)
    {
        for (int i = (int)ctrls.size() - 1; i >= 0; i--) {
            RECT r = CtrlRect(ctrls[i]);
            if (PtInRect(&r, pt)) return i;
        }
        return -1;
    }

    void Canvas::Select(int idx)
    {
        if (selected != idx) {
            selected = idx;
            host->OnSelectionChanged();
        }
    }

    // =========================================================================
    // Canvas interaction
    // =========================================================================
    LRESULT Canvas::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        switch (m) {
            case WM_SIZE:
                formRc = FormRect();
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_SETFOCUS: case WM_KILLFOCUS:
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_MOUSEMOVE: {
                hover.On(h, m, l);
                POINT pt = { GetXL(l), GetYL(l) };
                if (drag == H_NONE) {
                    int hi = HitCtrl(pt);
                    SetCursor(LoadCursorW(nullptr,
                              (LPCWSTR)(hi >= 0 ? IDC_HAND : IDC_ARROW)));
                } else if (selected >= 0 && selected < (int)ctrls.size()) {
                    float s = S();
                    int dx = (int)std::round((pt.x - dragStart.x) / s);
                    int dy = (int)std::round((pt.y - dragStart.y) / s);
                    if (g_mw->snap) { dx = (int)std::round(dx / 8.0) * 8; dy = (int)std::round(dy / 8.0) * 8; }
                    ControlInstance& c = ctrls[selected];
                    switch (drag) {
                        case H_MOVE:
                            c.x = dragOrig.x + dx; c.y = dragOrig.y + dy; break;
                        case H_SE:
                            c.w = std::max(8, dragOrig.w + dx);
                            c.h = std::max(8, dragOrig.h + dy); break;
                        case H_E: c.w = std::max(8, dragOrig.w + dx); break;
                        case H_S: c.h = std::max(8, dragOrig.h + dy); break;
                        default: break;
                    }
                    c.x = std::max(0, std::min(form.w - 8, c.x));
                    c.y = std::max(0, std::min(form.h - 8, c.y));
                    InvalidateRect(h, nullptr, FALSE);
                }
                return 0;
            }
            case WM_MOUSELEAVE:
                hover.On(h, m, l);
                return 0;
            case WM_LBUTTONDOWN: {
                SetFocus(h);
                POINT pt = { GetXL(l), GetYL(l) };
                // resize handles on the current selection (SE / E / S)
                if (selected >= 0 && selected < (int)ctrls.size()) {
                    RECT r = CtrlRect(ctrls[selected]);
                    int hs = ScaleI(h, 6);
                    RECT rSE = { r.right - hs, r.bottom - hs, r.right + hs, r.bottom + hs };
                    RECT rE  = { r.right - hs, r.top, r.right + hs, r.bottom };
                    RECT rS  = { r.left, r.bottom - hs, r.right + hs, r.bottom + hs };
                    if (PtInRect(&rSE, pt)) { drag = H_SE; dragStart = pt; dragOrig = ctrls[selected]; SetCapture(h); return 0; }
                    if (PtInRect(&rE, pt))  { drag = H_E;  dragStart = pt; dragOrig = ctrls[selected]; SetCapture(h); return 0; }
                    if (PtInRect(&rS, pt))  { drag = H_S;  dragStart = pt; dragOrig = ctrls[selected]; SetCapture(h); return 0; }
                }
                int hi = HitCtrl(pt);
                Select(hi);
                if (hi >= 0) {
                    drag = H_MOVE;
                    dragStart = pt;
                    dragOrig = ctrls[hi];
                    SetCapture(h);
                } else {
                    drag = H_NONE;
                }
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            case WM_LBUTTONUP:
                if (drag != H_NONE) {
                    ReleaseCapture();
                    drag = H_NONE;
                    host->OnCanvasEdited();
                }
                return 0;
            case WM_CAPTURECHANGED:
                drag = H_NONE;
                return 0;
            case WM_LBUTTONDBLCLK: {
                POINT pt = { GetXL(l), GetYL(l) };
                int hi = HitCtrl(pt);
                if (hi >= 0) {
                    const ControlInstance& c = ctrls[hi];
                    std::wstring ev = Catalog::DefaultDblEventFor(c.type, false);
                    g_mw->OnControlDblClick(host->project, c.name, c.type, ev);
                } else if (PtInRect(&formRc, pt)) {
                    g_mw->OnFormDblClick(host->project);
                }
                return 0;
            }
            case WM_KEYDOWN:
                if (w == VK_DELETE && selected >= 0 && selected < (int)ctrls.size()) {
                    ctrls.erase(ctrls.begin() + selected);
                    Select(-1);
                    InvalidateRect(h, nullptr, FALSE);
                    host->OnCanvasEdited();
                }
                return 0;
            case WM_USER_REFRESH:
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_PAINT: {
                BufferedPaint bp(h, TC(C_BG));
                formRc = FormRect();
                Paint(*bp.g, bp.rc, ScaleF(h));
                return 0;
            }
        }
        return DefWindowProcW(h, m, w, l);
    }

    // =========================================================================
    // Canvas paint
    // =========================================================================
    void Canvas::Paint(Graphics& g, const RECT& rc, float scale)
    {
        (void)rc;
        RECT f = formRc;
        float fl = (float)f.left, ft = (float)f.top;
        float fw = (float)(f.right - f.left), fh = (float)(f.bottom - f.top);
        DrawShadow(g, fl, ft, fw, fh, 10 * scale, 4);
        FillRound(g, fl, ft, fw, fh, 8 * scale,
                  HexCol(form.backColor, TC(C_CARD)));
        StrokeRound(g, fl, ft, fw, fh, 8 * scale, TC(C_BORDER), 1.2f);
        // title bar
        float tbh = 32 * scale;
        FillRoundTop(g, fl, ft, fw, tbh, 8 * scale, TC(C_GRAY_SOFT));
        ThemeText(g, form.text.empty() ? form.name : form.text, Theme_Font(F_BODY_SB),
                  TC(C_TEXT),
                  RectF(fl + 12 * scale, ft, fw - 110 * scale, tbh),
                  StringAlignmentNear, StringAlignmentCenter);
        float gy = ft + tbh / 2;
        float gx = f.right - 72 * scale;
        DrawCloseGlyph(g, gx, gy, 7 * scale, TC(C_TEXT2), false);
        DrawPlusGlyph(g, gx + 26 * scale, gy, 6 * scale, TC(C_TEXT2), 1.4f);
        {
            Pen p(TC(C_TEXT2), 1.2f);
            g.DrawLine(&p, gx + 46 * scale, gy, gx + 56 * scale, gy);
        }
        // grid dots
        if (g_mw->showGrid) {
            SolidBrush dotB(TC(C_BORDER));
            float step = 16 * scale;
            for (float gx2 = fl + step; gx2 < f.right - 2; gx2 += step)
                for (float gy2 = ft + tbh + step; gy2 < f.bottom - 2; gy2 += step)
                    g.FillRectangle(&dotB, gx2, gy2, 1.4f, 1.4f);
        }
        // controls
        for (size_t i = 0; i < ctrls.size(); i++) {
            if (!ctrls[i].visible) continue;
            RECT cr = CtrlRect(ctrls[i]);
            PaintControlGfx(g, ctrls[i], cr, scale, ((int)i == selected));
        }
        // selection frame + handles
        if (selected >= 0 && selected < (int)ctrls.size()) {
            RECT r = CtrlRect(ctrls[selected]);
            Pen pen(TC(C_ACCENT), 1.6f);
            g.DrawRectangle(&pen, RectF((float)r.left - 2, (float)r.top - 2,
                             (float)(r.right - r.left) + 4,
                             (float)(r.bottom - r.top) + 4));
            float hs = 4.5f * scale;
            PointF pts[4] = {
                { (float)r.left, (float)r.top },
                { (float)r.right, (float)r.top },
                { (float)r.left, (float)r.bottom },
                { (float)r.right, (float)r.bottom },
            };
            for (auto& p : pts) {
                FillRound(g, p.X - hs, p.Y - hs, hs * 2, hs * 2, 2 * scale, TC(C_CARD));
                StrokeRound(g, p.X - hs, p.Y - hs, hs * 2, hs * 2, 2 * scale, TC(C_ACCENT), 1.4f);
            }
        }
        // empty hint
        if (ctrls.empty()) {
            ThemeText(g, L"Boş form — üstteki \"Araç Kutusu\" menüsünden kontrol seçin, "
                         L"\"+\" ile instance oluşturun",
                      Theme_Font(F_SMALL), TC(C_TEXT3),
                      RectF(fl, ft + 40 * scale, fw, 30 * scale),
                      StringAlignmentCenter, StringAlignmentCenter);
        }
        // caption under the form
        ThemeText(g, Fmt(L"%s — %d kontrol  •  çift tık: olay kodu  •  sürükle: taşı  •  Del: sil",
                         host->project.c_str(), (int)ctrls.size()),
                  Theme_Font(F_SMALL), TC(C_TEXT3),
                  RectF(fl, f.bottom + 8 * scale, fw, 22 * scale),
                  StringAlignmentNear, StringAlignmentCenter);
    }

    // =========================================================================
    // control renderings (Apple-clean, Win32-faithful)
    // =========================================================================
    void FillRoundTop(Graphics& g, float x, float y, float w, float h, float r, Color c)
    {
        FillRound(g, x, y, w, h, r, c);
        FillRectC(g, x, y + h / 2, w, h / 2, c);
    }

    void PaintControlGfx(Graphics& g, const ControlInstance& c, const RECT& r,
                         float scale, bool isSelected)
    {
        (void)isSelected;
        float x = (float)r.left, y = (float)r.top;
        float w = (float)(r.right - r.left), h = (float)(r.bottom - r.top);
        if (w <= 0 || h <= 0) return;
        auto getProp = [&](const wchar_t* k) -> std::wstring {
            auto it = c.props.find(k);
            return it == c.props.end() ? std::wstring() : it->second;
        };
        Color back = HexCol(getProp(L"BackColor"), Color(0, 0, 0, 0));
        Color fore = HexCol(getProp(L"ForeColor"), TC(C_TEXT));
        const std::wstring& t = c.text;

        if (c.type == L"Button") {
            DrawPill(g, x, y, w, h, t, Theme_Font(F_BODY_SB), TC(C_ACCENT), TC(C_WHITE), false);
        }
        else if (c.type == L"Label") {
            ThemeText(g, t, Theme_Font(F_BODY), fore, RectF(x, y, w, h),
                      StringAlignmentNear, StringAlignmentCenter);
        }
        else if (c.type == L"TextBox") {
            FillRound(g, x, y, w, h, 6 * scale, TC(C_FIELD));
            StrokeRound(g, x, y, w, h, 6 * scale, TC(C_BORDER), 1.2f);
            ThemeText(g, t, Theme_Font(F_BODY), TC(C_TEXT2),
                      RectF(x + 9 * scale, y, w - 18 * scale, h),
                      StringAlignmentNear, StringAlignmentCenter);
            float cx = x + 11 * scale + MeasureTextW(g, t, Theme_Font(F_BODY)).Width;
            Pen p(TC(C_ACCENT), 1.4f);
            g.DrawLine(&p, cx, y + 6 * scale, cx, y + h - 6 * scale);
        }
        else if (c.type == L"CheckBox" || c.type == L"RadioButton") {
            float bs = 16 * scale;
            float by = y + (h - bs) / 2;
            bool chk = c.props.count(L"Checked") > 0;
            if (c.type == L"CheckBox") {
                FillRound(g, x, by, bs, bs, 4.5f * scale, chk ? TC(C_ACCENT) : TC(C_FIELD));
                StrokeRound(g, x, by, bs, bs, 4.5f * scale, chk ? TC(C_ACCENT) : TC(C_BORDER), 1.2f);
                if (chk) {
                    Pen p(TC(C_WHITE), 1.8f);
                    g.DrawLine(&p, x + 4 * scale, by + bs / 2, x + bs / 2, by + bs - 5 * scale);
                    g.DrawLine(&p, x + bs / 2, by + bs - 5 * scale, x + bs - 4 * scale, by + 4 * scale);
                }
            } else {
                { SolidBrush br(TC(C_FIELD));
                  g.FillEllipse(&br, x, by, bs, bs); }
                Pen p(TC(C_BORDER), 1.2f);
                g.DrawEllipse(&p, x, by, bs, bs);
                if (chk) {
                    float in = bs * 0.32f;
                    { SolidBrush br(TC(C_ACCENT));
                      g.FillEllipse(&br, x + in, by + in, bs - in * 2, bs - in * 2); }
                }
            }
            ThemeText(g, t, Theme_Font(F_BODY), TC(C_TEXT),
                      RectF(x + bs + 7 * scale, y, w - bs - 7 * scale, h),
                      StringAlignmentNear, StringAlignmentCenter);
        }
        else if (c.type == L"GroupBox") {
            float labh = 20 * scale;
            FillRound(g, x, y + labh / 2, w, h - labh / 2, 8 * scale,
                      back.GetA() ? back : TC(C_CARD));
            StrokeRound(g, x, y + labh / 2, w, h - labh / 2, 8 * scale, TC(C_BORDER), 1.2f);
            ThemeText(g, t, Theme_Font(F_SMALL_SB), TC(C_TEXT),
                      RectF(x + 10 * scale, y, w - 20 * scale, labh),
                      StringAlignmentNear, StringAlignmentCenter);
        }
        else if (c.type == L"PictureBox") {
            FillRound(g, x, y, w, h, 8 * scale, TC(C_GRAY_SOFT));
            StrokeRound(g, x, y, w, h, 8 * scale, TC(C_BORDER), 1.2f);
            float mx = x + w / 2 - 17 * scale, my = y + h / 2 - 12 * scale;
            PointF poly[5] = {
                PointF(mx, my + 20 * scale),
                PointF(mx + 12 * scale, my + 2 * scale),
                PointF(mx + 20 * scale, my + 12 * scale),
                PointF(mx + 26 * scale, my + 5 * scale),
                PointF(mx + 34 * scale, my + 20 * scale),
            };
            GraphicsPath mp;
            mp.AddPolygon(poly, 5);
            { SolidBrush br(TC(C_TEXT3));
              g.FillPath(&br, &mp); }
            DrawDot(g, mx + 27 * scale, my + 1 * scale, 3.2f * scale, TC(C_TEXT2));
            ThemeText(g, c.name, Theme_Font(F_TINY), TC(C_TEXT3),
                      RectF(x, y + h - 16 * scale, w, 14 * scale),
                      StringAlignmentCenter, StringAlignmentCenter);
        }
        else if (c.type == L"ComboBox") {
            FillRound(g, x, y, w, h, 6 * scale, TC(C_FIELD));
            StrokeRound(g, x, y, w, h, 6 * scale, TC(C_BORDER), 1.2f);
            DrawChevron(g, x + w - 16 * scale, y + h / 2, 6 * scale, TC(C_TEXT2), true);
            ThemeText(g, t, Theme_Font(F_BODY), TC(C_TEXT2),
                      RectF(x + 9 * scale, y, w - 30 * scale, h),
                      StringAlignmentNear, StringAlignmentCenter);
        }
        else if (c.type == L"ListBox") {
            FillRound(g, x, y, w, h, 6 * scale, TC(C_FIELD));
            StrokeRound(g, x, y, w, h, 6 * scale, TC(C_BORDER), 1.2f);
            float ry = y + 6 * scale;
            for (int i = 0; i < 4; i++) {
                if (ry + 20 * scale > y + h - 4 * scale) break;
                if (i == 0)
                    FillRound(g, x + 4 * scale, ry, w - 8 * scale, 20 * scale, 4 * scale, TC(C_SEL_SOFT));
                std::wstring item = (i == 0 && !t.empty()) ? t : Fmt(L"Öğe %d", i + 1);
                ThemeText(g, item, Theme_Font(F_SMALL), TC(C_TEXT2),
                          RectF(x + 10 * scale, ry, w - 20 * scale, 20 * scale),
                          StringAlignmentNear, StringAlignmentCenter);
                ry += 22 * scale;
            }
        }
        else if (c.type == L"ProgressBar") {
            int val = 70;
            if (!getProp(L"Value").empty()) val = _wtoi(getProp(L"Value").c_str());
            val = std::max(0, std::min(100, val));
            FillRound(g, x, y, w, h, h / 2, TC(C_GRAY_SOFT));
            if (val > 0)
                FillRound(g, x, y, std::max(h, w * val / 100.0f), h, h / 2, TC(C_ACCENT));
        }
        else if (c.type == L"TrackBar") {
            float cy = y + h / 2;
            Pen line(TC(C_BORDER), 2.0f);
            g.DrawLine(&line, x + 8 * scale, cy, x + w - 8 * scale, cy);
            float kn = x + (w - 16 * scale) * 0.4f + 8 * scale;
            { SolidBrush br(TC(C_CARD));
              g.FillEllipse(&br, kn - 8 * scale, cy - 8 * scale, 16 * scale, 16 * scale); }
            { Pen penP(TC(C_ACCENT), 1.6f);
              g.DrawEllipse(&penP, kn - 8 * scale, cy - 8 * scale, 16 * scale, 16 * scale); }
        }
        else if (c.type == L"DateTimePicker") {
            FillRound(g, x, y, w, h, 6 * scale, TC(C_FIELD));
            StrokeRound(g, x, y, w, h, 6 * scale, TC(C_BORDER), 1.2f);
            ThemeText(g, t.empty() ? std::wstring(L"01.01.2026") : t,
                      Theme_Font(F_BODY), TC(C_TEXT2),
                      RectF(x + 9 * scale, y, w - 30 * scale, h),
                      StringAlignmentNear, StringAlignmentCenter);
            DrawChevron(g, x + w - 16 * scale, y + h / 2, 6 * scale, TC(C_TEXT2), true);
        }
        else if (c.type == L"Panel" || c.type == L"FlowLayoutPanel") {
            FillRound(g, x, y, w, h, 6 * scale, back.GetA() ? back : TC(C_CARD));
            StrokeRound(g, x, y, w, h, 6 * scale, TC(C_BORDER), 1.2f);
        }
        else if (c.type == L"MenuStrip") {
            FillRoundTop(g, x, y, w, 26 * scale, 4 * scale, TC(C_CARD));
            StrokeRound(g, x, y, w, 26 * scale, 4 * scale, TC(C_BORDER), 1.0f);
            float ix = x + 10 * scale;
            const wchar_t* items[] = { L"Dosya", L"Düzenle", L"Görünüm", L"Yardım" };
            for (const wchar_t* it : items) {
                ThemeText(g, it, Theme_Font(F_SMALL), TC(C_TEXT),
                          RectF(ix, y, 70 * scale, 26 * scale),
                          StringAlignmentNear, StringAlignmentCenter);
                ix += 64 * scale;
            }
        }
        else {
            // generic catalog element: dashed placeholder card
            FillRound(g, x, y, w, h, 6 * scale, TC(C_CARD));
            Pen dash(TC(C_TEXT3), 1.2f);
            dash.SetDashStyle(DashStyleDash);
            auto p = RoundPath(x, y, w, h, 6 * scale);
            g.DrawPath(&dash, p.get());
            ThemeText(g, c.name, Theme_Font(F_SMALL), TC(C_TEXT2),
                      RectF(x, y, w, h), StringAlignmentCenter, StringAlignmentCenter);
        }
    }
}
