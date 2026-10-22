// codegen.cpp — kod üretici (Bölüm 1: Form1.h/cpp üretimi)
#include "codegen.h"
#include "app.h"
#include "catalog.h"
#include "factory.h"
#include <sstream>

namespace codegen {

std::wstring IdMacroOf(const std::wstring& ctrlName) {
    std::wstring up = ctrlName;
    for (auto& ch : up) {
        if (iswalnum(ch)) ch = (wchar_t)towupper(ch);
        else ch = L'_';
    }
    return L"IDC_" + up;
}


// kontrol için CreateWindowExW çağrısı bloğu (PARENT-AWARE:
// konteyner çocuğu konteynerin hwnd değişkenine yaratılır, koordinatlar zaten parent-görelisi)
static std::wstring EmitCreateCall(const Ctrl* c) {
    factory::Win32Class m = factory::MapClass(c->type);
    std::wostringstream o;
    if (m.cls.empty()) {
        // bileşen — SetTimer / soyut
        if (c->type == L"Timer") {
            o << L"    // Timer: SetTimer(hWnd, TIMER_" << c->name << L", " << c->Get(L"Interval", L"100") << L", NULL);\n";
        } else {
            o << L"    // " << c->type << L" bileşeni (kod tarafında nesne olarak kullanılır)\n";
        }
        return o.str();
    }
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | m.style;
    std::wstring ex = m.exstyle ? L"WS_EX_CLIENTEDGE" : L"0";
    std::wstring txt = c->Get(L"Text");
    // metin kaçışı
    std::wstring esc = ReplaceAll(txt, L"\\", L"\\\\");
    esc = ReplaceAll(esc, L"\"", L"\\\"");
    std::wstring parentVar = c->parent ? c->parent->name : std::wstring(L"hWnd");
    if (c->parent)
        o << L"    // " << c->name << L" → " << c->parent->name << L" içine (child)\n";
    o << L"    " << c->name << L" = CreateWindowExW(" << ex << L", L\"" << m.cls << L"\", L\"" << esc << L"\",\n";
    std::wstring styleExpr = m.styleText.empty() ? std::wstring(L"0") : m.styleText;
    styleExpr += L" | WS_CHILD | WS_VISIBLE | WS_TABSTOP";
    o << L"        " << styleExpr << L", " << c->x << L", " << c->y << L", " << c->w << L", " << c->h << L",\n";
    o << L"        " << parentVar << L", (HMENU)(INT_PTR)" << IdMacroOf(c->name) << L", hInst, NULL);\n";
    o << L"    SendMessageW(" << c->name << L", WM_SETFONT, (WPARAM)hFont, TRUE);\n";
    // konteyner bildirim köprüsü: konteyner çocuklarının WM_COMMAND/WM_NOTIFY'ı forma taşınır
    if (IsContainerType(c->type))
        o << L"    SetWindowSubclass(" << c->name << L", ContainerBridgeProc, 1, 0);\n";
    // renk / font özellik notları
    if (c->props.count(L"Font") && c->Get(L"Font") != L"Segoe UI, 9pt")
        o << L"    // Font: " << c->Get(L"Font") << L"\n";
    if (c->props.count(L"ForeColor") && c->Get(L"ForeColor") != L"ControlText")
        o << L"    // ForeColor: " << c->Get(L"ForeColor") << L" (WM_CTLCOLORSTATIC/WM_CTLCOLOREDIT içinde uygula)\n";
    if (c->props.count(L"BackColor") && c->Get(L"BackColor") != L"Control")
        o << L"    // BackColor: " << c->Get(L"BackColor") << L" (WM_CTLCOLORSTATIC/WM_CTLCOLOREDIT içinde uygula)\n";
    if (!_wcsicmp(c->Get(L"Enabled", L"True").c_str(), L"false"))
        o << L"    EnableWindow(" << c->name << L", FALSE);\n";
    if (_wcsicmp(c->Get(L"Visible", L"True").c_str(), L"false") == 0)
        o << L"    ShowWindow(" << c->name << L", SW_HIDE);\n";
    return o.str();
}

std::wstring GenerateForm1Cpp() {
    std::wostringstream o;
    o << L"// Form1.cpp — M2GodWin32 UI Designer (Faz 2.3) tarafından üretilmiştir.\n";
    o << L"// Marker blokları (//== M2GOD:... ) tasarımcı senkronizasyonu içindir; dışını düzenleyebilirsiniz.\n\n";
    o << L"#include \"Form1.h\"\n";
    o << L"#include <commctrl.h>\n\n";
    o << L"#pragma comment(lib, \"Comctl32.lib\")\n\n";
    o << L"namespace M2GodForm {\n\n";
    o << L"static HFONT  hFont     = NULL;\n";
    o << L"static HBRUSH hBackBrush= NULL;\n";
    o << L"static HBRUSH hCtrlBrush= NULL;\n\n";
    o << L"static void EnsureStatics(HINSTANCE hInst) {\n";
    o << L"    if (hFont) return;\n";
    o << L"    hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,\n";
    o << L"        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,\n";
    o << L"        DEFAULT_PITCH | FF_DONTCARE, L\"Segoe UI\");\n";
    o << L"    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES };\n";
    o << L"    InitCommonControlsEx(&icc);\n";
    o << L"}\n\n";
    // HANDLES
    o << L"//== M2GOD:HANDLES-BEGIN\n";
    for (auto* c : g_model.ctrls) {
        if (c->nonVisual) continue;
        o << L"HWND " << c->name << L" = NULL;\n";
    }
    o << L"//== M2GOD:HANDLES-END\n\n";
    // CREATE — parent-sıralı: konteyner önce, alt kontroller sonra (aynı parent içinde
    // oluşturma sırası korunur; böylece HWND z-sırası tasarımla birebir aynıdır)
    // NOT: Konteyner bildirim köprüsü CREATE bölgesi İÇİNDE tanımlanır; böylece senkron
    // (SyncIntoFile) eski üretilmiş dosyalara köprüyü de beraberinde taşır.
    o << L"//== M2GOD:CREATE-BEGIN\n";
    o << L"// Konteyner bildirim köprüsü: GroupBox/TabControl/Panel içine yaratılan alt kontrollerin\n";
    o << L"// bildirimleri (WM_COMMAND/WM_NOTIFY) doğrudan konteynere gider; köprü bunları ana forma taşır.\n";
    o << L"static LRESULT CALLBACK ContainerBridgeProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {\n";
    o << L"    if (m == WM_COMMAND || m == WM_NOTIFY) {\n";
    o << L"        HWND p = GetParent(h);\n";
    o << L"        if (p) { SendMessageW(p, m, w, l); return 0; }\n";
    o << L"    }\n";
    o << L"    return DefSubclassProc(h, m, w, l);\n";
    o << L"}\n\n";
    o << L"void InitializeControls(HWND hWnd) {\n";
    o << L"    HINSTANCE hInst = GetModuleHandleW(NULL);\n";
    o << L"    EnsureStatics(hInst);\n";
    std::function<void(Ctrl*)> emitBranch = [&](Ctrl* parent) {
        for (auto* c : g_model.ChildrenOf(parent)) {
            if (c->nonVisual) continue; // bileşenler sonda tek kez üretilir (çift Timer yorumu olmasın)
            o << EmitCreateCall(c) << L"\n";
            emitBranch(c);
        }
    };
    emitBranch(nullptr);
    for (auto* c : g_model.ctrls) if (c->nonVisual) o << EmitCreateCall(c) << L"\n";
    o << L"}\n";
    o << L"//== M2GOD:CREATE-END\n\n";
    // EVENTS — bağlı her olay için TODO stub'ı üret
    o << L"//== M2GOD:EVENTS-BEGIN  (olay işleyicileri; mevcut gövdeler senkronda korunur)\n";
    std::vector<std::wstring> emitted;
    auto emitStub = [&](const std::wstring& hname) {
        for (auto& e : emitted) if (_wcsicmp(e.c_str(), hname.c_str()) == 0) return;
        emitted.push_back(hname);
        o << L"void " << hname << L"(HWND hWnd) {\n";
        o << L"    // TODO: " << hname << L" — kodunuzu buraya yazın\n";
        o << L"}\n\n";
    };
    for (auto* c : g_model.ctrls)
        for (auto& kv : c->events) emitStub(kv.second);
    for (auto& kv : g_model.formEvents) emitStub(kv.second);
    o << L"//== M2GOD:EVENTS-END\n\n";
    o << L"} // namespace M2GodForm\n\n";
    // WNDPROC
    o << L"//== M2GOD:WNDPROC-BEGIN\n";
    o << L"static LRESULT CALLBACK Form1_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {\n";
    o << L"    switch (message) {\n";
    o << L"    case WM_CREATE:\n";
    o << L"        M2GodForm::InitializeControls(hWnd);\n";
    if (g_model.formEvents.count(L"Load"))
        o << L"        M2GodForm::" << g_model.formEvents[L"Load"] << L"(hWnd);\n";
    else
        o << L"        // Form1_Load burada çağrılır (Olaylar sekmesinden ekleyin)\n";
    o << L"        return 0;\n";
    o << L"    case WM_COMMAND: {\n";
    o << L"        int id = LOWORD(wParam), ev = HIWORD(wParam);\n";
    o << L"        switch (id) {\n";
    // router
    for (auto* c : g_model.ctrls) {
        if (c->events.empty()) continue;
        const ElementDef* def = g_catalog.Find(c->type);
        bool routed = false;
        for (auto& kv : c->events) {
            const std::wstring& evName = kv.first;
            std::wstring notify;
            if (evName == L"Click" || evName == L"CheckedChanged") notify = L"BN_CLICKED";
            else if (evName == L"TextChanged") notify = L"EN_CHANGE";
            else if (evName == L"SelectedIndexChanged") {
                if (c->type == L"ComboBox") notify = L"CBN_SELCHANGE";
                else notify = L"LBN_SELCHANGE";
            }
            if (notify.empty()) continue;
            routed = true;
            o << L"        case M2GodForm::" << IdMacroOf(c->name) << L":\n";
            o << L"            if (ev == " << notify << L") M2GodForm::" << kv.second << L"(hWnd);\n";
            o << L"            break;\n";
        }
        (void)def;
        if (routed) o << L"\n";
    }
    o << L"        }\n";
    o << L"        break;\n";
    o << L"    }\n";
    o << L"    case WM_DESTROY:\n";
    o << L"        PostQuitMessage(0);\n";
    o << L"        return 0;\n";
    o << L"    }\n";
    o << L"    return DefWindowProcW(hWnd, message, wParam, lParam);\n";
    o << L"}\n\n";
    o << L"int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {\n";
    o << L"    M2GodForm::EnsureStatics(hInst);\n";
    o << L"    WNDCLASSW wc = { 0 };\n";
    o << L"    wc.lpfnWndProc  = Form1_WndProc;\n";
    o << L"    wc.hInstance    = hInst;\n";
    o << L"    wc.hCursor      = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);\n";
    // form backcolor
    std::wstring fb = g_model.formProps.count(L"BackColor") ? g_model.formProps[L"BackColor"] : L"Control";
    o << L"    wc.hbrBackground = CreateSolidBrush(RGB(";
    COLORREF cr; ParseColor(fb, cr);
    o << GetRValue(cr) << L"," << GetGValue(cr) << L"," << GetBValue(cr) << L"));\n";
    o << L"    wc.lpszClassName = L\"M2GodForm1\";\n";
    o << L"    RegisterClassW(&wc);\n\n";
    int fw = g_model.formW, fh = g_model.formH;
    o << L"    // Form1 boyutu (ClientSize: " << fw << L" × " << fh << L")\n";
    o << L"    RECT rc = { 0, 0, " << fw << L", " << fh << L" };\n";
    o << L"    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);\n";
    o << L"    HWND hWnd = CreateWindowW(L\"M2GodForm1\", L\"";
    o << (g_model.formProps.count(L"Text") ? g_model.formProps[L"Text"] : std::wstring(L"Form1"));
    o << L"\", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,\n";
    o << L"        rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);\n";
    o << L"    if (!hWnd) return 0;\n";
    o << L"    ShowWindow(hWnd, nCmdShow);\n";
    o << L"    UpdateWindow(hWnd);\n";
    o << L"    MSG msg;\n";
    o << L"    while (GetMessageW(&msg, NULL, 0, 0)) {\n";
    o << L"        TranslateMessage(&msg);\n";
    o << L"        DispatchMessageW(&msg);\n";
    o << L"    }\n";
    o << L"    return (int)msg.wParam;\n";
    o << L"}\n";
    o << L"//== M2GOD:WNDPROC-END\n";
    return o.str();
}

std::wstring GenerateForm1H() {
    std::wostringstream o;
    o << L"// Form1.h — M2GodWin32 UI Designer (Faz 2.3) tarafından üretilmiştir.\n";
    o << L"#pragma once\n";
    o << L"#include <windows.h>\n\n";
    o << L"namespace M2GodForm {\n\n";
    o << L"//== M2GOD:IDS-BEGIN\n";
    o << L"enum {\n";
    UINT nextId = 3001;
    bool first = true;
    for (auto* c : g_model.ctrls) {
        if (c->nonVisual) continue;
        if (!first) o << L",\n";
        o << L"    " << IdMacroOf(c->name) << L" = " << (3000 + (UINT)(c->cid - 3000));
        first = false;
        (void)nextId;
    }
    o << L"\n};\n";
    o << L"//== M2GOD:IDS-END\n\n";
    o << L"//== M2GOD:HANDLES-BEGIN\n";
    for (auto* c : g_model.ctrls) {
        if (c->nonVisual) continue;
        o << L"extern HWND " << c->name << L";\n";
    }
    o << L"//== M2GOD:HANDLES-END\n\n";
    o << L"//== M2GOD:PROTO-BEGIN\n";
    o << L"void InitializeControls(HWND hWnd);\n";
    o << L"//== M2GOD:PROTO-END\n\n";
    o << L"//== M2GOD:EVENTDECL-BEGIN\n";
    for (auto* c : g_model.ctrls) {
        for (auto& kv : c->events)
            o << L"void " << kv.second << L"(HWND hWnd);\n";
    }
    for (auto& kv : g_model.formEvents)
        o << L"void " << kv.second << L"(HWND hWnd);\n";
    o << L"//== M2GOD:EVENTDECL-END\n\n";
    o << L"} // namespace M2GodForm\n";
    return o.str();
}

// ===PART2===

// --- bölge değiştirme -----------------------------------------------------------

static bool ReplaceRegion(std::wstring& text, const wchar_t* beginMark, const wchar_t* endMark, const std::wstring& newContent) {
    size_t b = text.find(beginMark);
    if (b == std::wstring::npos) return false;
    size_t e = text.find(endMark, b);
    if (e == std::wstring::npos) return false;
    size_t contentStart = b + wcslen(beginMark);
    text = text.substr(0, contentStart) + L"\n" + newContent + text.substr(e);
    return true;
}

static bool HasRegion(const std::wstring& text, const wchar_t* beginMark) {
    return text.find(beginMark) != std::wstring::npos;
}

static int LineOfPos(const std::wstring& text, size_t pos) {
    int line = 1;
    for (size_t i = 0; i < pos && i < text.size(); i++)
        if (text[i] == L'\n') line++;
    return line;
}

int EnsureHandler(const std::wstring& cppPath, const std::wstring& hPath,
                  const std::wstring& handlerName, const std::wstring& declLine) {
    std::string tmp8;
    if (!ReadFileText(cppPath, tmp8)) return 0;
    std::wstring cpp = Utf8ToWide(tmp8);
    std::wstring sig = L"void " + handlerName + L"(";
    size_t found = Lower(cpp).find(Lower(sig));
    if (found == std::wstring::npos) {
        // stub ekle
        std::wstring stub = L"void " + handlerName + L"(HWND hWnd) {\n    // TODO: " + handlerName + L"\n}\n\n";
        const wchar_t* endMark = L"//== M2GOD:EVENTS-END";
        size_t e = cpp.find(endMark);
        if (e != std::wstring::npos) {
            cpp.insert(e, stub);
        } else {
            cpp += L"\n" + stub;
        }
        WriteFileText(cppPath, WideToUtf8(cpp));
        found = Lower(cpp).find(Lower(sig));
    }
    // bildirimi .h'ye ekle (yoksa)
    if (!declLine.empty() && FileExists(hPath)) {
        std::string t8;
        ReadFileText(hPath, t8);
        std::wstring h = Utf8ToWide(t8);
        std::wstring decl = L"    " + declLine + L"\n";
        size_t dr = Lower(h).find(Lower(L"void " + declLine));
        if (dr == std::wstring::npos) {
            const wchar_t* endMark = L"//== M2GOD:EVENTDECL-END";
            size_t e = h.find(endMark);
            if (e != std::wstring::npos) h.insert(e, decl);
            else h += L"\n" + decl;
            WriteFileText(hPath, WideToUtf8(h));
        }
    }
    if (found == std::wstring::npos) return 0;
    return LineOfPos(cpp, found);
}

int SyncIntoFile(const std::wstring& cppPath, const std::wstring& hPath,
                 Ctrl* targetCtrl, const std::wstring& eventName, int* handlerLine) {
    std::wstring cpp;
    if (!FileExists(cppPath)) {
        cpp = GenerateForm1Cpp();
        WriteFileText(cppPath, WideToUtf8(cpp));
    } else {
        std::string t8;
        if (!ReadFileText(cppPath, t8)) return 0;
        cpp = Utf8ToWide(t8);
    }

    std::wstring h;
    bool hNew = false;
    if (!FileExists(hPath)) {
        h = GenerateForm1H();
        hNew = true;
    } else {
        std::string t8;
        if (!ReadFileText(hPath, t8)) return 0;
        h = Utf8ToWide(t8);
    }

    if (!HasRegion(cpp, L"//== M2GOD:HANDLES-BEGIN") || !HasRegion(cpp, L"//== M2GOD:CREATE-BEGIN")) {
        Status(L"Hata: Form1.cpp içinde M2GOD marker blokları yok");
        return 0;
    }
    // bölge içerikleri
    // NOT: HANDLES bölgesi TİPLİ tanım içermeli (HWND x = NULL;) — tipsiz 'x = NULL;'
    // global alanda geçersizdir (C2040/C4430 derleme hatası)
    std::wostringstream handles;
    for (auto* c : g_model.ctrls) {
        if (c->nonVisual) continue;
        handles << L"HWND " << c->name << L" = NULL;\n";
    }
    ReplaceRegion(cpp, L"//== M2GOD:HANDLES-BEGIN", L"//== M2GOD:HANDLES-END", handles.str());
    // CREATE bölgesi: fonksiyon gövdesini yeniden üret
    std::wstring fresh = GenerateForm1Cpp();
    size_t cb = fresh.find(L"//== M2GOD:CREATE-BEGIN");
    size_t ce = fresh.find(L"//== M2GOD:CREATE-END");
    if (cb != std::wstring::npos && ce != std::wstring::npos) {
        std::wstring createBody = fresh.substr(cb + wcslen(L"//== M2GOD:CREATE-BEGIN"), ce - (cb + wcslen(L"//== M2GOD:CREATE-BEGIN")));
        ReplaceRegion(cpp, L"//== M2GOD:CREATE-BEGIN", L"//== M2GOD:CREATE-END", createBody);
    }
    // WNDPROC bölgesi de tazelenir: yeni bağlanan olaylar WM_COMMAND yönlendirmesine
    // yazılsın (aksi halde üretilen uygulamada tıklamalar tetiklenmez)
    size_t wb = fresh.find(L"//== M2GOD:WNDPROC-BEGIN");
    size_t we = fresh.find(L"//== M2GOD:WNDPROC-END");
    if (wb != std::wstring::npos && we != std::wstring::npos &&
        HasRegion(cpp, L"//== M2GOD:WNDPROC-BEGIN")) {
        std::wstring wndBody = fresh.substr(wb + wcslen(L"//== M2GOD:WNDPROC-BEGIN"), we - (wb + wcslen(L"//== M2GOD:WNDPROC-BEGIN")));
        ReplaceRegion(cpp, L"//== M2GOD:WNDPROC-BEGIN", L"//== M2GOD:WNDPROC-END", wndBody);
    }
    WriteFileText(cppPath, WideToUtf8(cpp));
    // header bölgeleri
    if (!hNew && !HasRegion(h, L"//== M2GOD:IDS-BEGIN")) {
        Status(L"Hata: Form1.h içinde M2GOD marker blokları yok");
        return 0;
    }
    {
        std::wstring freshH = GenerateForm1H();
        auto region = [&](const wchar_t* b, const wchar_t* e, std::wstring& dst) {
            size_t rb = freshH.find(b), re = freshH.find(e);
            if (rb != std::wstring::npos && re != std::wstring::npos)
                ReplaceRegion(dst, b, e, freshH.substr(rb + wcslen(b), re - (rb + wcslen(b))));
        };
        region(L"//== M2GOD:IDS-BEGIN", L"//== M2GOD:IDS-END", h);
        region(L"//== M2GOD:HANDLES-BEGIN", L"//== M2GOD:HANDLES-END", h);
        region(L"//== M2GOD:PROTO-BEGIN", L"//== M2GOD:PROTO-END", h);
        region(L"//== M2GOD:EVENTDECL-BEGIN", L"//== M2GOD:EVENTDECL-END", h);
        WriteFileText(hPath, WideToUtf8(h));
    }
    // hedef handler
    int line = 0;
    if (targetCtrl && !eventName.empty()) {
        std::wstring hname = g_model.HandlerNameOf(targetCtrl, eventName);
        line = EnsureHandler(cppPath, hPath, hname, L"void " + hname + L"(HWND hWnd);");
    }
    if (handlerLine) *handlerLine = line;
    return 1;
}

bool EmitProject(const std::wstring& folder) {
    if (folder.empty()) return false;
    std::wstring cpp = folder + L"\\Form1.cpp";
    std::wstring h = folder + L"\\Form1.h";
    if (!WriteFileText(cpp, WideToUtf8(GenerateForm1Cpp()))) return false;
    if (!WriteFileText(h, WideToUtf8(GenerateForm1H()))) return false;
    return true;
}

// --- Faz1.4 GeneratedForm.cpp içe aktarma -----------------------------------------
bool ImportLegacyGenerated(const std::wstring& cppPath) {
    std::string data;
    if (!ReadFileText(cppPath, data)) return false;
    std::wstring text = Utf8ToWide(data);
    g_model.ClearAll();
    g_model.PushUndo();
    int count = 0;
    size_t pos = 0;
    while (true) {
        size_t cw = text.find(L"CreateWindow", pos);
        if (cw == std::wstring::npos) break;
        size_t lp = text.find(L'(', cw);
        size_t semicolon = text.find(L';', cw);
        if (lp == std::wstring::npos || semicolon == std::wstring::npos) break;
        std::wstring line = text.substr(lp + 1, semicolon - lp);
        // tırnaklı dizeler
        std::vector<std::wstring> strs;
        size_t q = 0;
        while ((q = line.find(L'"', q)) != std::wstring::npos) {
            size_t q2 = line.find(L'"', q + 1);
            if (q2 == std::wstring::npos) break;
            strs.push_back(line.substr(q + 1, q2 - q - 1));
            q = q2 + 1;
        }
        // sayılar (x,y,w,h) — ikinci dizeden sonra, (HMENU) öncesi
        if (strs.size() >= 2) {
            std::wstring cls = strs[0], txt = strs[1];
            std::wstring rest = line.substr(line.find(L'"', line.find(L'"') + 1));
            size_t hm = Lower(rest).find(L"hmenu");
            if (hm == std::wstring::npos) hm = rest.size();
            std::wstring numsPart = rest.substr(0, hm);
            std::vector<int> nums;
            size_t i = 0;
            while (i < numsPart.size() && nums.size() < 4) {
                if (iswdigit(numsPart[i]) || (numsPart[i] == L'-' && i + 1 < numsPart.size() && iswdigit(numsPart[i + 1]))) {
                    int v = (int)wcstol(numsPart.c_str() + i, nullptr, 10);
                    nums.push_back(v);
                    while (i < numsPart.size() && (iswdigit(numsPart[i]) || numsPart[i] == L'-')) i++;
                } else i++;
            }
            if (nums.size() >= 4) {
                // sınıf → tip
                std::wstring type;
                std::wstring low = Lower(line);
                if (_wcsicmp(cls.c_str(), L"BUTTON") == 0) {
                    if (low.find(L"bs_autocheckbox") != std::wstring::npos) type = L"CheckBox";
                    else if (low.find(L"bs_autoradiobutton") != std::wstring::npos) type = L"RadioButton";
                    else if (low.find(L"bs_groupbox") != std::wstring::npos) type = L"GroupBox";
                    else type = L"Button";
                }
                else if (_wcsicmp(cls.c_str(), L"EDIT") == 0) type = L"TextBox";
                else if (_wcsicmp(cls.c_str(), L"STATIC") == 0) type = L"Label";
                else if (_wcsicmp(cls.c_str(), L"COMBOBOX") == 0) type = L"ComboBox";
                else if (_wcsicmp(cls.c_str(), L"LISTBOX") == 0) type = L"ListBox";
                else if (_wcsicmp(cls.c_str(), L"msctls_progress32") == 0) type = L"ProgressBar";
                else if (_wcsicmp(cls.c_str(), L"msctls_trackbar32") == 0) type = L"TrackBar";
                else if (_wcsicmp(cls.c_str(), L"SysListView32") == 0) type = L"ListView";
                else if (_wcsicmp(cls.c_str(), L"SysTreeView32") == 0) type = L"TreeView";
                else if (_wcsicmp(cls.c_str(), L"SysTabControl32") == 0) type = L"TabControl";
                else if (_wcsicmp(cls.c_str(), L"RichEdit20W") == 0) type = L"RichTextBox";
                else type = L"Label";
                Ctrl* c = g_model.Add(type, nums[0], nums[1]);
                if (c) {
                    c->x = nums[0]; c->y = nums[1]; c->w = nums[2]; c->h = nums[3];
                    c->props[L"Text"] = txt;
                    wchar_t b1[64], b2[64];
                    swprintf(b1, 64, L"%d, %d", c->x, c->y);
                    swprintf(b2, 64, L"%d, %d", c->w, c->h);
                    c->props[L"Location"] = b1; c->props[L"Size"] = b2;
                    count++;
                }
            }
        }
        pos = semicolon + 1;
    }
    return count > 0;
}

} // namespace codegen
