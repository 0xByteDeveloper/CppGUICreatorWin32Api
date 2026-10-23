// hstub.cpp — harness için main↔wWinMain uyarlayıcı (-mconsole derlemesi)
#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif
#define UNICODE 1
#define _UNICODE 1
#include <windows.h>
#include <stdio.h>
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int);
extern "C" int main(int argc, char** argv) {
    (void)argc; (void)argv;
    return wWinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_SHOWNORMAL);
}
