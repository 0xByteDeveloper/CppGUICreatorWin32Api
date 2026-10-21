// ============================================================================
//  Theme.h — Apple-style theme: colors, fonts, GDI+ drawing helpers
// ============================================================================
#pragma once
#include "stdafx.h"

// ---------------------------------------------------------------------------
// Palette (Apple.com light theme)
// ---------------------------------------------------------------------------
enum ThemeCol
{
    C_BG,          // #F5F5F7  app background
    C_CARD,        // #FFFFFF  cards / fields
    C_TEXT,        // #1D1D1F  primary text
    C_TEXT2,       // #6E6E73  secondary text
    C_TEXT3,       // #AEAEB2  tertiary
    C_ACCENT,      // #0071E3  apple blue
    C_ACCENT_HOVER,// #0077ED
    C_ACCENT_SOFT, // #E8F1FB  soft blue fill
    C_BORDER,      // #D2D2D7  hairline borders
    C_DANGER,      // #FF3B30
    C_OK,          // #34C759
    C_WARN,        // #FF9F0A
    C_TABBAR,      // #E8E8ED  tab strip background
    C_TAB_ACTIVE,  // #FFFFFF  active tab
    C_HOVER_SOFT,  // #EBEBF0  row hover
    C_SEL_SOFT,    // #DCEBFB  selected row soft
    C_FIELD,       // #FFFFFF  input field
    C_GRAY_SOFT,   // #F0F0F2  disabled / track fill
    C_SHADOW,      // rgba black soft
    C_WHITE,
    C_BLACK,
    C_COUNT
};

Gdiplus::Color TC(int which);                  // theme color
Gdiplus::Color HexCol(const std::wstring& hex, Gdiplus::Color def); // "1D1D1F"

// ---------------------------------------------------------------------------
// Fonts (recreated on DPI change)
// ---------------------------------------------------------------------------
enum ThemeFont
{
    F_TITLE,      // 26 semibold — page titles
    F_H2,         // 17 semibold — section titles
    F_BODY,       // 13 regular
    F_BODY_SB,    // 13 semibold
    F_SMALL,      // 11 regular
    F_SMALL_SB,   // 11 semibold
    F_TINY,       // 9  regular
    F_MONO,       // 12 Consolas
    F_BIGTITLE,   // 34 semibold — welcome hero
    F_COUNT
};
void  Theme_Reload(HWND anyWnd);   // recreate fonts for current DPI
HFONT Theme_Font(int which);
int   Theme_FontPt(int which);     // nominal pt
std::wstring Theme_FontFamily();

// ---------------------------------------------------------------------------
// GDI+ lifetime
// ---------------------------------------------------------------------------
void  Theme_InitGdiPlus();
void  Theme_ShutdownGdiPlus();
ULONG_PTR Theme_GdiPlusToken();

// ---------------------------------------------------------------------------
// Drawing helpers (all coordinates are device pixels)
// ---------------------------------------------------------------------------
std::unique_ptr<Gdiplus::GraphicsPath> RoundPath(float x, float y, float w, float h, float r);
void FillRound(Gdiplus::Graphics& g, float x, float y, float w, float h, float r, Gdiplus::Color c);
void StrokeRound(Gdiplus::Graphics& g, float x, float y, float w, float h, float r,
                 Gdiplus::Color c, float width = 1.0f);
void FillRectC(Gdiplus::Graphics& g, float x, float y, float w, float h, Gdiplus::Color c);
void DrawShadow(Gdiplus::Graphics& g, float x, float y, float w, float h, float r, int depth);

void ThemeText(Gdiplus::Graphics& g, const std::wstring& text, HFONT font,
               Gdiplus::Color color, const Gdiplus::RectF& rc,
               int alignH = Gdiplus::StringAlignmentNear,
               int alignV = Gdiplus::StringAlignmentCenter,
               bool ellipsis = true);
Gdiplus::RectF MeasureTextW(Gdiplus::Graphics& g, const std::wstring& text, HFONT font);

// Apple pill button; r==0 -> fully rounded (h/2)
void DrawPill(Gdiplus::Graphics& g, float x, float y, float w, float h,
              const std::wstring& text, HFONT font,
              Gdiplus::Color fill, Gdiplus::Color textColor,
              bool hover = false, bool selected = false, float r = 0);

// Apple-style close glyph (×) inside a soft circle when hovered
void DrawCloseGlyph(Gdiplus::Graphics& g, float cx, float cy, float r,
                    Gdiplus::Color col, bool circleBg);

// chevron / plus / dot helpers
void DrawChevron(Gdiplus::Graphics& g, float cx, float cy, float r, Gdiplus::Color c, bool down);
void DrawPlusGlyph(Gdiplus::Graphics& g, float cx, float cy, float r, Gdiplus::Color c, float width);
void DrawDot(Gdiplus::Graphics& g, float cx, float cy, float r, Gdiplus::Color c);

// ---------------------------------------------------------------------------
// Double-buffered paint (RAII)
// ---------------------------------------------------------------------------
struct BufferedPaint
{
    BufferedPaint(HWND hwnd, Gdiplus::Color bg);
    ~BufferedPaint();
    Gdiplus::Graphics* g;
    HWND    hwnd;
    RECT    rc;          // full client rect
    int     W() const { return rc.right - rc.left; }
    int     H() const { return rc.bottom - rc.top; }
private:
    HDC     hdc_;
    HDC     mem_;
    HBITMAP bmp_;
    HBITMAP oldBmp_;
    PAINTSTRUCT ps_;
};

// hover tracking helper
struct HoverTrack
{
    bool over = false;
    void On(HWND hwnd, UINT msg, LPARAM lp);
};

inline bool PtInRc(const RECT* r, int x, int y)
{
    return x >= r->left && x < r->right && y >= r->top && y < r->bottom;
}

// focus rectangle for keyboard nav (unused v1) — reserved
