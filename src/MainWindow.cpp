// ============================================================================
//  MainWindow.cpp — app shell orchestration
// ============================================================================
#include "UI.h"
#include "Theme.h"
#include "CodeGen.h"
#include "DTEBridge.h"

using namespace Gdiplus;

MainWindow* g_mw = nullptr;
HWND        g_hMain = nullptr;
bool        g_autotour = false;
static void TourMark(int n);

// locate the bundled W32GuiRuntime.h directory (dev tree or installed layout)
static std::wstring RuntimeDir()
{
    std::wstring cands[] = {
        ExeDirW() + L"\\..\\runtime",
        ExeDirW() + L"\\..\\..\\runtime",
        ExeDirW() + L"\\runtime",
    };
    for (auto& c : cands)
        if (FileExistsW(c + L"\\W32GuiRuntime.h")) return c;
    return ExeDirW();
}

// ===========================================================================
// window proc plumbing
// ===========================================================================
static LRESULT MainProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)l;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(h, m, w, l);
    }
    auto* self = (MainWindow*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (!self) return DefWindowProcW(h, m, w, l);

    switch (m) {
        case WM_SIZE:
            self->LayoutChildren();
            return 0;
        case WM_GETMINMAXINFO: {
            auto* mmi = (MINMAXINFO*)l;
            mmi->ptMinTrackSize = { ScaleI(h, 980), ScaleI(h, 620) };
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_CLOSE:
            // save active layout before exit
            if (self->active >= 0 && self->active < (int)self->tabs.size() &&
                !self->tabs[self->active].welcome)
                DesignerNS::SaveNow(self->tabs[self->active].host);
            DestroyWindow(h);
            return 0;
        case WM_TIMER:
            if (w == 0x7E57 && g_autotour) { self->TourTick(); return 0; }
            break;
        case WM_APP + 0x100:                       // tour: run modal flow from a
            self->CreateNewProjectFlow();          // queued message (NOT from the
            return 0;                              // timer tick — avoids deep re-entry)
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

// ===========================================================================
// lifecycle
// ===========================================================================
bool MainWindow::Create(HINSTANCE inst)
{
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = MainProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.lpszClassName = L"CGC_Main";
    wc.hbrBackground = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);

    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int w = std::min(1500, (int)(wa.right - wa.left) - 60);
    int hh = std::min(940, (int)(wa.bottom - wa.top) - 60);
    int x = wa.left + ((wa.right - wa.left) - w) / 2;
    int y = wa.top + ((wa.bottom - wa.top) - hh) / 2;

    hwnd = CreateWindowExW(0, L"CGC_Main", APP_NAME L" — pure Win32 GUI Creator",
                           WS_OVERLAPPEDWINDOW, x, y, w, hh,
                           nullptr, nullptr, inst, this);
    if (!hwnd) return false;
    g_hMain = hwnd;

    HICON icon = nullptr;
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);

    // children
    TopBarNS::RegisterClass1(inst);
    OptionsNS::RegisterClass1(inst);
    TabBarNS::RegisterClass1(inst);
    StatusNS::RegisterClass1(inst);
    WelcomeNS::RegisterClass1(inst);
    DesignerNS::RegisterClass1(inst);

    hTop    = TopBarNS::Create(hwnd);
    hOpts   = OptionsNS::Create(hwnd);
    hTabs   = TabBarNS::Create(hwnd);
    hStatus = StatusNS::Create(hwnd);

    // initial data
    recents = Store::LoadProjects();
    RefreshVS(false);
    Store::SortRecents(recents);

    OpenWelcomeTab();
    ActivateTab(0);
    SetMenu(MENU_WELCOME);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    LayoutChildren();
    return true;
}

void MainWindow::LayoutChildren()
{
    RECT rc; GetClientRect(hwnd, &rc);
    if (!hTop) return;
    int topH = ScaleI(hwnd, 50);
    int optH = ScaleI(hwnd, 44);
    int tabH = ScaleI(hwnd, 36);
    int stH  = ScaleI(hwnd, 24);
    MoveWindow(hTop,    0, 0, rc.right, topH, TRUE);
    MoveWindow(hOpts,   0, topH, rc.right, optH, TRUE);
    MoveWindow(hTabs,   0, topH + optH, rc.right, tabH, TRUE);
    MoveWindow(hStatus, 0, rc.bottom - stH, rc.right, stH, TRUE);
    int contentTop = topH + optH + tabH;
    int contentH = rc.bottom - stH - contentTop;
    for (auto& t : tabs)
        MoveWindow(t.host, 0, contentTop, rc.right, std::max(0, contentH), TRUE);
    InvalidateRect(hwnd, nullptr, FALSE);
}

// ===========================================================================
// menus & options
// ===========================================================================
void MainWindow::SetMenu(int which)
{
    currentMenu = which;
    TopBarNS::SetActiveMenu(hTop, which);
    RebuildOptions();
    if (which == MENU_WELCOME) {
        // focus welcome tab (create if closed)
        int wi = FindTabByProject(L"");
        if (wi < 0) { OpenWelcomeTab(); wi = FindTabByProject(L""); }
        if (wi >= 0) ActivateTab(wi);
    } else if (which == MENU_PROJELER) {
        RefreshVS(false);
    } else if (which == MENU_BAGLANTI) {
        RefreshVS(true);
    }
}

void MainWindow::RebuildOptions()
{
    options.clear();
    selectedOption = -1;
    if (currentMenu == MENU_WELCOME) {
        OptionItem o; o.text = L"Welcome Page sekmesini göster"; o.isAction = true;
        options.push_back(o);
        OptionItem o2; o2.text = L"Yeni Proje..."; o2.isAction = true; o2.tag = 1;
        options.push_back(o2);
    } else if (currentMenu == MENU_PROJELER) {
        VSDiscovery::MergeRunning(recents);
        Store::SortRecents(recents);
        // running devenv first (x1 source of truth)
        for (auto& v : vs) {
            if (v.solutionName.empty()) continue;
            OptionItem o;
            o.text = Fmt(L"● %s  (devenv PID %lu)", v.solutionName.c_str(),
                         (unsigned long)v.pid);
            o.isAction = true;
            options.push_back(o);
        }
        // disk / stored projects (newest first)
        for (auto& p : recents) {
            if (p.name.empty()) continue;
            bool already = false;
            for (auto& o : options)
                if (o.text.find(p.name) != std::wstring::npos) { already = true; break; }
            if (already) continue;
            OptionItem o;
            o.text = p.fromVS ? (L"● " + p.name) : p.name;
            o.isAction = true;
            options.push_back(o);
        }
        OptionItem o2; o2.text = L"Yeni Proje..."; o2.isAction = true; o2.tag = 1;
        options.push_back(o2);
    } else if (currentMenu == MENU_ARAC) {
        // toolbox from Config_info catalog, grouped by category order
        for (const auto& cat : Catalog::Categories()) {
            for (const auto* cm : Catalog::ByCategory(cat)) {
                OptionItem o;
                o.text = cm->type;
                o.tag = 2;                       // toolbox entry
                o.isAction = false;              // selectable (highlight)
                options.push_back(o);
            }
        }
    } else if (currentMenu == MENU_GORUNUM) {
        OptionItem g; g.text = showGrid ? L"Izgara: açık" : L"Izgara: kapalı";
        g.isAction = true; g.tag = 10; options.push_back(g);
        OptionItem s; s.text = snap ? L"Yapışma (8px): açık" : L"Yapışma (8px): kapalı";
        s.isAction = true; s.tag = 11; options.push_back(s);
        OptionItem pr; pr.text = propsVisible ? L"Özellik paneli: açık" : L"Özellik paneli: kapalı";
        pr.isAction = true; pr.tag = 12; options.push_back(pr);
    } else if (currentMenu == MENU_BAGLANTI) {
        if (vs.empty()) {
            OptionItem o; o.text = L"Çalışan devenv.exe yok — \"VS'yi Tara\"ya basın";
            o.isAction = true; options.push_back(o);
        }
        for (auto& v : vs) {
            OptionItem o;
            o.text = !v.solutionName.empty()
                     ? Fmt(L"%s — %s", v.solutionName.c_str(),
                           v.cppPath.empty() ? L"cpp bulunamadı" : NameFromPathW(v.cppPath).c_str())
                     : Fmt(L"devenv PID %lu (çözüm adı yok)", (unsigned long)v.pid);
            o.isAction = true; o.tag = 20; options.push_back(o);
        }
        OptionItem scan; scan.text = L"VS'yi Tara"; scan.isAction = true; scan.tag = 21;
        options.push_back(scan);
        OptionItem dep; dep.text = L"Bağımlılık Raporu"; dep.isAction = true; dep.tag = 22;
        options.push_back(dep);
    } else if (currentMenu == MENU_YARDIM) {
        OptionItem a; a.text = L"Hakkında"; a.isAction = true; a.tag = 30; options.push_back(a);
        OptionItem d; d.text = L"Bağımlılık Raporu"; d.isAction = true; d.tag = 22; options.push_back(d);
        OptionItem e; e.text = L"Aktif projeyi aktar (main_w32.cpp üret)"; e.isAction = true; e.tag = 31; options.push_back(e);
    }
    OptionsNS::SetOptions(hOpts, options, selectedOption);
}

void MainWindow::OnTopBarVSChip()
{
    SetMenu(MENU_BAGLANTI);
}

// ===========================================================================
// option click (including "+")
// ===========================================================================
void MainWindow::OnOptionClick(int index)
{
    if (index == -1) {
        // "+" pill — behavior depends on current menu
        if (currentMenu == MENU_ARAC) {
            int sel = selectedOption;
            if (sel < 0 || sel >= (int)options.size()) {
                Status(L"Önce araç kutusundan bir kontrol seçin");
                return;
            }
            AddControlToActive(options[sel].text);
        } else if (currentMenu == MENU_PROJELER || currentMenu == MENU_WELCOME) {
            CreateNewProjectFlow();
        } else {
            CreateNewProjectFlow();
        }
        return;
    }
    if (index < 0 || index >= (int)options.size()) return;
    OptionItem& o = options[index];

    if (currentMenu == MENU_ARAC) {
        selectedOption = index;
        OptionsNS::SetOptions(hOpts, options, selectedOption);
        Status(L"Seçili: " + o.text + L"  —  \"+\" ile instance oluştur");
        return;
    }
    if (currentMenu == MENU_WELCOME) {
        if (o.tag == 1) { CreateNewProjectFlow(); return; }
        int wi = FindTabByProject(L"");
        if (wi < 0) OpenWelcomeTab(); else ActivateTab(wi);
        return;
    }
    if (currentMenu == MENU_PROJELER) {
        if (o.tag == 1) { CreateNewProjectFlow(); return; }
        // derive project name from the option text ("● name  (devenv PID..)")
        std::wstring t = o.text;
        if (StartsWithW(t, L"● ")) t = t.substr(2);
        size_t cut = t.find(L"  (");
        if (cut != std::wstring::npos) t = t.substr(0, cut);
        t = TrimW(t);
        if (!t.empty()) OpenProjectTab(t);
        return;
    }
    if (currentMenu == MENU_GORUNUM) {
        if (o.tag == 10) showGrid = !showGrid;
        else if (o.tag == 11) snap = !snap;
        else if (o.tag == 12) {
            propsVisible = !propsVisible;
            for (auto& t : tabs)
                if (!t.welcome) DesignerNS::SetPropsVisible(t.host, propsVisible);
        }
        RebuildOptions();
        for (auto& t : tabs)
            if (!t.welcome) { DesignerNS::RefreshProps(t.host); InvalidateRect(t.host, nullptr, FALSE); }
        return;
    }
    if (currentMenu == MENU_BAGLANTI) {
        if (o.tag == 21) { RefreshVS(true); return; }
        if (o.tag == 22) { ShowDependencyReport(hwnd); return; }
        if (o.tag == 20) {
            std::wstring t = o.text;
            size_t cut = t.find(L" — ");
            if (cut != std::wstring::npos) t = t.substr(0, cut);
            if (!t.empty()) OpenProjectTab(t);
            return;
        }
        return;
    }
    if (currentMenu == MENU_YARDIM) {
        if (o.tag == 30) { ShowAboutDialog(hwnd); return; }
        if (o.tag == 22) { ShowDependencyReport(hwnd); return; }
        if (o.tag == 31) { ExportActiveProject(); return; }
        return;
    }
}

// ===========================================================================
// tabs
// ===========================================================================
void MainWindow::ActivateTab(int idx)
{
    if (idx < 0 || idx >= (int)tabs.size()) return;
    active = idx;
    for (size_t i = 0; i < tabs.size(); i++)
        ShowWindow(tabs[i].host, (int)i == active ? SW_SHOW : SW_HIDE);
    TabBarNS::SetTabs(hTabs, tabs, active);
    LayoutChildren();
    // touch recents
    if (!tabs[active].welcome) {
        ProjectRecord* p = Store::FindProject(recents, tabs[active].project);
        if (p) { p->lastOpened = NowUnix(); Store::SaveProjects(recents); }
    }
}

void MainWindow::CloseTab(int idx)
{
    if (idx < 0 || idx >= (int)tabs.size()) return;
    // Welcome Page can be closed too (reopen from top menu)
    if (!tabs[idx].welcome) DesignerNS::SaveNow(tabs[idx].host);
    DestroyWindow(tabs[idx].host);
    tabs.erase(tabs.begin() + idx);
    if (active >= (int)tabs.size()) active = (int)tabs.size() - 1;
    ActivateTab(std::max(0, active));
    Status(tabs.empty() ? L"Tüm sekmeler kapandı (Welcome: üst menüden geri açılır)"
                        : L"Sekme kapatıldı");
}

int MainWindow::FindTabByProject(const std::wstring& name)
{
    for (size_t i = 0; i < tabs.size(); i++) {
        if (tabs[i].welcome && name.empty()) return (int)i;
        if (!tabs[i].welcome && LowerW(tabs[i].project) == LowerW(name)) return (int)i;
    }
    return -1;
}

void MainWindow::OpenWelcomeTab()
{
    Tab t;
    t.welcome = true;
    t.host = WelcomeNS::Create(hwnd);
    tabs.push_back(t);
    TabBarNS::SetTabs(hTabs, tabs, active);
    WelcomeNS::Refresh(t.host);
}

bool MainWindow::OpenProjectTab(const std::wstring& name, std::wstring* err)
{
    if (name.empty()) { if (err) *err = L"boş isim"; return false; }
    int ex = FindTabByProject(name);
    if (ex >= 0) { ActivateTab(ex); return true; }

    // record (create if new)
    ProjectRecord* p = Store::FindProject(recents, name);
    if (!p) {
        ProjectRecord np;
        np.name = name;
        np.created = NowUnix();
        np.lastOpened = NowUnix();
        Store::UpsertProject(recents, np);
        p = Store::FindProject(recents, name);
    }
    p->lastOpened = NowUnix();
    // enrich from VS when possible
    for (auto& v : vs) {
        if (LowerW(v.solutionName) == LowerW(name)) {
            p->fromVS = true;
            p->pid = (int)v.pid;
            if (p->slnPath.size() < v.slnPath.size()) p->slnPath = v.slnPath;
            if (p->sourceDir.size() < v.sourceDir.size()) p->sourceDir = v.sourceDir;
            if (p->cppPath.size() < v.cppPath.size()) p->cppPath = v.cppPath;
        }
    }
    Store::SaveProjects(recents);

    Tab t;
    t.welcome = false;
    t.project = name;
    t.host = DesignerNS::Create(hwnd, name);
    tabs.push_back(t);
    ActivateTab((int)tabs.size() - 1);
    Status(L"Proje açıldı: " + name);
    return true;
}

// ===========================================================================
// data flows
// ===========================================================================
void MainWindow::RefreshVS(bool notify)
{
    vs = VSDiscovery::ScanRunning();
    TopBarNS::SetVSCount(hTop, (int)vs.size());
    VSDiscovery::MergeRunning(recents);
    Store::SortRecents(recents);
    Store::SaveProjects(recents);
    RebuildOptions();
    TabBarNS::SetTabs(hTabs, tabs, active);
    for (auto& t : tabs)
        if (t.welcome) WelcomeNS::Refresh(t.host);
    if (notify)
        Status(vs.empty() ? L"Çalışan devenv.exe bulunamadı"
                          : Fmt(L"%d çalışan Visual Studio bulundu", (int)vs.size()));
}

void MainWindow::RefreshRecents()
{
    // merge on-disk VS projects into recents
    auto disk = VSDiscovery::ScanDiskProjects();
    for (auto& d : disk) {
        ProjectRecord* p = Store::FindProject(recents, d.name);
        if (!p) { Store::UpsertProject(recents, d); }
        else {
            if (p->slnPath.size() < d.slnPath.size()) p->slnPath = d.slnPath;
            if (p->sourceDir.size() < d.sourceDir.size()) p->sourceDir = d.sourceDir;
            if (p->cppPath.size() < d.cppPath.size()) p->cppPath = d.cppPath;
        }
    }
    Store::SortRecents(recents);
    Store::SaveProjects(recents);
    for (auto& t : tabs)
        if (t.welcome) WelcomeNS::Refresh(t.host);
}

void MainWindow::AddControlToActive(const std::wstring& type)
{
    // no designer tab? create an untitled project first
    if (active < 0 || active >= (int)tabs.size() || tabs[active].welcome) {
        // find any designer tab, else create AdsızN
        int di = -1;
        for (size_t i = 0; i < tabs.size(); i++)
            if (!tabs[i].welcome) { di = (int)i; break; }
        if (di < 0) {
            int n = 1;
            std::wstring name;
            do { name = Fmt(L"Adsız%d", n++); } while (FindTabByProject(name) >= 0);
            OpenProjectTab(name);
            for (size_t i = 0; i < tabs.size(); i++)
                if (!tabs[i].welcome && tabs[i].project == name) di = (int)i;
        }
        ActivateTab(di);
    }
    DesignerNS::AddControl(tabs[active].host, type);
    Status(type + L" instance oluşturuldu");
}

void MainWindow::CreateNewProjectFlow()
{
    NameDialogResult r = ShowNameDialog(hwnd, L"Yeni Proje",
                                        L"Proje adı girin — bu ad yeni sekmenin başlığı olur",
                                        L"", L"örn. MyForm");
    if (!r.ok || r.text.empty()) return;
    std::wstring name = NormalizeIdentW(r.text);
    if (name.empty()) { Status(L"Geçersiz proje adı"); return; }
    if (FindTabByProject(name) >= 0) { Status(L"Bu ada sahip sekme zaten açık"); return; }
    OpenProjectTab(name);
}

void MainWindow::Status(const std::wstring& s)
{
    StatusNS::SetText(hStatus, s);
}

// ===========================================================================
// double-click flows — the heart of the VS2019 connection
// ===========================================================================
void MainWindow::OnControlDblClick(const std::wstring& project,
                                   const std::wstring& ctrlName,
                                   const std::wstring& ctrlType,
                                   const std::wstring& ev)
{
    ProjectRecord* p = Store::FindProject(recents, project);
    if (!p) {
        ProjectRecord np; np.name = project; np.created = NowUnix(); np.lastOpened = NowUnix();
        Store::UpsertProject(recents, np);
        p = Store::FindProject(recents, project);
    }

    // locate cpp
    if (p->cppPath.empty()) {
        p->cppPath = VSDiscovery::FindCppForProject(p->name, p->sourceDir);
        if (!p->cppPath.empty()) Store::SaveProjects(recents);
    }
    if (p->cppPath.empty()) {
        // let the user pick the cpp manually
        OPENFILENAMEW ofn;
        wchar_t file[MAX_PATH] = L"";
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = L"C++ Kaynak (*.cpp)\0*.cpp\0Tüm Dosyalar\0*.*\0";
        ofn.lpstrFile = file;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrTitle = L"Bağlı .cpp dosyasını seçin (VS projesi)";
        if (GetOpenFileNameW(&ofn)) {
            p->cppPath = file;
            p->sourceDir = DirFromPathW(file);
            Store::SaveProjects(recents);
            Status(L"Proje bağlandı: " + p->cppPath);
        } else {
            Status(L".cpp bulunamadı — önce VS2019 projesini açın veya cpp seçin");
            return;
        }
    }

    // generate/locate handler
    CodeGen::EnsureResult r = CodeGen::EnsureHandler(
        p->cppPath, ctrlName, ev, false, RuntimeDir());

    // bring VS forward + open + scroll (best-effort)
    HWND vsHwnd = nullptr;
    DWORD vsPid = 0;
    for (auto& v : vs) {
        if (LowerW(v.solutionName) == LowerW(project)) { vsHwnd = v.hwnd; vsPid = v.pid; break; }
    }
    std::wstring dteErr;
    bool viaDTE = false;
    if (vsHwnd) {
        viaDTE = DTE::OpenFileAtLine(vsHwnd, vsPid, p->cppPath, r.line, &dteErr);
    }
    if (!viaDTE) {
        // fallback: open with default association, VS usually grabs it
        if (vsHwnd) DTE::ActivateVS(vsHwnd);
        ShellExecuteW(hwnd, L"open", p->cppPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    std::wstring msg = r.existed
        ? Fmt(L"%s zaten var — satır %d", r.handler.c_str(), r.line)
        : Fmt(L"%s oluşturuldu — satır %d", r.handler.c_str(), r.line);
    if (r.runtimeCopied) msg += L"  •  W32GuiRuntime.h projeye kopyalandı";
    if (vsHwnd && !viaDTE) msg += L"  •  (DTE: " + dteErr + L")";
    Status(msg);
}

void MainWindow::OnFormDblClick(const std::wstring& project)
{
    // Generate <Project>_Load — Win32 equivalent of MyForm_Load
    OnControlDblClick(project, project, L"Form", L"Load");
}

// ===========================================================================
// export
// ===========================================================================
void MainWindow::ExportActiveProject()
{
    if (active < 0 || active >= (int)tabs.size() || tabs[active].welcome) {
        Status(L"Aktif proje yok — önce bir proje sekmesi açın");
        return;
    }
    std::wstring err;
    if (DesignerNS::Export(tabs[active].host, &err)) {
        Status(L"Aktarıldı: " + err + L"  (main_w32.cpp + W32GuiRuntime.h + build_mingw.bat)");
    } else {
        Status(L"Aktarma hatası: " + err);
    }
}

// ===========================================================================
// AutoTour — scripted UI walkthrough for screenshot demos (--autotour)
// Drives the REAL handlers on a WM_TIMER heartbeat (150 ms per tick), so every
// screen is produced by the production code path. Marks are appended to
// %TEMP%\tour_marks.log for the host-side screenshot sync.
// ===========================================================================
static FILE* g_tourLog  = nullptr;
static int   g_tourTick = 0;

static void TourMark(int n)
{
    if (!g_tourLog) {
        wchar_t tmp[MAX_PATH];
        GetTempPathW(MAX_PATH, tmp);
        g_tourLog = _wfopen((std::wstring(tmp) + L"tour_marks.log").c_str(), L"w, ccs=UTF-8");
    }
    if (g_tourLog) { fwprintf(g_tourLog, L"[mark] %d\n", n); fflush(g_tourLog); }
}

void MainWindow::StartAutoTour()
{
    g_tourTick = 0;
    SetTimer(g_hMain, 0x7E57, 150, nullptr);
}

void MainWindow::TourTick()
{
    static bool inTick = false;                        // re-entrancy guard: modal loops
    if (inTick) return;                                // dispatch our WM_TIMER while we
    inTick = true;                                     // are inside TourTick — skip those
    g_tourTick++;
    switch (g_tourTick) {
        case 6:  TourMark(1); break;                       // initial Welcome
        case 9:  SetMenu(MENU_PROJELER); break;
        case 13: TourMark(2); break;                       // Projeler menu
        case 15: TourShowNameDialogPeek(g_hMain, L""); break;  // dialog (no modal loop)
        case 17: TourMark(3); break;                       // dialog visible
        case 21: TourShowNameDialogPeek(g_hMain, L"DenemeApp"); break;
        case 23: TourMark(4); break;                       // name typed
        case 25: TourHideNameDialogPeek();
                 OpenProjectTab(L"DenemeApp"); break;      // create for real
        case 26: break;
        case 27: break;
        case 28: break;
        case 29: TourMark(5); break;                       // designer (DenemeApp)
        case 30: { // pre-attach a cpp so the dclick flow skips the file picker
                   ProjectRecord* p = Store::FindProject(recents, L"DenemeApp");
                   if (p && p->cppPath.empty()) {
                       wchar_t tmp[MAX_PATH];
                       GetTempPathW(MAX_PATH, tmp);
                       std::wstring cpp = std::wstring(tmp) + L"DenemeApp.cpp";
                       FILE* fp = _wfopen(cpp.c_str(), L"w, ccs=UTF-8");
                       if (fp) { fwprintf(fp, L"// DenemeApp — tour target\n"); fclose(fp); }
                       p->cppPath = cpp;
                       Store::SaveProjects(recents);
                   }
                 } break;
        case 31: OnFormDblClick(L"DenemeApp"); break;      // form dclick -> Load
        case 35: TourMark(6); break;
        case 37: SetMenu(MENU_ARAC); break;
        case 41: TourMark(7); break;                       // toolbox catalog
        case 43: SetMenu(MENU_GORUNUM); break;
        case 47: TourMark(8); break;                       // view settings
        case 49: SetMenu(MENU_BAGLANTI); break;
        case 53: TourMark(9); break;                       // VS connection
        case 55: SetMenu(MENU_YARDIM); break;
        case 59: TourMark(10); break;                      // help/about
        case 61: SetMenu(MENU_WELCOME); break;
        case 65: TourMark(11); break;                      // welcome with project
        case 67: TourShowNameDialogPeek(g_hMain, L"IkinciUygulama"); break;
        case 71: TourHideNameDialogPeek();
                 OpenProjectTab(L"IkinciUygulama"); break;
        case 73: break;
        case 75: break;
        case 79: TourMark(12); break;                      // multiple tabs
        case 81: { int wi = FindTabByProject(L"DenemeApp");
                   if (wi > 0) CloseTab(wi); } break;
        case 85: TourMark(13); break;                      // tab closed
        case 87: SetMenu(MENU_WELCOME); break;
        case 91: TourMark(14);                             // welcome, 2 projects
                 KillTimer(g_hMain, 0x7E57);
                 if (g_tourLog) { fclose(g_tourLog); g_tourLog = nullptr; }
                 break;
    }
    inTick = false;
}
