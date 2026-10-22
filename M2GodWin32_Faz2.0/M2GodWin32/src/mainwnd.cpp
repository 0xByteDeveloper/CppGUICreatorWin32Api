// mainwnd.cpp — ana çerçeve: menü, toolbar, paneller, diyaloglar, komutlar (Bölüm 1)
#include "mainwnd.h"
#include "app.h"
#include "catalog.h"
#include "model.h"
#include "factory.h"
#include "surface.h"
#include "toolbox.h"
#include "props.h"
#include "codegen.h"
#include "vsint.h"
#include <commdlg.h>
#include <shellapi.h>
#include <objbase.h>
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

// komut idleri
enum {
    CMD_NEW = 2001, CMD_OPEN, CMD_SAVE, CMD_SAVEAS, CMD_GENCODE, CMD_PREVIEW, CMD_EXIT,
    CMD_CUT = 2101, CMD_COPY, CMD_PASTE, CMD_DELETE, CMD_UNDO, CMD_REDO, CMD_FRONT, CMD_BACK,
    CMD_VIEW_TB = 2201, CMD_VIEW_PROPS, CMD_VIEW_GRID, CMD_VIEW_STATUS,
    CMD_VS_CONNECT = 2301, CMD_VS_DISCONNECT, CMD_VS_SYNC, CMD_VS_OPENFOLDER,
    CMD_ABOUT = 2401, CMD_ESC_CANCEL = 2150,
};

static HWND s_wnd = nullptr, s_toolbar = nullptr, s_status = nullptr;
static HWND s_toolbox = nullptr, s_props = nullptr, s_surface = nullptr;
static std::wstring s_file;            // .m2god proje dosyası
static Ctrl* s_clipboard = nullptr;
static bool s_showToolbox = true, s_showProps = true, s_showGrid = true, s_showStatus = true;
bool g_gridVisible = true;             // ızgara noktaları (surface çizer)
static std::wstring pendingOpen;   // giriş ekranından gelecek .m2god/.cpp yolu

// otomatik kayıt (bağlı proje klasöründeki .m2god) debounce zamanlayıcısı
#define IDT_AUTOSAVE 7

// --- ortak diyalog pencere sınıfı ---------------------------------------------
static void EnsureDialogClass() {
    static bool reg = false;
    if (reg) return;
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);   // siyah arka plan olmasın
    wc.lpszClassName = L"M2GOD_CODEDLG";
    RegisterClassW(&wc);
    reg = true;
}
static void RunModal(HWND dlg, HWND parent);   // ileri bildirim (modal pompa)

// --- diyalog: kod önizleme ---------------------------------------------------
static void ShowCodePreview(HWND parent) {
    EnsureDialogClass();
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"M2GOD_CODEDLG",
        L"Üretilen Kod — Form1.cpp / Form1.h",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        parent ? 260 : 200, 140, 780, 560, parent, nullptr, g_hInst, nullptr);
    // sekmeli görünüm: cpp/h toggle butonu + salt okunur edit
    HWND toggle = CreateWindowExW(0, L"BUTTON", L"Form1.h'ye Geç",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 8, 130, 28, dlg, (HMENU)3001, g_hInst, nullptr);
    HWND copy = CreateWindowExW(0, L"BUTTON", L"Kopyala",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 148, 8, 90, 28, dlg, (HMENU)3002, g_hInst, nullptr);
    HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
        10, 44, 744, 470, dlg, (HMENU)3003, g_hInst, nullptr);
    SendMessageW(edit, WM_SETFONT, (WPARAM)g_fontMono, TRUE);
    SendMessageW(edit, EM_SETLIMITTEXT, 0, 0);
    std::wstring* which = new std::wstring(L"cpp");
    auto fill = [&](HWND dlg, HWND edit, std::wstring* which) {
        std::wstring code = (*which == L"cpp") ? codegen::GenerateForm1Cpp() : codegen::GenerateForm1H();
        SetWindowTextW(edit, code.c_str());
        SetWindowTextW(toggle, (*which == L"cpp") ? L"Form1.h'ye Geç" : L"Form1.cpp'ye Geç");
        (void)dlg;
    };
    struct Ctx { HWND dlg, edit, toggle; std::wstring* which; } ctx = { dlg, edit, toggle, which };
    // orijinal proc sakla
    WNDPROC oldProc = (WNDPROC)GetWindowLongPtrW(dlg, GWLP_WNDPROC);
    SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)(+[](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        static Ctx* ctx = nullptr;
        if (m == WM_CREATE) {}
        // ctx'yi prop'tan al
        if (!ctx) ctx = (Ctx*)GetPropW(h, L"m2ctx");
        switch (m) {
        case WM_COMMAND: {
            if (LOWORD(w) == 3001 && ctx) {
                *ctx->which = (*ctx->which == L"cpp") ? L"h" : L"cpp";
                std::wstring code = (*ctx->which == L"cpp") ? codegen::GenerateForm1Cpp() : codegen::GenerateForm1H();
                SetWindowTextW(ctx->edit, code.c_str());
                SetWindowTextW(ctx->toggle, (*ctx->which == L"cpp") ? L"Form1.h'ye Geç" : L"Form1.cpp'ye Geç");
            } else if (LOWORD(w) == 3002 && ctx) {
                int len = GetWindowTextLengthW(ctx->edit);
                std::wstring buf(len + 1, 0);
                GetWindowTextW(ctx->edit, &buf[0], len + 1);
                if (OpenClipboard(h)) {
                    EmptyClipboard();
                    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
                    memcpy(GlobalLock(hg), buf.c_str(), (len + 1) * sizeof(wchar_t));
                    GlobalUnlock(hg);
                    SetClipboardData(CF_UNICODETEXT, hg);
                    CloseClipboard();
                }
                Status(L"Kod panoya kopyalandı");
            }
            return 0;
        }
        case WM_SIZE: {
            if (ctx) {
                RECT rc; GetClientRect(h, &rc);
                MoveWindow(ctx->edit, 10, 44, rc.right - 20, rc.bottom - 54, TRUE);
            }
            break;
        }
        case WM_DESTROY: {
            if (ctx) { delete ctx->which; delete ctx; ctx = nullptr; }
            RemovePropW(h, L"m2ctx");
            break;
        }
        }
        return DefWindowProcW(h, m, w, l);
    }));
    SetPropW(dlg, L"m2ctx", (HANDLE)&ctx);
    fill(dlg, edit, which);
    RunModal(dlg, parent);   // MODAL: stack ctx güvenli
}

// ===PART2===

// --- ortak modal pompa (sahipsiz diyaloglar için) --------------------------------
// NOT: stack'te veri + modal-olmayan diyalog = dangling pointer çökmesi (eski hata).
// ctx artık HEAP'te tutulur ve diyalog bu pompa ile MODAL çalışır.
static void RunModal(HWND dlg, HWND parent) {
    if (parent) EnableWindow(parent, FALSE);
    ShowWindow(dlg, SW_SHOWNORMAL);
    UpdateWindow(dlg);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_QUIT) { PostQuitMessage((int)msg.wParam); break; }
        if (!IsDialogMessageW(dlg, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        if (!IsWindow(dlg)) break;
    }
    if (parent) { EnableWindow(parent, TRUE); SetForegroundWindow(parent); SetFocus(parent); }
}

// VS örneğinin satır metni: "PID 1234 | Çözüm: Foo | Proje: Bar.vcxproj | DTE ✓"
static std::wstring InstLine(const vsint::VSInstance& i) {
    wchar_t buf[768];
    std::wstring proj;
    if (!i.projects.empty()) proj = GetFileName(i.projects[0]);
    else if (!i.projectName.empty()) proj = i.projectName;
    std::wstring dte = i.hasDte ? L"DTE ✓" : L"pencere";
    swprintf(buf, 768, L"PID %lu  |  %ls  |  %ls  |  %ls",
             (unsigned long)i.pid,
             i.solutionName.empty() ? L"(çözüm yok)" : i.solutionName.c_str(),
             proj.empty() ? L"-" : proj.c_str(),
             dte.c_str());
    return buf;
}

// örnek + proje seçimi (vcxproj LİSTESİ diyaloğu) → connect → tasarım çözümlemesi
// Kullanıcı isteği: PID'in dosya konumu otomatik bulunsun, .vcxproj'lar LİSTEDEN
// seçilsin — dosya konumu elle seçmek zorunluluğu kalmasın.

// dizindeki *.m2god tasarımlarını listeler
static std::vector<std::wstring> ListDesignFiles(const std::wstring& dir) {
    std::vector<std::wstring> out;
    WIN32_FIND_DATAW fd;
    HANDLE hf = FindFirstFileW((dir + L"\\*.m2god").c_str(), &fd);
    if (hf == INVALID_HANDLE_VALUE) return out;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            out.push_back(dir + L"\\" + fd.cFileName);
    } while (FindNextFileW(hf, &fd));
    FindClose(hf);
    return out;
}

// diyalog sonuçları (ctx WM_DESTROY'da silindiği için statikte taşınır)
static int g_pickResult = 0;
static int g_pickIndex = -1;

// --- diyalog: proje (.vcxproj) seçimi — çözümde bulunanlar LİSTEDEN seçilir ------
// dönüş: 1 seçildi (chosen dolu), 0 iptal
static int ShowProjectPickDialog(HWND parent, const vsint::VSInstance& inst,
                                 const std::vector<std::wstring>& projects, std::wstring& chosen) {
    if (projects.empty()) return 0;
    EnsureDialogClass();
    g_pickResult = 0; g_pickIndex = -1;
    wchar_t cap[512];
    swprintf(cap, 512, L"Proje Seç — PID %lu (%ls)",
             (unsigned long)inst.pid,
             inst.solutionName.empty() ? L"çözüm adı yok" : inst.solutionName.c_str());
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"M2GOD_CODEDLG", cap,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 340, 240, 660, 320, parent, nullptr, g_hInst, nullptr);
    SetWindowLongW(dlg, GWL_STYLE, (GetWindowLongW(dlg, GWL_STYLE) & ~WS_MAXIMIZEBOX) | WS_CAPTION);
    HWND hint = CreateWindowExW(0, L"STATIC",
        L"PID'in çözümünde bulunan projeler — listeden .vcxproj seçin (çift tık = seç):",
        WS_CHILD | WS_VISIBLE, 10, 8, 620, 20, dlg, nullptr, g_hInst, nullptr);
    SendMessageW(hint, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    HWND lb = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        10, 32, 628, 190, dlg, (HMENU)4101, g_hInst, nullptr);
    SendMessageW(lb, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    for (auto& p : projects) {
        // "Adı — klasör" biçiminde göster
        std::wstring line = GetFileName(p) + L"   —   " + GetFileDir(p);
        SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)line.c_str());
    }
    SendMessageW(lb, LB_SETCURSEL, 0, 0);
    HWND btnOk = CreateWindowExW(0, L"BUTTON", L"Bu Projeye Bağlan",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 400, 236, 150, 30, dlg, (HMENU)4102, g_hInst, nullptr);
    HWND btnNo = CreateWindowExW(0, L"BUTTON", L"İptal",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 558, 236, 100, 30, dlg, (HMENU)4103, g_hInst, nullptr);
    SendMessageW(btnOk, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(btnNo, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)(+[](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        switch (m) {
        case WM_COMMAND: {
            int id = LOWORD(w), note = HIWORD(w);
            if (id == 4103) { g_pickResult = 0; DestroyWindow(h); return 0; }
            if (id == 4102 || (id == 4101 && note == LBN_DBLCLK)) {
                int sel = (int)SendMessageW(GetDlgItem(h, 4101), LB_GETCURSEL, 0, 0);
                if (sel < 0) {
                    MessageBoxW(h, L"Lütfen listeden bir proje seçin.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                g_pickIndex = sel; g_pickResult = 1; DestroyWindow(h); return 0;
            }
            return 0;
        }
        case WM_CLOSE: g_pickResult = 0; DestroyWindow(h); return 0;
        }
        return DefWindowProcW(h, m, w, l);
    }));
    RunModal(dlg, parent);
    int r = g_pickResult; g_pickResult = 0;
    if (r == 1 && g_pickIndex >= 0 && g_pickIndex < (int)projects.size())
        chosen = projects[g_pickIndex];
    else r = 0;
    g_pickIndex = -1;
    return r;
}

// --- diyalog: proje için kayıtlı tasarım bulundu — seçenekler ----------------------
// Kullanıcı isteği: "o pidi seçince buna dair var diyerek otomatik yüklensin;
// istersek öncekini silmeden yeni formla, istersek öncekini silip açalım."
// dönüş: 1=yükle(loadFile) 2=yeni-koru(newFile) 3=yeni-sil(newFile) 0=iptal
static int g_designAction = 0;
static int g_designIndex = -1;
static std::wstring g_designNewFile;

static int ShowDesignOptionsDialog(HWND parent, const std::wstring& projDir,
                                   const std::vector<std::wstring>& designs,
                                   std::wstring& loadFile, std::wstring& newFile) {
    EnsureDialogClass();
    g_designAction = 0; g_designIndex = -1; g_designNewFile.clear();
    int sel0 = 0;   // varsayılan: en yeni (son) tasarım
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"M2GOD_CODEDLG",
        L"Bu Proje İçin Kayıtlı Tasarım Var", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        360, 240, 600, 340, parent, nullptr, g_hInst, nullptr);
    SetWindowLongW(dlg, GWL_STYLE, (GetWindowLongW(dlg, GWL_STYLE) & ~WS_MAXIMIZEBOX) | WS_CAPTION);
    wchar_t hint[512];
    swprintf(hint, 512, L"%ls klasöründe %u tasarım bulundu:", projDir.c_str(), (unsigned)designs.size());
    HWND hHint = CreateWindowExW(0, L"STATIC", hint, WS_CHILD | WS_VISIBLE, 10, 8, 570, 20, dlg, nullptr, g_hInst, nullptr);
    SendMessageW(hHint, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    HWND lb = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        10, 32, 570, 110, dlg, (HMENU)4201, g_hInst, nullptr);
    SendMessageW(lb, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    for (auto& d : designs) SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)GetFileName(d).c_str());
    SendMessageW(lb, LB_SETCURSEL, sel0, 0);
    HWND btnLoad = CreateWindowExW(0, L"BUTTON", L"Önceki Tasarımı YÜKLE",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 10, 156, 270, 32, dlg, (HMENU)4202, g_hInst, nullptr);
    HWND btnKeep = CreateWindowExW(0, L"BUTTON", L"YENİ FORM — öncekini KORU",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 290, 156, 290, 32, dlg, (HMENU)4203, g_hInst, nullptr);
    HWND btnDel = CreateWindowExW(0, L"BUTTON", L"YENİ FORM — öncekini SİL",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 196, 270, 32, dlg, (HMENU)4204, g_hInst, nullptr);
    HWND btnCancel = CreateWindowExW(0, L"BUTTON", L"İptal",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 290, 196, 290, 32, dlg, (HMENU)4205, g_hInst, nullptr);
    for (HWND b : { btnLoad, btnKeep, btnDel, btnCancel }) SendMessageW(b, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    // yeni form adı: FormN.m2god (çakışmayan ilk)
    int n = 1;
    auto exists = [&](const std::wstring& f) {
        for (auto& d : designs) if (_wcsicmp(d.c_str(), f.c_str()) == 0) return true;
        return false;
    };
    std::wstring nf;
    do { nf = projDir + L"\\Form" + std::to_wstring(n++) + L".m2god"; } while (exists(nf));
    g_designNewFile = nf;
    SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)(+[](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        switch (m) {
        case WM_COMMAND: {
            switch (LOWORD(w)) {
            case 4202: {
                int sel = (int)SendMessageW(GetDlgItem(h, 4201), LB_GETCURSEL, 0, 0);
                if (sel < 0) { MessageBoxW(h, L"Listeden bir tasarım seçin.", L"Bilgi", MB_OK); return 0; }
                g_designIndex = sel; g_designAction = 1; DestroyWindow(h); return 0;
            }
            case 4203: g_designAction = 2; DestroyWindow(h); return 0;
            case 4204: g_designAction = 3; DestroyWindow(h); return 0;
            case 4205: g_designAction = 0; DestroyWindow(h); return 0;
            }
            return 0;
        }
        case WM_CLOSE: g_designAction = 0; DestroyWindow(h); return 0;
        }
        return DefWindowProcW(h, m, w, l);
    }));
    RunModal(dlg, parent);
    int act = g_designAction; g_designAction = 0;
    if (act == 1 && g_designIndex >= 0 && g_designIndex < (int)designs.size())
        loadFile = designs[g_designIndex];
    else if (act == 1) act = 0;
    g_designIndex = -1;
    newFile = g_designNewFile;
    return act;
}

// boş (sıfır) tasarım aç + hedef .m2god'a anında kaydet
static void NewBlankDesign(const std::wstring& path) {
    g_model.ClearAll();
    g_model.formProps = { { L"Text", L"Form1" }, { L"Name", L"Form1" }, { L"BackColor", L"Control" } };
    g_model.formW = 800; g_model.formH = 600;
    s_file = path;
    g_model.Save(s_file);
    g_modelDirty = false;
    surface::RecreateAll(s_surface);
    props::Refresh(s_props);
    InvalidateRect(s_surface, nullptr, TRUE);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
    Status(L"Yeni tasarım: " + GetFileName(path) + L"  (bağlı proje klasörüne kaydedildi)");
}

// tasarımı dosyadan yükle (diyalogsuz)
static void LoadDesignSilent(const std::wstring& path) {
    if (g_model.Load(path)) {
        s_file = path;
        surface::RecreateAll(s_surface);
        props::Refresh(s_props);
        InvalidateRect(s_surface, nullptr, TRUE);
        PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
        Status(L"Kayıtlı tasarım yüklendi: " + GetFileName(path));
    } else {
        Status(L"Tasarım okunamadı: " + path + L" — boş form açılıyor");
        NewBlankDesign(path);
    }
}

// örnek + proje seçimi → connect → tasarımla devam (yükle / yeni-koru / yeni-sil)
static bool DoConnectInstance(HWND h, const vsint::VSInstance& inst) {
    std::vector<std::wstring> projects = inst.projects;
    // yedek keşif: örnek listesinde proje yoksa çözümü diskte tekrar ara
    if (projects.empty() && !inst.solutionPath.empty())
        projects = vsint::EnumerateProjectsOfSolution(inst.solutionPath);
    if (projects.empty() && !inst.solutionName.empty()) {
        std::wstring sln = vsint::FindSlnOnDisk(inst.solutionName);
        if (!sln.empty()) projects = vsint::EnumerateProjectsOfSolution(sln);
    }
    std::wstring projPath;
    if (!projects.empty()) {
        std::wstring chosen;
        if (!ShowProjectPickDialog(h, inst, projects, chosen)) return false;
        projPath = chosen;
    } else {
        // son çare: çözüm/proje hiç bulunamadı — elle seç
        wchar_t file[MAX_PATH] = L"";
        OPENFILENAMEW ofn = { sizeof(ofn) };
        ofn.hwndOwner = h;
        ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = L"Proje bulunamadı — .vcxproj dosyasını seçin";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;
        ofn.lpstrFilter = L"Visual C++ Projesi\0*.vcxproj\0Tüm dosyalar\0*.*\0";
        if (GetOpenFileNameW(&ofn)) projPath = GetFileDir(file);
        else return false;
    }
    // Normalize: .vcxproj DOSYA yolu geldiyse klasörüne çevir (projectPath = klasör sözleşmesi)
    {
        std::wstring lp = Lower(projPath);
        if (lp.size() > 8 && lp.rfind(L".vcxproj") == lp.size() - 8)
            projPath = GetFileDir(projPath);
    }
    vsint::Connect(const_cast<vsint::VSInstance&>(inst), projPath);
    vsint::PrepareProjectFiles();   // Form1.h / Form1.cpp + vcxproj
    vsint::SyncToProject(nullptr, L"", nullptr);
    Status(L"Bağlandı: " + inst.solutionName + L" — " + GetFileName(projPath));
    // --- TASARIM KALICILIĞI: proje klasöründeki .m2god'lar -------------------
    std::vector<std::wstring> designs = ListDesignFiles(projPath);
    if (!designs.empty()) {
        std::wstring loadFile, newFile;
        int act = ShowDesignOptionsDialog(h, projPath, designs, loadFile, newFile);
        if (act == 1) LoadDesignSilent(loadFile);
        else if (act == 2) NewBlankDesign(newFile);
        else if (act == 3) {
            for (auto& d : designs) DeleteFileW(d.c_str());
            NewBlankDesign(projPath + L"\\Form1.m2god");
        } else { vsint::Disconnect(); return false; }
    } else {
        NewBlankDesign(projPath + L"\\Form1.m2god");
    }
    return true;
}

// --- diyalog: VS'ye bağlan (modal + heap ctx — çökme düzeltildi) ------------------
static void ShowConnectDialog(HWND parent) {
    EnsureDialogClass();
    auto instances = vsint::Enumerate();
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"M2GOD_CODEDLG",
        L"Visual Studio'ya Bağlan", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        320, 220, 620, 380, parent, nullptr, g_hInst, nullptr);
    SetWindowLongW(dlg, GWL_STYLE, (GetWindowLongW(dlg, GWL_STYLE) & ~WS_MAXIMIZEBOX) | WS_CAPTION);
    HWND hint = CreateWindowExW(0, L"STATIC",
        L"Çalışan Visual Studio örnekleri (devenv.exe):", WS_CHILD | WS_VISIBLE,
        10, 8, 580, 20, dlg, nullptr, g_hInst, nullptr);
    SendMessageW(hint, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    HWND lb = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        10, 32, 588, 210, dlg, (HMENU)4001, g_hInst, nullptr);
    SendMessageW(lb, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    HWND btnConn = CreateWindowExW(0, L"BUTTON", L"Bağlan",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 384, 296, 100, 30, dlg, (HMENU)4002, g_hInst, nullptr);
    HWND btnRefresh = CreateWindowExW(0, L"BUTTON", L"Yenile",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 274, 296, 100, 30, dlg, (HMENU)4004, g_hInst, nullptr);
    HWND btnClose = CreateWindowExW(0, L"BUTTON", L"Kapat",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 494, 296, 100, 30, dlg, (HMENU)4003, g_hInst, nullptr);
    SendMessageW(btnConn, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(btnRefresh, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(btnClose, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    // ctx HEAP'te — diyalog kapandığında silinir
    struct Ctx { std::vector<vsint::VSInstance> insts; HWND lb; }* ctx = new Ctx{ std::move(instances), lb };
    int k = 0;
    for (auto& i : ctx->insts) {
        SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)InstLine(i).c_str());
        k++;
    }
    if (k == 0) {
        SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"Çalışan Visual Studio örneği bulunamadı (devenv.exe).");
        SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"");
        SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"• Visual Studio 2019'u açıp bir çözüm yükleyin;");
        SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"• bu tasarımıcıyı VS ile AYNI ayrıcalık seviyesinde açın");
        SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"  (VS normal ise tasarımıcıyı da normal açın), sonra Yenile.");
    }
    SetWindowLongPtrW(dlg, GWLP_USERDATA, (LONG_PTR)ctx);
    SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)(+[](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        Ctx* ctx = (Ctx*)GetWindowLongPtrW(h, GWLP_USERDATA);
        switch (m) {
        case WM_COMMAND: {
            int id = LOWORD(w);
            if (id == 4003) { DestroyWindow(h); return 0; }
            if (id == 4004) {   // Yenile
                if (ctx) {
                    ctx->insts = vsint::Enumerate();
                    SendMessageW(ctx->lb, LB_RESETCONTENT, 0, 0);
                    for (auto& i : ctx->insts)
                        SendMessageW(ctx->lb, LB_ADDSTRING, 0, (LPARAM)InstLine(i).c_str());
                }
                return 0;
            }
            if (id == 4002 && ctx) {
                int sel = (int)SendMessageW(ctx->lb, LB_GETCURSEL, 0, 0);
                if (sel < 0 || sel >= (int)ctx->insts.size()) {
                    MessageBoxW(h, L"Lütfen listeden bir Visual Studio örneği seçin.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                vsint::VSInstance inst = ctx->insts[sel];   // kopya (dte ref+1; Dikkat: orijinalin dte'sini sıfırla)
                ctx->insts[sel].dte = nullptr;              // çift Release yok
                if (DoConnectInstance(h, inst))
                    DestroyWindow(h);
            }
            return 0;
        }
        case WM_SIZE: {
            RECT rc; GetClientRect(h, &rc);
            MoveWindow(GetDlgItem(h, 4001), 10, 32, rc.right - 20, rc.bottom - 112, TRUE);
            MoveWindow(GetDlgItem(h, 4002), rc.right - 326, rc.bottom - 52, 100, 30, TRUE);
            MoveWindow(GetDlgItem(h, 4004), rc.right - 216, rc.bottom - 52, 100, 30, TRUE);
            MoveWindow(GetDlgItem(h, 4003), rc.right - 110, rc.bottom - 52, 100, 30, TRUE);
            return 0;
        }
        case WM_DESTROY: {
            if (ctx) {
                for (auto& i : ctx->insts) if (i.dte) { i.dte->Release(); i.dte = nullptr; }
                delete ctx;
                ctx = nullptr;
            }
            return 0;
        }
        }
        return DefWindowProcW(h, m, w, l);
    }));
    RunModal(dlg, parent);
}

// --- giriş ekranı: ÖNCE VS seçimi, sonra sıfır form -------------------------------
// Sonuç: 1 = VS'ye bağlanıldı (sıfır form), 2 = .m2god açıldı, 0 = VS'siz boş form
static std::wstring g_welcomeOpenPath;
static int g_welcomeResult = 0;

static int ShowWelcomeDialog(std::wstring& openPath) {
    EnsureDialogClass();
    int result = 0;
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"M2GOD_CODEDLG",
        L"M2GodWin32 Faz 2.4 — Başlangıç", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        360, 200, 640, 420, nullptr, nullptr, g_hInst, nullptr);
    SetWindowLongW(dlg, GWL_STYLE, (GetWindowLongW(dlg, GWL_STYLE) & ~WS_MAXIMIZEBOX) | WS_CAPTION);
    HWND hint = CreateWindowExW(0, L"STATIC",
        L"1) Çalışan Visual Studio seçin ve bağlanın — vcxproj listeden seçilir:",
        WS_CHILD | WS_VISIBLE, 12, 10, 600, 20, dlg, nullptr, g_hInst, nullptr);
    SendMessageW(hint, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    HWND lb = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        12, 34, 604, 150, dlg, (HMENU)5001, g_hInst, nullptr);
    SendMessageW(lb, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    HWND btnRefresh = CreateWindowExW(0, L"BUTTON", L"Yenile",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 12, 192, 96, 28, dlg, (HMENU)5002, g_hInst, nullptr);
    HWND btnConnect = CreateWindowExW(0, L"BUTTON", L"VS'ye Bağlan",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 116, 192, 240, 28, dlg, (HMENU)5003, g_hInst, nullptr);
    HWND hint2 = CreateWindowExW(0, L"STATIC",
        L"Bağlanınca: çözümdeki .vcxproj'lar listelenir → proje seçilir → varsa kayıtlı tasarım\nyüklenir (veya yeni form açılır: öncekini koru / sil). Tasarım proje klasörüne kaydedilir.",
        WS_CHILD | WS_VISIBLE, 116, 224, 500, 34, dlg, nullptr, g_hInst, nullptr);
    SendMessageW(hint2, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    HWND sep = CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, 12, 234, 604, 4, dlg, nullptr, g_hInst, nullptr);
    (void)sep;
    HWND btnOpen = CreateWindowExW(0, L"BUTTON", L"Önceki tasarımı aç (.m2god / .cpp) — eski menü yüklenir",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 12, 266, 480, 28, dlg, (HMENU)5004, g_hInst, nullptr);
    HWND btnEmpty = CreateWindowExW(0, L"BUTTON", L"VS'siz boş formla devam et",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 12, 302, 480, 28, dlg, (HMENU)5005, g_hInst, nullptr);
    SendMessageW(btnRefresh, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(btnConnect, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(btnOpen, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(btnEmpty, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    // ctx: heap — örnek listesi + sonuç
    struct Ctx { std::vector<vsint::VSInstance> insts; int result = 0; std::wstring path; }* ctx = new Ctx();
    auto fillList = [&](Ctx* c) {
        SendMessageW(lb, LB_RESETCONTENT, 0, 0);
        for (auto& i : c->insts)
            SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)InstLine(i).c_str());
        if (c->insts.empty()) {
            SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"(Çalışan Visual Studio bulunamadı — devenv.exe)");
            SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"Aşağıdaki seçeneklerle devam edebilirsiniz.");
        }
    };
    ctx->insts = vsint::Enumerate();
    fillList(ctx);
    SetWindowLongPtrW(dlg, GWLP_USERDATA, (LONG_PTR)ctx);
    SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)(+[](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        Ctx* ctx = (Ctx*)GetWindowLongPtrW(h, GWLP_USERDATA);
        switch (m) {
        case WM_COMMAND: {
            int id = LOWORD(w);
            if (id == 5002 && ctx) {   // Yenile
                for (auto& i : ctx->insts) if (i.dte) { i.dte->Release(); i.dte = nullptr; }
                ctx->insts = vsint::Enumerate();
                // listeyi yeniden doldur
                HWND lb = GetDlgItem(h, 5001);
                SendMessageW(lb, LB_RESETCONTENT, 0, 0);
                for (auto& i : ctx->insts)
                    SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)InstLine(i).c_str());
                if (ctx->insts.empty()) {
                    SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"(Çalışan Visual Studio bulunamadı — devenv.exe)");
                    SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)L"Aşağıdaki seçeneklerle devam edebilirsiniz.");
                }
                return 0;
            }
            if (id == 5003 && ctx) {   // VS'ye bağlan
                int sel = (int)SendMessageW(GetDlgItem(h, 5001), LB_GETCURSEL, 0, 0);
                if (sel < 0 || sel >= (int)ctx->insts.size()) {
                    MessageBoxW(h, L"Lütfen listeden bir Visual Studio örneği seçin (yoksa 'VS'siz devam et').",
                        L"Bilgi", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                vsint::VSInstance inst = ctx->insts[sel];
                ctx->insts[sel].dte = nullptr;   // sahiplik kopyaya geçti
                for (auto& i : ctx->insts) if (i.dte) { i.dte->Release(); i.dte = nullptr; }
                if (DoConnectInstance(h, inst)) { ctx->result = 1; DestroyWindow(h); }
                return 0;
            }
            if (id == 5004 && ctx) {   // önceki tasarımı aç
                wchar_t file[MAX_PATH] = L"";
                OPENFILENAMEW ofn = { sizeof(ofn) };
                ofn.hwndOwner = h;
                ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
                ofn.lpstrFilter = L"M2God Tasarım (*.m2god)\0*.m2god\0Üretilmiş Kod (*.cpp)\0*.cpp\0Tüm dosyalar\0*.*\0";
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;
                if (GetOpenFileNameW(&ofn)) { ctx->result = 2; ctx->path = file; DestroyWindow(h); }
                return 0;
            }
            if (id == 5005) { ctx->result = 0; DestroyWindow(h); return 0; }   // boş form
            return 0;
        }
        case WM_CLOSE: { if (ctx) { ctx->result = 0; ctx->path.clear(); } DestroyWindow(h); return 0; }
        case WM_DESTROY: {
            // sonuçları dışarı taşı
            if (ctx) {
                g_welcomeResult = ctx->result;
                g_welcomeOpenPath = ctx->path;
                for (auto& i : ctx->insts) if (i.dte) { i.dte->Release(); i.dte = nullptr; }
                delete ctx;
            }
            return 0;
        }
        }
        return DefWindowProcW(h, m, w, l);
    }));
    RunModal(dlg, nullptr);
    result = g_welcomeResult;
    openPath = g_welcomeOpenPath;
    return result;
}

// --- toolbar -------------------------------------------------------------------
static HWND MakeToolbar(HWND parent) {
    HWND tb = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"",
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_TOP | CCS_NODIVIDER,
        0, 0, 0, 0, parent, (HMENU)IDC_TOOLBAR, g_hInst, nullptr);
    SendMessageW(tb, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(tb, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    struct { int id; const wchar_t* txt; } btns[] = {
        { CMD_NEW, L"Yeni" }, { CMD_OPEN, L"Aç" }, { CMD_SAVE, L"Kaydet" }, { 0, L"" },
        { CMD_UNDO, L"Geri Al" }, { CMD_REDO, L"Yinele" }, { 0, L"" },
        { CMD_GENCODE, L"Kod Üret" }, { CMD_PREVIEW, L"Kod Önizleme" }, { 0, L"" },
        { CMD_VS_CONNECT, L"VS'ye Bağlan" }, { CMD_VS_SYNC, L"Senkron" },
    };
    TBBUTTON tbb[32] = { 0 };
    int n = 0;
    for (auto& b : btns) {
        tbb[n].iBitmap = I_IMAGENONE;
        tbb[n].idCommand = b.id;
        tbb[n].fsState = TBSTATE_ENABLED;
        tbb[n].fsStyle = b.id ? BTNS_AUTOSIZE | BTNS_SHOWTEXT : BTNS_SEP;
        tbb[n].iString = (INT_PTR)b.txt;
        n++;
    }
    SendMessageW(tb, TB_ADDBUTTONSW, n, (LPARAM)tbb);
    SendMessageW(tb, TB_AUTOSIZE, 0, 0);
    RECT tr; SendMessageW(tb, TB_GETITEMRECT, n - 1, (LPARAM)&tr);
    SendMessageW(tb, TB_SETBUTTONSIZE, 0, MAKELPARAM(0, 24));
    // yükseklik
    RECT rc; GetWindowRect(tb, &rc);
    (void)tr; (void)rc;
    return tb;
}

// --- menü ----------------------------------------------------------------------
static HMENU MakeMenu() {
    HMENU m = CreateMenu();
    HMENU file = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, CMD_NEW, L"Yeni Tasarım\tCtrl+N");
    AppendMenuW(file, MF_STRING, CMD_OPEN, L"Aç...\tCtrl+O");
    AppendMenuW(file, MF_STRING, CMD_SAVE, L"Kaydet\tCtrl+S");
    AppendMenuW(file, MF_STRING, CMD_SAVEAS, L"Farklı Kaydet...");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, CMD_GENCODE, L"Kod Üret (Proje Klasörü)...\tF6");
    AppendMenuW(file, MF_STRING, CMD_PREVIEW, L"Kod Önizleme\tF12");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, CMD_EXIT, L"Çıkış");
    HMENU edit = CreatePopupMenu();
    AppendMenuW(edit, MF_STRING, CMD_UNDO, L"Geri Al\tCtrl+Z");
    AppendMenuW(edit, MF_STRING, CMD_REDO, L"Yinele\tCtrl+Y");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, CMD_CUT, L"Kes\tCtrl+X");
    AppendMenuW(edit, MF_STRING, CMD_COPY, L"Kopyala\tCtrl+C");
    AppendMenuW(edit, MF_STRING, CMD_PASTE, L"Yapıştır\tCtrl+V");
    AppendMenuW(edit, MF_STRING, CMD_DELETE, L"Sil\tDel");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, CMD_FRONT, L"Öne Getir\tCtrl+T");
    AppendMenuW(edit, MF_STRING, CMD_BACK, L"Arkaya Gönder\tCtrl+B");
    HMENU view = CreatePopupMenu();
    AppendMenuW(view, MF_STRING | MF_CHECKED, CMD_VIEW_TB, L"Araç Kutusu");
    AppendMenuW(view, MF_STRING | MF_CHECKED, CMD_VIEW_PROPS, L"Özellikler Paneli");
    AppendMenuW(view, MF_STRING | MF_CHECKED, CMD_VIEW_GRID, L"Izgara Yapış");
    AppendMenuW(view, MF_STRING | MF_CHECKED, CMD_VIEW_STATUS, L"Durum Çubuğu");
    HMENU proj = CreatePopupMenu();
    AppendMenuW(proj, MF_STRING, CMD_VS_CONNECT, L"Visual Studio'ya Bağlan...\tCtrl+Shift+V");
    AppendMenuW(proj, MF_STRING, CMD_VS_DISCONNECT, L"Bağlantıyı Kes");
    AppendMenuW(proj, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(proj, MF_STRING, CMD_VS_SYNC, L"Form1.h/.cpp Senkronize Et\tF7");
    AppendMenuW(proj, MF_STRING, CMD_VS_OPENFOLDER, L"Bağlı Proje Klasörünü Aç");
    HMENU help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, CMD_ABOUT, L"Hakkında...");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)file, L"Dosya");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)edit, L"Düzen");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)view, L"Görünüm");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)proj, L"Proje");
    AppendMenuW(m, MF_POPUP, (UINT_PTR)help, L"Yardım");
    return m;
}

// ===PART3===

// --- dosya diyalogları -----------------------------------------------------------
static bool AskSave(HWND parent, std::wstring& path) {
    wchar_t file[MAX_PATH] = L"";
    wcsncpy(file, path.empty() ? L"Tasarim.m2god" : GetFileName(path).c_str(), MAX_PATH - 1);
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = parent;
    ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"M2God Tasarım (*.m2god)\0*.m2god\0Tüm dosyalar\0*.*\0";
    ofn.lpstrDefExt = L"m2god";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    if (!GetSaveFileNameW(&ofn)) return false;
    path = file;
    return true;
}
static bool AskOpen(HWND parent, std::wstring& path) {
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = parent;
    ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"M2God Tasarım (*.m2god)\0*.m2god\0Üretilmiş Kod (*.cpp)\0*.cpp\0Tüm dosyalar\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) return false;
    path = file;
    return true;
}

// --- komutlar ---------------------------------------------------------------------
static void DoNew() {
    g_model.PushUndo();
    g_model.ClearAll();
    g_model.formProps = { { L"Text", L"Form1" }, { L"Name", L"Form1" }, { L"BackColor", L"Control" } };
    g_model.formW = 800; g_model.formH = 600;
    s_file.clear();
    surface::RecreateAll(s_surface);
    props::Refresh(s_props);
    InvalidateRect(s_surface, nullptr, TRUE);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
    Status(L"Yeni tasarım");
}
static void DoLoadFile(HWND h, const std::wstring& path) {
    if (Lower(path).rfind(L".cpp") != std::wstring::npos) {
        if (codegen::ImportLegacyGenerated(path)) {
            surface::RecreateAll(s_surface);
            props::Refresh(s_props);
            InvalidateRect(s_surface, nullptr, TRUE);
            PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
            Status(L"Faz1.4 kodu içe aktarıldı");
        } else MessageBoxW(h, L"CreateWindow satırları ayrıştırılamadı.", L"M2GodWin32", MB_ICONWARNING);
        return;
    }
    if (g_model.Load(path)) {
        s_file = path;
        surface::RecreateAll(s_surface);
        props::Refresh(s_props);
        InvalidateRect(s_surface, nullptr, TRUE);
        PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
        Status(L"Yüklendi: " + path);
    } else MessageBoxW(h, (L"Dosya okunamadı: " + path).c_str(), L"M2GodWin32", MB_ICONERROR);
}
static void DoSave(HWND h, bool saveAs) {
    if (s_file.empty() || saveAs) {
        if (!AskSave(h, s_file)) return;
    }
    if (g_model.Save(s_file)) { g_modelDirty = false; Status(L"Kaydedildi: " + s_file); }
    else MessageBoxW(h, L"Kaydedilemedi.", L"M2GodWin32", MB_ICONERROR);
}
static void DoGenerate(HWND h) {
    // klasör seç — basitçe .cpp kaydet diyaloğu ile klasör+ad al
    wchar_t file[MAX_PATH] = L"Form1.cpp";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = h;
    ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"C++ Kaynak\0*.cpp\0";
    ofn.lpstrDefExt = L"cpp";
    ofn.lpstrTitle = L"Kod üretilecek klasörü seçin (Form1.cpp olarak kaydedilir)";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    if (!GetSaveFileNameW(&ofn)) return;
    std::wstring dir = GetFileDir(file);
    if (codegen::EmitProject(dir)) {
        Status(L"Form1.h / Form1.cpp üretildi → " + dir);
        MessageBoxW(h, (L"Üretim tamam:\n" + dir + L"\\Form1.cpp\n" + dir + L"\\Form1.h\n\n"
            L"Bunları VS2019 projesine ekleyip derleyebilirsiniz.").c_str(), L"Kod Üretimi", MB_OK | MB_ICONINFORMATION);
    } else MessageBoxW(h, L"Kod yazılamadı.", L"M2GodWin32", MB_ICONERROR);
}

static void DoEventDefault(Ctrl* c, const std::wstring& evName) {
    // varsayılan olay işleyicisini bağla + VS varsa oraya git
    if (c) {
        std::wstring hname = g_model.HandlerNameOf(c, evName);
        if (!c->events.count(evName)) c->events[evName] = hname;
        props::Refresh(s_props);
    } else {
        std::wstring hname = L"Form1_" + evName;
        if (!g_model.formEvents.count(evName)) g_model.formEvents[evName] = hname;
        props::Refresh(s_props);
    }
    if (vsint::IsConnected()) {
        vsint::EnsureHandlerAndGoto(c, evName);
    } else {
        // yerel üretim: kod önizlemede görünür
        std::wstring who = c ? (c->name + L"_" + evName) : (L"Form1_" + evName);
        Status(L"Olay işleyicisi hazır: " + who + L" (F12 ile kodu gör, Ctrl+Shift+V ile VS'ye bağlan)");
    }
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
}

static void DoLayout(HWND h) {
    RECT rc; GetClientRect(h, &rc);
    int w = rc.right, hh = rc.bottom;
    int tbH = 0;
    if (s_toolbar) {
        RECT tr; GetWindowRect(s_toolbar, &tr);
        tbH = tr.bottom - tr.top;
    }
    int top = tbH;
    int bottom = s_showStatus ? STATUS_H : 0;
    int cx = 0;
    if (s_showToolbox) { MoveWindow(s_toolbox, 0, top, TOOLBOX_W, hh - top - bottom, TRUE); cx = TOOLBOX_W; }
    int rightW = 0;
    if (s_showProps) { MoveWindow(s_props, w - PROPS_W, top, PROPS_W, hh - top - bottom, TRUE); rightW = PROPS_W; }
    MoveWindow(s_surface, cx, top, w - cx - rightW, hh - top - bottom, TRUE);
    if (s_status) MoveWindow(s_status, 0, hh - bottom, w, bottom, TRUE);
}

// --- demo tasarım -----------------------------------------------------------------
static void LoadDemo() {
    g_model.formProps = { { L"Text", L"M2GodWin32 — God Mode MVP" }, { L"Name", L"Form1" }, { L"BackColor", L"Control" } };
    g_model.formW = 760; g_model.formH = 520;
    struct Add { const wchar_t* type; int x, y, w, h; const wchar_t* text; };
    Add adds[] = {
        { L"Label",      24, 20, 320, 26, L"Win32 API Görsel Tasarım Ortamı" },
        { L"TextBox",    24, 60, 220, 26, L"Merhaba M2God" },
        { L"Button",     260, 58, 120, 34, L"Tıkla" },
        { L"CheckBox",   24, 104, 180, 26, L"Otomatik senkron" },
        { L"RadioButton",24, 138, 150, 26, L"Win32 API modu" },
        { L"ComboBox",   24, 176, 160, 28, L"" },
        { L"GroupBox",   210, 108, 220, 110, L"Ayarlar" },
        { L"ProgressBar",210, 232, 220, 26, L"" },
        { L"ListBox",    24, 224, 160, 120, L"" },
        { L"ListView",   460, 60, 250, 130, L"" },
        { L"TreeView",   460, 208, 250, 140, L"" },
        { L"Button",     460, 364, 120, 34, L"Çıkış" },
    };
    for (auto& a : adds) {
        Ctrl* c = g_model.Add(a.type, a.x, a.y);
        if (c) {
            c->w = a.w; c->h = a.h;
            c->props[L"Text"] = a.text;
            wchar_t b1[64], b2[64];
            swprintf(b1, 64, L"%d, %d", a.x, a.y);
            swprintf(b2, 64, L"%d, %d", a.w, a.h);
            c->props[L"Location"] = b1; c->props[L"Size"] = b2;
        }
    }
    // örnek olay bağları
    for (auto* c : g_model.ctrls) {
        if (c->type == L"Button" && c->Get(L"Text") == L"Tıkla")
            c->events[L"Click"] = L"button1_Click";
        if (c->type == L"TextBox")
            c->events[L"TextChanged"] = L"textBox1_TextChanged";
    }
    g_model.formEvents[L"Load"] = L"Form1_Load";
    g_model.formEvents[L"Paint"] = L"Form1_Paint";
    // bileşen
    Ctrl* t = g_model.Add(L"Timer", 0, 0);
    if (t) { t->props[L"Interval"] = L"1000"; t->props[L"Enabled"] = L"True"; }
    // seçim: ilk buton
    for (auto* c : g_model.ctrls)
        if (c->type == L"Button" && c->Get(L"Text") == L"Tıkla") { g_model.selected = c; g_model.formSelected = false; break; }
    g_model.ClearUndo();
}

// ===PART4===

// --- ana çerçeve prosedürü ---------------------------------------------------------
static LRESULT CALLBACK FrameProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES };
        InitCommonControlsEx(&icc);
        SetMenu(h, MakeMenu());
        s_toolbar = MakeToolbar(h);
        s_toolbox = toolbox::Create(h);
        s_props = props::Create(h);
        s_surface = surface::Create(h);
        s_status = CreateWindowExW(0, STATUSCLASSNAMEW, L"Hazır",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, h, (HMENU)IDC_STATUSBAR, g_hInst, nullptr);
        SendMessageW(s_status, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
        SetStatusBarHwnd(s_status);
        return 0;
    }
    case WM_SIZE:
        DoLayout(h);
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(w);
        switch (id) {
        case CMD_NEW: DoNew(); return 0;
        case CMD_OPEN: { std::wstring p; if (AskOpen(h, p)) DoLoadFile(h, p); } return 0;
        case CMD_SAVE: DoSave(h, false); return 0;
        case CMD_SAVEAS: DoSave(h, true); return 0;
        case CMD_GENCODE: DoGenerate(h); return 0;
        case CMD_PREVIEW: props::ApplyPending(s_props); ShowCodePreview(h); return 0;
        case CMD_EXIT:
            if (g_modelDirty && !s_file.empty()) { g_model.Save(s_file); g_modelDirty = false; }
            PostQuitMessage(0);
            return 0;
        case CMD_UNDO: g_model.Undo(); surface::RecreateAll(s_surface); props::Refresh(s_props); InvalidateRect(s_surface, nullptr, TRUE); return 0;
        case CMD_REDO: g_model.Redo(); surface::RecreateAll(s_surface); props::Refresh(s_props); InvalidateRect(s_surface, nullptr, TRUE); return 0;
        case CMD_CUT:
            if (g_model.selected) { delete s_clipboard; s_clipboard = new Ctrl(*g_model.selected); s_clipboard->hwnd = nullptr; g_model.Delete(g_model.selected); surface::RecreateAll(s_surface); props::Refresh(s_props); InvalidateRect(s_surface, nullptr, TRUE); PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); }
            return 0;
        case CMD_COPY:
            if (g_model.selected) { delete s_clipboard; s_clipboard = new Ctrl(*g_model.selected); s_clipboard->hwnd = nullptr; Status(L"Kopyalandı: " + s_clipboard->name); }
            return 0;
        case CMD_PASTE:
            if (s_clipboard) {
                g_model.PushUndo();
                Ctrl* c = g_model.Clone(s_clipboard, GRID_SIZE * 2, GRID_SIZE * 2);
                if (c) {
                    factory::CreateOnSurface(s_surface, c);
                    g_model.selected = c; g_model.formSelected = false;
                    props::Refresh(s_props); InvalidateRect(s_surface, nullptr, FALSE);
                    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
                    Status(L"Yapıştırıldı: " + c->name);
                }
            }
            return 0;
        case CMD_DELETE:
            // odak bir metin düzenleyicideyse silme (öz. panelinde yazarken yanlışlıkla)
            {
                HWND foc = GetFocus();
                wchar_t cls[32] = L"";
                if (foc) GetClassNameW(foc, cls, 32);
                if (!wcscmp(cls, L"EDIT") || !wcscmp(cls, L"COMBOBOX") || !wcscmp(cls, L"RichEdit20W"))
                    return 0;
            }
            if (g_model.selected) {
                g_model.PushUndo();
                g_model.Delete(g_model.selected);
                surface::RecreateAll(s_surface);
                props::Refresh(s_props);
                InvalidateRect(s_surface, nullptr, TRUE);
                PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
                Status(L"Silindi");
            }
            return 0;
        case CMD_FRONT:
            if (g_model.selected) { g_model.PushUndo(); g_model.BringToFront(g_model.selected); surface::RecreateAll(s_surface); InvalidateRect(s_surface, nullptr, TRUE); }
            return 0;
        case CMD_BACK:
            if (g_model.selected) { g_model.PushUndo(); g_model.SendToBack(g_model.selected); surface::RecreateAll(s_surface); InvalidateRect(s_surface, nullptr, TRUE); }
            return 0;
        case CMD_VIEW_TB: s_showToolbox = !s_showToolbox; ShowWindow(s_toolbox, s_showToolbox); DoLayout(h); return 0;
        case CMD_VIEW_PROPS: s_showProps = !s_showProps; ShowWindow(s_props, s_showProps); DoLayout(h); return 0;
        case CMD_VIEW_STATUS: s_showStatus = !s_showStatus; ShowWindow(s_status, s_showStatus); DoLayout(h); return 0;
        case CMD_VIEW_GRID: s_showGrid = !s_showGrid; g_gridVisible = s_showGrid; InvalidateRect(s_surface, nullptr, TRUE); return 0;
        case CMD_VS_CONNECT: ShowConnectDialog(h); return 0;
        case CMD_ESC_CANCEL:
            if (!toolbox::ArmedType().empty()) {
                toolbox::Disarm();
                Status(L"Yerleştirme iptal edildi (ESC)");
                InvalidateRect(s_surface, nullptr, FALSE);
            }
            return 0;
        case CMD_VS_DISCONNECT: vsint::Disconnect(); return 0;
        case CMD_VS_SYNC: props::ApplyPending(s_props); vsint::SyncToProject(nullptr, L"", nullptr); return 0;
        case CMD_VS_OPENFOLDER:
            if (vsint::IsConnected()) ShellExecuteW(h, L"open", vsint::Current().projectPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        case CMD_ABOUT:
            MessageBoxW(h,
                L"M2GodWin32 — Faz 2.4 God Mode MVP\n\n"
                L"Win32 API Görsel Form Tasarımcısı\n"
                L"• Config_info.json kataloğu ile tüm .NET elemanları\n"
                L"• Sürükle-bırak VE tıkla-yerleştir (ESC = iptal)\n"
                L"• Overlay sürükleme: hayalet görsel + bırakma hedefi vurgusu\n"
                L"• Konteyner desteği: Panel/GroupBox/TabControl içine bırakma\n"
                L"• PID → çözüm → .vcxproj LİSTESİ (dosya konumu elle seçilmez)\n"
                L"• Tasarım proje klasörüne kaydedilir, bağlanınca algılanıp yüklenir\n"
                L"  (yeni form: öncekini koru / sil seçenekleri)\n"
                L"• Otomatik kayıt + Form1.h/.cpp + Global.h marker'lı üretim + VS2019\n"
                L"• Renk/Yazı tipi/Boyut/enum düzenleyicileri (ChooseColor/ChooseFont)\n\n"
                L"Faz 2.4 yenilikleri:\n"
                L"• Global.h — WinForms benzeri nesne sözdizimi:\n"
                L"  timer1->Enabled = false;  button1->Text = L\"Merhaba\";\n"
                L"  comboBox1->Items->Add(L\"...\");  checkBox1->Checked = true;\n"
                L"• Timer GERÇEK çalışır (SetTimer + WM_TIMER dispatch)\n"
                L"• Renk/Font üretilen kodda uygulanır (WM_CTLCOLOR*)\n"
                L"• Özellik paneli: tek tıkla combobox hemen açılır\n\n"
                L"Visual Studio 2019 C++ Win32 API projesi",
                L"Hakkında", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        return 0;
    }
    case MWM_SEL_CHANGED:
        props::Refresh(s_props);
        return 0;
    case MWM_MODEL_CHANGED:
        g_modelDirty = true;
        {
            std::wstring t = L"M2GodWin32 Faz 2.4";
            if (!s_file.empty()) t += L" — " + GetFileName(s_file) + (g_modelDirty ? L" *" : L"");
            SetWindowTextW(h, t.c_str());
        }
        // otomatik kayıt: bağlı proje klasöründeki .m2god (1.2 sn debounce)
        if (!s_file.empty()) SetTimer(h, IDT_AUTOSAVE, 1200, nullptr);
        return 0;
    case WM_TIMER:
        if (w == IDT_AUTOSAVE) {
            KillTimer(h, IDT_AUTOSAVE);
            if (g_modelDirty && !s_file.empty()) {
                if (g_model.Save(s_file)) {
                    g_modelDirty = false;
                    SYSTEMTIME st; GetLocalTime(&st);
                    wchar_t tb[64]; swprintf(tb, 64, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
                    Status(std::wstring(L"Otomatik kaydedildi: ") + GetFileName(s_file) + L"  " + tb);
                    std::wstring t = L"M2GodWin32 Faz 2.4 — " + GetFileName(s_file);
                    SetWindowTextW(h, t.c_str());
                }
            }
        }
        return 0;
    case WM_CLOSE:
        // çıkışta kaydedilmemiş tasarımı otomatik kaydet (bağlı proje klasörüne)
        if (g_modelDirty && !s_file.empty()) {
            g_model.Save(s_file);
            g_modelDirty = false;
        }
        DestroyWindow(h);
        return 0;
    case MWM_PROPS_APPLIED:
        // SÜRÜKLEME SIRASINDA: tüm liste yeniden kurulmaz (flicker yok) —
        // sadece Location/Size satır değerleri güncellenir
        props::UpdateLiveGeometry();
        return 0;
    case MWM_EVENT_DBLCLICK: {
        Ctrl* c = (Ctrl*)w;
        wchar_t* ev = (wchar_t*)l;
        if (ev) {
            DoEventDefault(c, ev);
            free(ev);
        }
        return 0;
    }
    case MWM_STATUS: {
        wchar_t* t = (wchar_t*)l;
        if (t) { Status(t); free(t); }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

// --- hızlandırıcılar -----------------------------------------------------------------
static HACCEL MakeAccel() {
    ACCEL a[] = {
        { FCONTROL | FVIRTKEY, 'N', CMD_NEW },
        { FCONTROL | FVIRTKEY, 'O', CMD_OPEN },
        { FCONTROL | FVIRTKEY, 'S', CMD_SAVE },
        { FCONTROL | FVIRTKEY, 'Z', CMD_UNDO },
        { FCONTROL | FVIRTKEY, 'Y', CMD_REDO },
        { FCONTROL | FVIRTKEY, 'X', CMD_CUT },
        { FCONTROL | FVIRTKEY, 'C', CMD_COPY },
        { FCONTROL | FVIRTKEY, 'V', CMD_PASTE },
        { FVIRTKEY, VK_DELETE, CMD_DELETE },
        { FVIRTKEY, VK_F6, CMD_GENCODE },
        { FVIRTKEY, VK_F12, CMD_PREVIEW },
        { FVIRTKEY, VK_F7, CMD_VS_SYNC },
        { FCONTROL | FSHIFT | FVIRTKEY, 'V', CMD_VS_CONNECT },
        { FCONTROL | FVIRTKEY, 'T', CMD_FRONT },
        { FCONTROL | FVIRTKEY, 'B', CMD_BACK },
        { FVIRTKEY, VK_ESCAPE, CMD_ESC_CANCEL },   // tıkla-yerleştir iptali her odakta çalışsın
    };
    return CreateAcceleratorTableW(a, 16);
}

namespace mainwnd {

int Run(HINSTANCE hInst, LPWSTR cmdLine) {
    g_hInst = hInst;
    // exe klasörü
    std::wstring exe = ExePath();
    g_exeDir = GetFileDir(exe) + L"\\";
    // fontlar
    g_fontUI = MakeFont(L"Segoe UI", 9, false, false, false);
    g_fontUIBold = MakeFont(L"Segoe UI", 9, true, false, false);
    g_fontMono = MakeFont(L"Consolas", 9, false, false, false);
    // COM (DTE)
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // özel sınıflar
    factory::RegisterCustomClasses(hInst);
    // katalog: exe yanındaki Config_info.json (yoksa yerleşik)
    std::wstring cfg = g_exeDir + L"Config_info.json";
    if (!FileExists(cfg)) cfg = L"C:\\M2GodWin32\\Config_info.json";
    bool fromConfig = g_catalog.Load(cfg);
    // pencere
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = FrameProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"M2GOD_MAIN";
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    RegisterClassW(&wc);
    HWND h = CreateWindowExW(0, L"M2GOD_MAIN", L"M2GodWin32 Faz 2.4 — God Mode MVP",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        8, 8, 1424, 856, nullptr, nullptr, hInst, nullptr);
    g_hMain = h;
    // katalog → toolbox
    toolbox::Refresh(s_toolbox);
    // --- GİRİŞ EKRANI: önce VS seçimi, sonra sıfır form -------------------------
    // (--demo verilirse eski demo tasarım yüklenir — geliştirici modu)
    bool demoMode = (cmdLine && wcsstr(cmdLine, L"--demo") != nullptr);
    std::wstring openPath;
    if (!demoMode) {
        int wr = ShowWelcomeDialog(openPath);
        if (wr == 2 && !openPath.empty()) {
            // eski tasarım yüklenecek (menü/form birlikte)
            s_file = Lower(openPath).rfind(L".cpp") != std::wstring::npos ? L"" : openPath;
            pendingOpen = openPath;
        }
    } else {
        LoadDemo();
    }
    surface::RecreateAll(s_surface);
    if (!pendingOpen.empty()) {
        DoLoadFile(h, pendingOpen);   // seçilen tasarım ana pencerede açılır
        pendingOpen.clear();
    }
    props::Refresh(s_props);
    PostMessageW(h, MWM_MODEL_CHANGED, 0, 0);
    Status(fromConfig
        ? (L"Katalog: Config_info.json (" + std::to_wstring(g_catalog.ElementCount()) + L" eleman) — hazır")
        : L"Katalog: yerleşik (Config_info.json bulunamadı) — hazır");

    HACCEL acc = MakeAccel();
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        // odak bir metin düzenleyicideyse Ctrl+C/X/V ve Delete hızlandırıcılarını YUTMA —
        // edit kontrolü kendi davranışını yürütsün (kopyala/yapıştır bozulmasın)
        bool skipAccel = false;
        if (msg.message == WM_KEYDOWN) {
            HWND foc = GetFocus();
            wchar_t cls[32] = L"";
            if (foc) GetClassNameW(foc, cls, 32);
            if (!wcscmp(cls, L"EDIT") || !wcscmp(cls, L"COMBOBOX") || !wcscmp(cls, L"RichEdit20W")) {
                UINT vk = (UINT)msg.wParam;
                bool ctrlKey = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                if (vk == VK_DELETE || vk == VK_BACK || (ctrlKey && (vk == 'C' || vk == 'X' || vk == 'V' || vk == 'Z' || vk == 'Y')))
                    skipAccel = true;
                // Özellik panelinde satır içi düzenleyici açıksa ESC düzenleyiciye gitsin (vazgeç)
                if (vk == VK_ESCAPE && props::IsEditing()) skipAccel = true;
            }
        }
        if (!skipAccel && TranslateAcceleratorW(h, acc, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    DestroyAcceleratorTable(acc);
    CoUninitialize();
    return (int)msg.wParam;
}

} // namespace mainwnd

