// ============================================================================
//  Theme.cpp — Apple-style theme implementation (GDI+)
// ============================================================================
#include "Theme.h"

using namespace Gdiplus;

// ===========================================================================
// Colors
// ===========================================================================
static Color g_pal[C_COUNT] =
{
    Color(255, 0xF5, 0xF5, 0xF7),   // C_BG
    Color(255, 0xFF, 0xFF, 0xFF),   // C_CARD
    Color(255, 0x1D, 0x1D, 0x1F),   // C_TEXT
    Color(255, 0x6E, 0x6E, 0x73),   // C_TEXT2
    Color(255, 0xAE, 0xAE, 0xB2),   // C_TEXT3
    Color(255, 0x00, 0x71, 0xE3),   // C_ACCENT
    Color(255, 0x00, 0x77, 0xED),   // C_ACCENT_HOVER
    Color(232, 0x00, 0x71, 0xE3),   // C_ACCENT_SOFT (alpha)
    Color(255, 0xD2, 0xD2, 0xD7),   // C_BORDER
    Color(255, 0xFF, 0x3B, 0x30),   // C_DANGER
    Color(255, 0x34, 0xC7, 0x59),   // C_OK
    Color(255, 0xFF, 0x9F, 0x0A),   // C_WARN
    Color(255, 0xE8, 0xE8, 0xED),   // C_TABBAR
    Color(255, 0xFF, 0xFF, 0xFF),   // C_TAB_ACTIVE
    Color(255, 0xEB, 0xEB, 0xF0),   // C_HOVER_SOFT
    Color(255, 0xDC, 0xEB, 0xFB),   // C_SEL_SOFT
    Color(255, 0xFF, 0xFF, 0xFF),   // C_FIELD
    Color(255, 0xF0, 0xF0, 0xF2),   // C_GRAY_SOFT
    Color(40,  0x00, 0x00, 0x00),   // C_SHADOW
    Color(255, 0xFF, 0xFF, 0xFF),   // C_WHITE
    Color(255, 0x00, 0x00, 0x00),   // C_BLACK
};
static Color g_palHover[C_COUNT]; // slightly darkened fills computed lazily

Color TC(int which)
{
    (void)g_palHover;
    if (which < 0 || which >= C_COUNT) return Color(255, 0, 0, 0);
    return g_pal[which];
}

Color HexCol(const std::wstring& hex, Color def)
{
    std::wstring h = hex;
    if (!h.empty() && h[0] == L'#') h = h.substr(1);
    if (h.size() != 6) return def;
    wchar_t* end = nullptr;
    long v = wcstol(h.c_str(), &end, 16);
    if (!end || *end) return def;
    return Color(255,
                 (BYTE)((v >> 16) & 0xFF),
                 (BYTE)((v >> 8) & 0xFF),
                 (BYTE)(v & 0xFF));
}

// ===========================================================================
// Fonts
// ===========================================================================
static HFONT  g_fonts[F_COUNT] = { 0 };
static UINT   g_dpiCache = 0;

// Resolved GDI+ families: the first candidate that GDI+ can actually create.
// Needed because GDI+ FontFamily() does NOT fall back like the GDI mapper does
// (observed under Wine: only bundled Tahoma is visible to GDI+), while GDI
// TextOut falls back gracefully. On real Windows "Segoe UI" resolves on the
// first try, so behaviour there is unchanged.
static Gdiplus::FontFamily* g_gpFamily = nullptr;
static std::wstring         g_gpFamilyName = L"Tahoma";
static Gdiplus::FontFamily* g_gpMonoFamily = nullptr;
static std::wstring         g_gpMonoName   = L"Consolas";

static void ResolveFontFamilies()
{
    if (g_gpFamily) return;
    const wchar_t* cands[] = {
        L"Segoe UI", L"Segoe UI Variable", L"Tahoma", L"Arial", L"Arimo",
        L"Helvetica Neue", L"Liberation Sans", L"Noto Sans", L"DejaVu Sans",
        L"Verdana"
    };
    for (const wchar_t* c : cands) {
        Gdiplus::FontFamily* ff = new Gdiplus::FontFamily(c);
        if (ff->GetLastStatus() == Gdiplus::Ok) {
            g_gpFamily = ff; g_gpFamilyName = c; break;
        }
        delete ff;
    }
    if (!g_gpFamily) { g_gpFamily = new Gdiplus::FontFamily(L"Tahoma"); g_gpFamilyName = L"Tahoma"; }

    const wchar_t* monos[] = {
        L"Consolas", L"Cascadia Mono", L"Cascadia Code", L"Courier New",
        L"DejaVu Sans Mono", L"Liberation Mono", L"Sarasa Mono SC", L"Courier"
    };
    for (const wchar_t* c : monos) {
        Gdiplus::FontFamily* ff = new Gdiplus::FontFamily(c);
        if (ff->GetLastStatus() == Gdiplus::Ok) {
            g_gpMonoFamily = ff; g_gpMonoName = c; break;
        }
        delete ff;
    }
    if (!g_gpMonoFamily) { g_gpMonoFamily = g_gpFamily; g_gpMonoName = g_gpFamilyName; }
}

std::wstring Theme_FontFamily()
{
    ResolveFontFamilies();
    return g_gpFamilyName;
}

static int PtToPx(int pt, UINT dpi) { return -(int)lround(pt * dpi / 72.0); }

static HFONT MakeFont(int pt, UINT dpi, int weight, bool mono = false)
{
    return CreateFontW(PtToPx(pt, dpi), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       mono ? L"Consolas" : Theme_FontFamily().c_str());
}

void Theme_Reload(HWND anyWnd)
{
    UINT dpi = anyWnd ? DpiForWnd(anyWnd) : 96;
    if (dpi == g_dpiCache && g_fonts[F_BODY]) return;
    g_dpiCache = dpi;
    struct Spec { int which, pt, weight; bool mono; } specs[] = {
        { F_TITLE,    26, 600, false },
        { F_H2,       17, 600, false },
        { F_BODY,     13, 400, false },
        { F_BODY_SB,  13, 600, false },
        { F_SMALL,    11, 400, false },
        { F_SMALL_SB, 11, 600, false },
        { F_TINY,      9, 400, false },
        { F_MONO,     12, 400, true  },
        { F_BIGTITLE, 34, 600, false },
    };
    for (auto& s : specs) {
        if (g_fonts[s.which]) DeleteObject(g_fonts[s.which]);
        g_fonts[s.which] = MakeFont(s.pt, dpi, s.weight, s.mono);
    }
}

HFONT Theme_Font(int which)
{
    if (which < 0 || which >= F_COUNT) return nullptr;
    if (!g_fonts[which]) Theme_Reload(nullptr);
    return g_fonts[which];
}
int Theme_FontPt(int which)
{
    static const int pts[F_COUNT] = { 26, 17, 13, 13, 11, 11, 9, 12, 34 };
    return pts[which];
}

// ===========================================================================
// GDI+ lifetime
// ===========================================================================
static ULONG_PTR g_gdipToken = 0;
void Theme_InitGdiPlus()
{
    GdiplusStartupInput si;
    GdiplusStartup(&g_gdipToken, &si, nullptr);
}
void Theme_ShutdownGdiPlus()
{
    if (g_gdipToken) { GdiplusShutdown(g_gdipToken); g_gdipToken = 0; }
}
ULONG_PTR Theme_GdiPlusToken() { return g_gdipToken; }

// ===========================================================================
// Shapes
// ===========================================================================
std::unique_ptr<GraphicsPath> RoundPath(float x, float y, float w, float h, float r)
{
    auto p = std::make_unique<GraphicsPath>();
    if (w < 2) w = 2;
    if (h < 2) h = 2;
    if (r > h / 2) r = h / 2;
    if (r > w / 2) r = w / 2;
    if (r <= 0.5f) {
        p->AddRectangle(RectF(x, y, w, h));
        return p;
    }
    float d = r * 2;
    p->AddArc(x, y, d, d, 180, 90);
    p->AddArc(x + w - d, y, d, d, 270, 90);
    p->AddArc(x + w - d, y + h - d, d, d, 0, 90);
    p->AddArc(x, y + h - d, d, d, 90, 90);
    p->CloseFigure();
    return p;
}

void FillRound(Graphics& g, float x, float y, float w, float h, float r, Color c)
{
    auto p = RoundPath(x, y, w, h, r);
    SolidBrush br(c);
    g.FillPath(&br, p.get());
}

void StrokeRound(Graphics& g, float x, float y, float w, float h, float r,
                 Color c, float width)
{
    auto p = RoundPath(x, y, w, h, r);
    Pen pen(c, width);
    g.DrawPath(&pen, p.get());
}

void FillRectC(Graphics& g, float x, float y, float w, float h, Color c)
{
    SolidBrush br(c);
    g.FillRectangle(&br, x, y, w, h);
}

void DrawShadow(Graphics& g, float x, float y, float w, float h, float r, int depth)
{
    // cheap layered shadow: soft alpha rings expanding outward
    for (int i = depth; i >= 1; i--) {
        int a = 6 + (depth - i) * 2;
        if (a > 34) a = 34;
        float grow = (float)i * 1.6f;
        FillRound(g, x - grow, y - grow + 2, w + grow * 2, h + grow * 2, r + grow,
                  Color((BYTE)a, 0x00, 0x00, 0x00));
    }
}

// ===========================================================================
// Text
// ===========================================================================
static StringFormat* SF(int alignH, int alignV, bool ellipsis)
{
    StringFormat* sf = new StringFormat(StringFormat::GenericDefault());
    sf->SetAlignment((StringAlignment)alignH);
    sf->SetLineAlignment((StringAlignment)alignV);
    sf->SetFormatFlags(StringFormatFlagsNoWrap);
    sf->SetTrimming(ellipsis ? StringTrimmingEllipsisCharacter : StringTrimmingNone);
    return sf;
}

static Font* FontFrom(HDC hdc, HFONT hf)
{
    if (!hf) return nullptr;
    ResolveFontFamilies();
    // native path first (real Windows): GDI+ reads the HFONT directly
    Font* f = new Font(hdc, hf);
    if (f->GetLastStatus() == Gdiplus::Ok) return f;
    delete f;
    // fallback: rebuild from the HFONT's metrics using a GDI+-visible family
    HGDIOBJ old = SelectObject(hdc, hf);
    TEXTMETRICW tm; memset(&tm, 0, sizeof(tm));
    BOOL haveTm = GetTextMetricsW(hdc, &tm);
    wchar_t face[64] = { 0 };
    GetTextFaceW(hdc, 64, face);
    SelectObject(hdc, old);
    int  px     = haveTm ? (tm.tmHeight - tm.tmInternalLeading) : 16;
    int  weight = haveTm ? tm.tmWeight : FW_NORMAL;
    bool italic = haveTm ? (tm.tmItalic != 0) : false;
    bool mono   = (wcscmp(face, L"Consolas") == 0);
    FontStyle st = FontStyleRegular;
    if (weight >= 600 && italic) st = FontStyleBoldItalic;
    else if (weight >= 600)      st = FontStyleBold;
    else if (italic)             st = FontStyleItalic;
    const Gdiplus::FontFamily* ff = mono ? g_gpMonoFamily : g_gpFamily;
    f = new Font(ff, (REAL)std::max(6, px), st, UnitPixel);
    if (f->GetLastStatus() == Gdiplus::Ok) return f;
    delete f;
    // last resort: default family, regular
    return new Font(g_gpFamily, (REAL)std::max(6, px), FontStyleRegular, UnitPixel);
}

void ThemeText(Graphics& g, const std::wstring& text, HFONT font,
               Color color, const RectF& rc, int alignH, int alignV, bool ellipsis)
{
    if (text.empty()) return;
    HDC hdc = g.GetHDC();
    Font* f = FontFrom(hdc, font);
    g.ReleaseHDC(hdc);
    if (!f) return;
    SolidBrush br(color);
    StringFormat* sf = SF(alignH, alignV, ellipsis);
    g.DrawString(text.c_str(), (INT)text.size(), f, rc, sf, &br);
    delete sf;
    delete f;
}

RectF MeasureTextW(Graphics& g, const std::wstring& text, HFONT font)
{
    RectF out(0, 0, 0, 0);
    if (text.empty()) return out;
    HDC hdc = g.GetHDC();
    Font* f = FontFrom(hdc, font);
    g.ReleaseHDC(hdc);
    if (!f) return out;
    StringFormat* sf = SF(StringAlignmentNear, StringAlignmentCenter, false);
    g.MeasureString(text.c_str(), (INT)text.size(), f, PointF(0, 0), sf, &out);
    delete sf;
    delete f;
    return out;
}

// ===========================================================================
// Pills / glyphs
// ===========================================================================
void DrawPill(Graphics& g, float x, float y, float w, float h,
              const std::wstring& text, HFONT font,
              Color fill, Color textColor, bool hover, bool selected, float r)
{
    float rr = (r > 0) ? r : h / 2;
    Color f = fill;
    if (hover) {
        BYTE a = f.GetA();
        // darken by 8%
        f = Color(a,
                  (BYTE)std::max(0, f.GetR() - 10),
                  (BYTE)std::max(0, f.GetG() - 10),
                  (BYTE)std::max(0, f.GetB() - 10));
    }
    FillRound(g, x, y, w, h, rr, f);
    if (selected) {
        Pen p(Color(255, 0x00, 0x71, 0xE3), 1.6f);
        auto path = RoundPath(x + 1, y + 1, w - 2, h - 2, rr - 1);
        g.DrawPath(&p, path.get());
    }
    if (!text.empty()) {
        RectF trc(x, y, w, h);
        ThemeText(g, text, font, textColor, trc, StringAlignmentCenter, StringAlignmentCenter);
    }
}

void DrawCloseGlyph(Graphics& g, float cx, float cy, float r, Color col, bool circleBg)
{
    if (circleBg)
        FillRound(g, cx - r, cy - r, r * 2, r * 2, r, Color(255, 0xE3, 0xE3, 0xE8));
    Pen p(col, 1.6f);
    float d = r * 0.52f;
    g.DrawLine(&p, cx - d, cy - d, cx + d, cy + d);
    g.DrawLine(&p, cx - d, cy + d, cx + d, cy - d);
}

void DrawChevron(Graphics& g, float cx, float cy, float r, Color c, bool down)
{
    Pen p(c, 1.6f);
    float d = r * 0.6f;
    if (down) {
        g.DrawLine(&p, cx - d, cy - d / 2, cx, cy + d / 2);
        g.DrawLine(&p, cx, cy + d / 2, cx + d, cy - d / 2);
    } else {
        g.DrawLine(&p, cx - d, cy + d / 2, cx, cy - d / 2);
        g.DrawLine(&p, cx, cy - d / 2, cx + d, cy + d / 2);
    }
}

void DrawPlusGlyph(Graphics& g, float cx, float cy, float r, Color c, float width)
{
    Pen p(c, width);
    p.SetStartCap(LineCapRound);
    p.SetEndCap(LineCapRound);
    g.DrawLine(&p, cx - r, cy, cx + r, cy);
    g.DrawLine(&p, cx, cy - r, cx, cy + r);
}

void DrawDot(Graphics& g, float cx, float cy, float r, Color c)
{
    SolidBrush br(c);
    g.FillEllipse(&br, cx - r, cy - r, r * 2, r * 2);
}

// ===========================================================================
// BufferedPaint
// ===========================================================================
BufferedPaint::BufferedPaint(HWND hwnd, Color bg)
    : hwnd(hwnd), hdc_(nullptr), mem_(nullptr), bmp_(nullptr), oldBmp_(nullptr)
{
    hdc_ = BeginPaint(hwnd, &ps_);
    GetClientRect(hwnd, &rc);
    int w = std::max(1, W()), h = std::max(1, H());
    mem_ = CreateCompatibleDC(hdc_);
    bmp_ = CreateCompatibleBitmap(hdc_, w, h);
    oldBmp_ = (HBITMAP)SelectObject(mem_, bmp_);
    g = new Graphics(mem_);
    g->SetSmoothingMode(SmoothingModeAntiAlias);
    g->SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g->SetPixelOffsetMode(PixelOffsetModeHalf);
    if (bg.GetA() > 0)
        FillRectC(*g, 0, 0, (float)w, (float)h, bg);
}
BufferedPaint::~BufferedPaint()
{
    BitBlt(hdc_, rc.left, rc.top, W(), H(), mem_, 0, 0, SRCCOPY);
    delete g;
    SelectObject(mem_, oldBmp_);
    DeleteObject(bmp_);
    DeleteDC(mem_);
    EndPaint(hwnd, &ps_);
}

// ===========================================================================
// HoverTrack
// ===========================================================================
void HoverTrack::On(HWND hwnd, UINT msg, LPARAM lp)
{
    if (msg == WM_MOUSEMOVE) {
        if (!over) {
            over = true;
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        (void)lp;
    } else if (msg == WM_MOUSELEAVE) {
        over = false;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}
