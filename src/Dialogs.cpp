// ============================================================================
//  Dialogs.cpp — Apple-style modal dialogs: New Project, About, Dependency
// ============================================================================
#include "UI.h"
#include "Theme.h"

using namespace Gdiplus;

// ===========================================================================
// common popup chrome
// ===========================================================================
static void PaintPopupChrome(HWND h, Graphics& g, const RECT& rc,
                             const std::wstring& title, const std::wstring& subtitle,
                             float scale)
{
    DrawShadow(g, 0, 0, (float)rc.right, (float)rc.bottom, 12 * scale, 6);
    FillRound(g, 0, 0, (float)rc.right, (float)rc.bottom, 14 * scale, TC(C_CARD));
    ThemeText(g, title, Theme_Font(F_H2), TC(C_TEXT),
              RectF(24 * scale, 18 * scale, (float)rc.right - 48 * scale, 30 * scale),
              StringAlignmentNear, StringAlignmentNear);
    if (!subtitle.empty())
        ThemeText(g, subtitle, Theme_Font(F_SMALL), TC(C_TEXT2),
                  RectF(24 * scale, 50 * scale, (float)rc.right - 48 * scale, 20 * scale),
                  StringAlignmentNear, StringAlignmentNear);
    // hairline bottom
    FillRectC(g, 12 * scale, (float)rc.bottom - 62 * scale,
              (float)rc.right - 24 * scale, 1, TC(C_BORDER));
}

static LRESULT PopupProcCommon(HWND h, UINT m, WPARAM w, LPARAM l)
{
    // round the popup window region on size
    if (m == WM_SIZE) {
        float scale = ScaleF(h);
        RECT rc; GetClientRect(h, &rc);
        HRGN rgn = CreateRoundRectRgn(0, 0, rc.right + 1, rc.bottom + 1,
                                      (int)(14 * scale), (int)(14 * scale));
        SetWindowRgn(h, rgn, TRUE);
        DeleteObject(rgn);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static ATOM RegPopup(const wchar_t* cls, WNDPROC p, HINSTANCE inst)
{
    WNDCLASSW wc;
    if (GetClassInfoW(inst, cls, &wc)) return 0;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = p;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.lpszClassName = cls;
    wc.style = CS_DROPSHADOW;
    wc.hbrBackground = nullptr;
    return RegisterClassW(&wc);
}

// ===========================================================================
// Name dialog ("Yeni Proje")
// ===========================================================================
struct NameDlg
{
    HWND hwnd = nullptr, hEdit = nullptr;
    bool ok = false;
    std::wstring text;
    RECT rcOk{}, rcCancel{};
    bool hotOk = false, hotCancel = false;
    HoverTrack hover;
    std::wstring title, subtitle, initial;

    LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l);
    void Layout();
};

void NameDlg::Layout()
{
    RECT rc; GetClientRect(hwnd, &rc);
    float scale = ScaleF(hwnd);
    float bh = 34 * scale;
    float by = (float)rc.bottom - 62 * scale + 14 * scale;
    float bw = 110 * scale;
    rcCancel = { (int)((float)rc.right - 24 * scale - bw * 2 - 10 * scale), (int)by,
                 (int)((float)rc.right - 24 * scale - bw - 10 * scale), (int)(by + bh) };
    rcOk = { (int)((float)rc.right - 24 * scale - bw), (int)by,
             (int)((float)rc.right - 24 * scale), (int)(by + bh) };
}

LRESULT NameDlg::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
        case WM_SIZE:
            PopupProcCommon(h, m, w, l);
            Layout();
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_MOUSEMOVE: {
            hover.On(h, m, l);
            POINT pt = { GetXL(l), GetYL(l) };
            hotOk = PtInRect(&rcOk, pt);
            hotCancel = PtInRect(&rcCancel, pt);
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        case WM_MOUSELEAVE:
            hover.On(h, m, l);
            hotOk = hotCancel = false;
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_LBUTTONDOWN: {
            POINT pt = { GetXL(l), GetYL(l) };
            if (PtInRect(&rcOk, pt)) {
                wchar_t buf[256] = { 0 };
                GetWindowTextW(hEdit, buf, 256);
                text = TrimW(buf);
                ok = !text.empty();
                if (ok) PostMessageW(h, WM_CLOSE, 0, 0);
                return 0;
            }
            if (PtInRect(&rcCancel, pt)) { ok = false; PostMessageW(h, WM_CLOSE, 0, 0); return 0; }
            return 0;
        }
        case WM_COMMAND:
            if (HIWORD(w) == EN_CHANGE) InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            BufferedPaint bp(h, Color(0, 0, 0, 0));
            Graphics& g = *bp.g;
            RECT rc = bp.rc;
            float scale = ScaleF(h);
            PaintPopupChrome(h, g, rc, title, subtitle, scale);
            // edit field (frame drawn by us; the EDIT child provides input)
            float ey = 86 * scale, eh = 36 * scale;
            RECT erc; GetClientRect(hEdit, &erc);
            (void)erc;
            FillRound(g, 24 * scale, ey, (float)rc.right - 48 * scale, eh, 9 * scale, TC(C_GRAY_SOFT));
            StrokeRound(g, 24 * scale, ey, (float)rc.right - 48 * scale, eh, 9 * scale,
                        TC(C_BORDER), 1.2f);
            // buttons
            DrawPill(g, (float)rcCancel.left, (float)rcCancel.top,
                     (float)(rcCancel.right - rcCancel.left),
                     (float)(rcCancel.bottom - rcCancel.top),
                     L"İptal", Theme_Font(F_BODY_SB), TC(C_GRAY_SOFT), TC(C_TEXT), hotCancel);
            DrawPill(g, (float)rcOk.left, (float)rcOk.top,
                     (float)(rcOk.right - rcOk.left),
                     (float)(rcOk.bottom - rcOk.top),
                     L"Oluştur", Theme_Font(F_BODY_SB),
                     hotOk ? TC(C_ACCENT_HOVER) : TC(C_ACCENT), TC(C_WHITE), hotOk);
            return 0;
        }
        case WM_DESTROY:
            // NOTE: no PostQuitMessage here — closing a popup dialog must not
            // terminate the whole application; the modal loop exits via
            // the IsWindow() check in ShowNameDialog/ShowInfoDialog.
            return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static LRESULT NameDlgProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)l;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(h, m, w, l);
    }
    auto* self = (NameDlg*)GetWindowLongPtrW(h, GWLP_USERDATA);
    return self ? self->Proc(h, m, w, l) : DefWindowProcW(h, m, w, l);
}

// ---------------------------------------------------------------------------
// tour/demo peek — same visual as ShowNameDialog but without the modal loop
// ---------------------------------------------------------------------------
static NameDlg* g_peekDlg = nullptr;

HWND TourShowNameDialogPeek(HWND parent, const std::wstring& typedText)
{
    if (!g_peekDlg) {
        HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
        RegPopup(L"CGC_NameDlg", NameDlgProc, inst);
        auto* dlg = new NameDlg();
        dlg->title = L"Yeni Proje";
        dlg->subtitle = L"Proje adı girin — bu ad yeni sekmenin başlığı olur";
        dlg->initial = typedText;
        int w = ScaleI(parent, 420), hh = ScaleI(parent, 220);
        RECT pr; GetWindowRect(parent, &pr);
        int x = pr.left + ((pr.right - pr.left) - w) / 2;
        int y = pr.top + ((pr.bottom - pr.top) - hh) / 2;
        dlg->hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"CGC_NameDlg", nullptr,
                                    WS_POPUP | WS_VISIBLE, x, y, w, hh,
                                    parent, nullptr, inst, dlg);
        dlg->hEdit = CreateWindowExW(0, L"EDIT", typedText.c_str(),
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     ScaleI(dlg->hwnd, 30), ScaleI(dlg->hwnd, 90),
                                     w - ScaleI(dlg->hwnd, 60), ScaleI(dlg->hwnd, 30),
                                     dlg->hwnd, nullptr, inst, nullptr);
        SendMessageW(dlg->hEdit, WM_SETFONT, (WPARAM)Theme_Font(F_BODY), TRUE);
        SendMessageW(dlg->hEdit, EM_SETCUEBANNER, TRUE,
                     (LPARAM)L"örn. MyForm");
        SetFocus(dlg->hEdit);
        dlg->Layout();
        g_peekDlg = dlg;
    }
    if (g_peekDlg->hEdit) SetWindowTextW(g_peekDlg->hEdit, typedText.c_str());
    return g_peekDlg ? g_peekDlg->hwnd : nullptr;
}

void TourHideNameDialogPeek()
{
    if (g_peekDlg) {
        if (IsWindow(g_peekDlg->hwnd)) DestroyWindow(g_peekDlg->hwnd);
        delete g_peekDlg;
        g_peekDlg = nullptr;
    }
}

NameDialogResult ShowNameDialog(HWND parent, const std::wstring& title,
                                const std::wstring& subtitle,
                                const std::wstring& initial,
                                const std::wstring& placeholder)
{
    NameDialogResult res;
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
    RegPopup(L"CGC_NameDlg", NameDlgProc, inst);

    NameDlg* dlg = new NameDlg();
    dlg->title = title;
    dlg->subtitle = subtitle;
    dlg->initial = initial;

    float scale = ScaleF(parent);
    int w = ScaleI(parent, 420), hh = ScaleI(parent, 220);
    RECT pr; GetWindowRect(parent, &pr);
    int x = pr.left + ((pr.right - pr.left) - w) / 2;
    int y = pr.top + ((pr.bottom - pr.top) - hh) / 2;

    EnableWindow(parent, FALSE);
    dlg->hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"CGC_NameDlg", nullptr,
                                WS_POPUP | WS_VISIBLE, x, y, w, hh,
                                parent, nullptr, inst, dlg);
    // edit child
    float scale2 = ScaleF(dlg->hwnd);
    dlg->hEdit = CreateWindowExW(0, L"EDIT", initial.c_str(),
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                 ScaleI(dlg->hwnd, 30), ScaleI(dlg->hwnd, 90),
                                 w - ScaleI(dlg->hwnd, 60), ScaleI(dlg->hwnd, 30),
                                 dlg->hwnd, nullptr, inst, nullptr);
    SendMessageW(dlg->hEdit, WM_SETFONT, (WPARAM)Theme_Font(F_BODY), TRUE);
    SendMessageW(dlg->hEdit, EM_SETCUEBANNER, TRUE, (LPARAM)placeholder.c_str());
    (void)scale2;
    SetFocus(dlg->hEdit);
    dlg->Layout();

    // modal loop
    bool dlgOk = false;
    std::wstring dlgText;
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_ESCAPE) { dlgOk = false; break; }
            if (msg.wParam == VK_RETURN) {
                wchar_t buf[256] = { 0 };
                GetWindowTextW(dlg->hEdit, buf, 256);
                dlgText = TrimW(buf);
                dlgOk = !dlgText.empty();
                if (dlgOk) break;
                continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!IsWindow(dlg->hwnd)) break;
    }
    dlgOk = dlg->ok ? true : dlgOk;                 // button path
    if (dlg->ok) dlgText = dlg->text;
    if (IsWindow(dlg->hwnd)) DestroyWindow(dlg->hwnd);
    delete dlg;
    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    res.ok = dlgOk;
    res.text = dlgText;
    return res;
}

// ===========================================================================
// Info dialog (About / Dependency report)
// ===========================================================================
struct InfoDlg
{
    HWND hwnd = nullptr;
    std::vector<std::wstring> lines;
    std::wstring title;
    RECT rcClose{};
    bool hotClose = false;
    HoverTrack hover;

    LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l);
};

LRESULT InfoDlg::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
        case WM_SIZE:
            PopupProcCommon(h, m, w, l);
            {
                RECT rc; GetClientRect(h, &rc);
                float scale = ScaleF(h);
                float bw = 110 * scale, bh = 32 * scale;
                rcClose = { (int)((float)rc.right - 24 * scale - bw),
                            (int)((float)rc.bottom - 54 * scale),
                            (int)((float)rc.right - 24 * scale),
                            (int)((float)rc.bottom - 54 * scale + bh) };
            }
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_MOUSEMOVE: {
            hover.On(h, m, l);
            POINT pt = { GetXL(l), GetYL(l) };
            hotClose = PtInRect(&rcClose, pt);
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        case WM_MOUSELEAVE:
            hover.On(h, m, l);
            hotClose = false;
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case WM_LBUTTONDOWN:
            if (PtInRect(&rcClose, POINT{ GetXL(l), GetYL(l) }))
                PostMessageW(h, WM_CLOSE, 0, 0);
            return 0;
        case WM_MOUSEWHEEL:
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            BufferedPaint bp(h, Color(0, 0, 0, 0));
            Graphics& g = *bp.g;
            RECT rc = bp.rc;
            float scale = ScaleF(h);
            PaintPopupChrome(h, g, rc, title, L"", scale);
            float y = 56 * scale;
            for (const auto& ln : lines) {
                bool isHead = EndsWithW(ln, L":") && ln.size() < 60;
                ThemeText(g, ln, Theme_Font(isHead ? F_SMALL_SB : F_SMALL),
                          isHead ? TC(C_TEXT) : TC(C_TEXT2),
                          RectF(24 * scale, y, (float)rc.right - 48 * scale, 18 * scale),
                          StringAlignmentNear, StringAlignmentNear);
                y += 17 * scale;
            }
            DrawPill(g, (float)rcClose.left, (float)rcClose.top,
                     (float)(rcClose.right - rcClose.left),
                     (float)(rcClose.bottom - rcClose.top),
                     L"Kapat", Theme_Font(F_BODY_SB),
                     hotClose ? TC(C_ACCENT_HOVER) : TC(C_ACCENT), TC(C_WHITE), hotClose);
            return 0;
        }
        case WM_DESTROY:
            // NOTE: no PostQuitMessage here — closing a popup dialog must not
            // terminate the whole application; the modal loop exits via
            // the IsWindow() check in ShowNameDialog/ShowInfoDialog.
            return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static LRESULT InfoDlgProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)l;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(h, m, w, l);
    }
    auto* self = (InfoDlg*)GetWindowLongPtrW(h, GWLP_USERDATA);
    return self ? self->Proc(h, m, w, l) : DefWindowProcW(h, m, w, l);
}

static void ShowInfoDialog(HWND parent, const std::wstring& title,
                           const std::vector<std::wstring>& lines)
{
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
    RegPopup(L"CGC_InfoDlg", InfoDlgProc, inst);
    InfoDlg* dlg = new InfoDlg();
    dlg->title = title;
    dlg->lines = lines;

    int w = ScaleI(parent, 560), hh = ScaleI(parent,
             std::min(560, 90 + (int)lines.size() * 18));
    RECT pr; GetWindowRect(parent, &pr);
    int x = pr.left + ((pr.right - pr.left) - w) / 2;
    int y = pr.top + ((pr.bottom - pr.top) - hh) / 2;

    EnableWindow(parent, FALSE);
    dlg->hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"CGC_InfoDlg", nullptr,
                                WS_POPUP | WS_VISIBLE, x, y, w, hh,
                                parent, nullptr, inst, dlg);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!IsWindow(dlg->hwnd)) break;
    }
    bool ok = dlg->hwnd != nullptr;
    (void)ok;
    if (IsWindow(dlg->hwnd)) DestroyWindow(dlg->hwnd);
    delete dlg;
    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
}

// ===========================================================================
// public helpers
// ===========================================================================
void ShowAboutDialog(HWND parent)
{
    ShowInfoDialog(parent, L"Hakkında", {
        L"CppGUI Creator " APP_VERSION_W " — pure Win32 API görsel GUI oluşturucu.",
        L"",
        L"Özellikler:",
        L"  • Üst menü seçici + seçenekler + \"+\" ile instance oluşturma",
        L"  • Kapatılabilir sekmeler, ilk sekme: Welcome Page",
        L"  • Welcome Page: projeler en yeni önce + Yeni Proje",
        L"  • devenv.exe otomatik algılama (Running Object Table + Toolhelp)",
        L"  • Çift tık -> VS2019 öne gelir, .cpp açılır, satıra gidilir",
        L"  • Win32 sender/e: W32Object* / W32EventArgs* (e->Handled)",
        L"  • Apple tarzı sade modern tema (GDI+, Segoe UI)",
        L"",
        L"Veri kaynağı: Config_info.json (144 eleman / 8.664 property / 5.785 event)",
        L"Lisans: repo sahibine aittir — 0xByteDeveloper",
    });
}

void ShowDependencyReport(HWND parent)
{
    ShowInfoDialog(parent, L"Bağımlılık Raporu", {
        L"Kullanılan kütüphaneler (hepsi Windows SDK ile gelir):",
        L"  user32, gdi32, shell32, advapi32 — çekirdek Win32",
        L"  gdiplus (GDI+) — Apple tarzı çizim ve ClearType metin",
        L"  comctl32 v6 — modern görsel stiller + SetWindowSubclass",
        L"  comdlg32 — dosya seçim iletişimi (yedek cpp seçimi)",
        L"  ole32, oleaut32, uuid — COM/DTE (VS2019 otomasyonu, late binding)",
        L"",
        L"Visual Studio 2019 SDK GEREKMEZ:",
        L"  Derleme için yalnızca Windows 10/11 SDK (VS kurulumunda seçili)",
        L"  DTE erişimi IDispatch late binding ile yapılır (compile-time bağımlılık yok)",
        L"",
        L"VS2019 bulunamazsa yedek davranış:",
        L"  ShellExecute ile .cpp dosyası varsayılan uygulamada açılır",
        L"  handler dosya sonuna eklenir ve satır numarası durum çubuğunda gösterilir",
        L"",
        L"Konfigürasyon verisi: assets/catalog.cfg (Config_info.json'dan üretilir)",
        L"Çalışma verileri: %LOCALAPPDATA%\\CppGUICreatorWin32Api",
    });
}
