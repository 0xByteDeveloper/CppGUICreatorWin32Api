// harness.cpp — SyncIntoFile birim testi: kullanıcının BOZUK Form1.cpp'sini düzeltmeyi doğrular
#include "app.h"
#include "model.h"
#include "catalog.h"
#include "codegen.h"
#include <windows.h>

// app.h global tanımları + Status() utils.cpp içinde tanımlı (burada tekrar tanımlanmaz)

// surface.cpp bağlanmadan: factory'nin çağırdığı iki fonksiyonun stub'u
namespace surface {
void SubclassForDesign(HWND h) { (void)h; }
void PositionOne(HWND surf, Ctrl* c) { (void)surf; (void)c; }
}

// Kullanıcının rapor ettiği BOZUK dosya (C2040 + boş WM_COMMAND switch)
static const wchar_t* BROKEN_CPP =
L"// Form1.cpp — M2GodWin32 UI Designer (Faz 2.1) tarafindan uretilmistir.\n"
L"#include \"Form1.h\"\n"
L"#include <commctrl.h>\n"
L"#pragma comment(lib, \"Comctl32.lib\")\n"
L"namespace M2GodForm {\n"
L"static HFONT  hFont     = NULL;\n"
L"static void EnsureStatics(HINSTANCE hInst) { }\n"
L"//== M2GOD:HANDLES-BEGIN\n"
L"button1 = NULL;\n"
L"radioButton1 = NULL;\n"
L"groupBox1 = NULL;\n"
L"//== M2GOD:HANDLES-END\n"
L"//== M2GOD:CREATE-BEGIN\n"
L"void InitializeControls(HWND hWnd) {\n"
L"}\n"
L"//== M2GOD:CREATE-END\n"
L"//== M2GOD:EVENTS-BEGIN\n"
L"void button1_Click(HWND hWnd) {\n"
L"    MessageBoxA(0, \"Mause\", \"Test\", 0);\n"
L"    // TODO: button1_Click\n"
L"}\n"
L"//== M2GOD:EVENTS-END\n"
L"} // namespace M2GodForm\n"
L"//== M2GOD:WNDPROC-BEGIN\n"
L"static LRESULT CALLBACK Form1_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {\n"
L"    switch (message) {\n"
L"    case WM_CREATE:\n"
L"        M2GodForm::InitializeControls(hWnd);\n"
L"        return 0;\n"
L"    case WM_COMMAND: {\n"
L"        int id = LOWORD(wParam), ev = HIWORD(wParam);\n"
L"        switch (id) {\n"
L"        }\n"
L"        break;\n"
L"    }\n"
L"    case WM_DESTROY:\n"
L"        PostQuitMessage(0);\n"
L"        return 0;\n"
L"    }\n"
L"    return DefWindowProcW(hWnd, message, wParam, lParam);\n"
L"}\n"
L"//== M2GOD:WNDPROC-END\n";

static const wchar_t* BROKEN_H =
L"// Form1.h\n"
L"#pragma once\n"
L"#include <windows.h>\n"
L"namespace M2GodForm {\n"
L"//== M2GOD:IDS-BEGIN\n"
L"enum {\n"
L"    IDC_BUTTON1 = 3001,\n"
L"    IDC_RADIOBUTTON1 = 3002,\n"
L"    IDC_GROUPBOX1 = 3004\n"
L"};\n"
L"//== M2GOD:IDS-END\n"
L"//== M2GOD:HANDLES-BEGIN\n"
L"extern HWND button1;\n"
L"extern HWND radioButton1;\n"
L"extern HWND groupBox1;\n"
L"//== M2GOD:HANDLES-END\n"
L"//== M2GOD:PROTO-BEGIN\n"
L"void InitializeControls(HWND hWnd);\n"
L"//== M2GOD:PROTO-END\n"
L"//== M2GOD:EVENTDECL-BEGIN\n"
L"void button1_Click(HWND hWnd);\n"
L"//== M2GOD:EVENTDECL-END\n"
L"} // namespace M2GodForm\n";

static void WriteW(const std::wstring& path, const std::wstring& text) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    DWORD wr = 0;
    // UTF-8 olarak yaz (tasarımcı okurken Utf8ToWide kullanıyor)
    std::string u8 = WideToUtf8(text);
    WriteFile(h, u8.data(), (DWORD)u8.size(), &wr, nullptr);
    CloseHandle(h);
}

int main() {
    g_catalog.Load(L""); // yerleşik katalog
    // model: button1 (Click bağlı) + groupBox1 + radioButton1 (groupBox1 içine)
    g_model.ClearAll();
    Ctrl* b = g_model.Add(L"Button", 48, 184);
    if (b) { b->name = L"button1"; b->w = 100; b->h = 32; b->props[L"Text"] = L"Button1"; b->events[L"Click"] = L"button1_Click"; }
    Ctrl* gb = g_model.Add(L"GroupBox", 176, 112);
    if (gb) { gb->name = L"groupBox1"; gb->w = 200; gb->h = 120; gb->props[L"Text"] = L"GroupBox1"; }
    Ctrl* rb = g_model.Add(L"RadioButton", 216, 152);
    if (rb && gb) { rb->name = L"radioButton1"; rb->w = 130; rb->h = 28; rb->props[L"Text"] = L"RadioButton1"; rb->parent = gb; }
    g_model.PushUndo();

    printf("ctrls=%d\n", (int)g_model.ctrls.size()); WriteW(L"C:\\m2god_sync\\Form1.cpp", BROKEN_CPP);
    WriteW(L"C:\\m2god_sync\\Form1.h", BROKEN_H);

    int line = 0;
    int ok = codegen::SyncIntoFile(L"C:\\m2god_sync\\Form1.cpp", L"C:\\m2god_sync\\Form1.h", b, L"Click", &line);
    printf("SyncIntoFile -> %d, handlerLine=%d\n", ok, line);

    // doğrulamalar
    std::string cpp8, h8;
    ReadFileText(L"C:\\m2god_sync\\Form1.cpp", cpp8);
    ReadFileText(L"C:\\m2god_sync\\Form1.h", h8);
    std::wstring cpp = Utf8ToWide(cpp8), h = Utf8ToWide(h8);
    auto has = [&](const std::wstring& s, const wchar_t* what) {
        bool f = s.find(what) != std::wstring::npos;
        printf("[%ls] %ls\n", f ? L"OK  " : L"FAIL", what);
        return f;
    };
    bool all = true;
    all &= has(cpp, L"HWND button1 = NULL;");           // C2040 düzeltmesi
    all &= has(cpp, L"case M2GodForm::IDC_BUTTON1:");   // WM_COMMAND yönlendirme
    all &= has(cpp, L"if (ev == BN_CLICKED) M2GodForm::button1_Click(hWnd);");
    all &= has(cpp, L"MessageBoxA(0, \"Mause\", \"Test\", 0);"); // kullanıcı kodu KORUNDU
    all &= has(cpp, L"SetWindowSubclass(groupBox1, ContainerBridgeProc, 1, 0);"); // köprü
    all &= has(cpp, L"groupBox1, (HMENU)(INT_PTR)IDC_RADIOBUTTON1"); // iç içe yaratım
    all &= has(h, L"extern HWND button1;");
    all &= has(h, L"void button1_Click(HWND hWnd);");
    printf(all ? "\n=== TUM DOGRULAMALAR GECTI ===\n" : "\n=== HATALAR VAR ===\n");
    return all ? 0 : 1;
}
