// main.cpp — giriş noktası
#include "app.h"
#include "mainwnd.h"

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR cmdLine, int) {
    return mainwnd::Run(hInst, cmdLine);
}
