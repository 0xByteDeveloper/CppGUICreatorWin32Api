// ============================================================================
//  main.cpp — entry point: COM (STA for DTE), GDI+, DPI, theme, main window
// ============================================================================
#include "UI.h"
#include "Theme.h"

static void EnableDpiAwareness()
{
    typedef BOOL (WINAPI *FnCtx)(HANDLE);
    HMODULE u32 = GetModuleHandleW(L"user32.dll");
    if (u32) {
        FnCtx f = (FnCtx)(void*)GetProcAddress(u32, "SetProcessDpiAwarenessContext");
        if (f) {
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = ((DPI_CONTEXT)-4)
            HANDLE ctx = (HANDLE)-4;
            if (f(ctx)) return;
        }
    }
    typedef BOOL (WINAPI *FnAware)();
    FnAware f2 = (FnAware)(void*)GetProcAddress(u32 ? u32 : GetModuleHandleW(L"user32.dll"),
                                                 "SetProcessDPIAware");
    if (f2) f2();
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    (void)nCmdShow;
    // COM apartment for DTE automation (STA required by EnvDTE)
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // DPI first, then GDI+ and fonts
    EnableDpiAwareness();
    Theme_InitGdiPlus();
    InitCommonControls();

    // catalog from Config_info (baked assets/catalog.cfg)
    Catalog::LoadFromAssets();
    Theme_Reload(nullptr);

    MainWindow mw;
    g_mw = &mw;
    bool ok = mw.Create(hInst);

    int rc = 0;
    if (ok) {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        rc = (int)msg.wParam;
    } else {
        MessageBoxW(nullptr, L"Ana pencere oluşturulamadı.", APP_NAME,
                    MB_ICONERROR);
        rc = 1;
    }

    g_mw = nullptr;
    Theme_ShutdownGdiPlus();
    if (SUCCEEDED(hrCo)) CoUninitialize();
    return rc;
}
