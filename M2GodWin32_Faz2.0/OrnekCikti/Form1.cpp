// Form1.cpp — M2GodWin32 UI Designer (Faz 2.0) tarafından üretilmiştir.
// Marker blokları (//== M2GOD:... ) tasarımcı senkronizasyonu içindir; dışını düzenleyebilirsiniz.

#include "Form1.h"
#include <commctrl.h>

#pragma comment(lib, "Comctl32.lib")

namespace M2GodForm {

static HFONT  hFont     = NULL;
static HBRUSH hBackBrush= NULL;
static HBRUSH hCtrlBrush= NULL;

static void EnsureStatics(HINSTANCE hInst) {
    if (hFont) return;
    hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
}

//== M2GOD:HANDLES-BEGIN
HWND label1 = NULL;
HWND textBox1 = NULL;
HWND button1 = NULL;
HWND checkBox1 = NULL;
HWND radioButton1 = NULL;
HWND comboBox1 = NULL;
HWND groupBox1 = NULL;
HWND progressBar1 = NULL;
HWND listBox1 = NULL;
HWND listView1 = NULL;
HWND treeView1 = NULL;
HWND button2 = NULL;
//== M2GOD:HANDLES-END

//== M2GOD:CREATE-BEGIN
void InitializeControls(HWND hWnd) {
    HINSTANCE hInst = GetModuleHandleW(NULL);
    EnsureStatics(hInst);
    label1 = CreateWindowExW(0, L"STATIC", L"Win32 API Görsel Tasarım Ortamı",
        SS_LEFT | SS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 24, 20, 320, 26,
        hWnd, (HMENU)(INT_PTR)IDC_LABEL1, hInst, NULL);
    SendMessageW(label1, WM_SETFONT, (WPARAM)hFont, TRUE);

    textBox1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Merhaba M2God",
        WS_BORDER | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 24, 60, 220, 26,
        hWnd, (HMENU)(INT_PTR)IDC_TEXTBOX1, hInst, NULL);
    SendMessageW(textBox1, WM_SETFONT, (WPARAM)hFont, TRUE);

    button1 = CreateWindowExW(0, L"BUTTON", L"Tıkla",
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 260, 58, 120, 34,
        hWnd, (HMENU)(INT_PTR)IDC_BUTTON1, hInst, NULL);
    SendMessageW(button1, WM_SETFONT, (WPARAM)hFont, TRUE);

    checkBox1 = CreateWindowExW(0, L"BUTTON", L"Otomatik senkron",
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 24, 104, 180, 26,
        hWnd, (HMENU)(INT_PTR)IDC_CHECKBOX1, hInst, NULL);
    SendMessageW(checkBox1, WM_SETFONT, (WPARAM)hFont, TRUE);

    radioButton1 = CreateWindowExW(0, L"BUTTON", L"Win32 API modu",
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 24, 138, 150, 26,
        hWnd, (HMENU)(INT_PTR)IDC_RADIOBUTTON1, hInst, NULL);
    SendMessageW(radioButton1, WM_SETFONT, (WPARAM)hFont, TRUE);

    comboBox1 = CreateWindowExW(0, L"COMBOBOX", L"",
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 24, 176, 160, 28,
        hWnd, (HMENU)(INT_PTR)IDC_COMBOBOX1, hInst, NULL);
    SendMessageW(comboBox1, WM_SETFONT, (WPARAM)hFont, TRUE);

    groupBox1 = CreateWindowExW(0, L"BUTTON", L"Ayarlar",
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 210, 108, 220, 110,
        hWnd, (HMENU)(INT_PTR)IDC_GROUPBOX1, hInst, NULL);
    SendMessageW(groupBox1, WM_SETFONT, (WPARAM)hFont, TRUE);

    progressBar1 = CreateWindowExW(0, L"msctls_progress32", L"",
        0 | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 210, 232, 220, 26,
        hWnd, (HMENU)(INT_PTR)IDC_PROGRESSBAR1, hInst, NULL);
    SendMessageW(progressBar1, WM_SETFONT, (WPARAM)hFont, TRUE);

    listBox1 = CreateWindowExW(0, L"LISTBOX", L"",
        WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 24, 224, 160, 120,
        hWnd, (HMENU)(INT_PTR)IDC_LISTBOX1, hInst, NULL);
    SendMessageW(listBox1, WM_SETFONT, (WPARAM)hFont, TRUE);

    listView1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysListView32", L"",
        WS_BORDER | LVS_REPORT | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 460, 60, 250, 130,
        hWnd, (HMENU)(INT_PTR)IDC_LISTVIEW1, hInst, NULL);
    SendMessageW(listView1, WM_SETFONT, (WPARAM)hFont, TRUE);

    treeView1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysTreeView32", L"",
        WS_BORDER | TVS_HASLINES | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 460, 208, 250, 140,
        hWnd, (HMENU)(INT_PTR)IDC_TREEVIEW1, hInst, NULL);
    SendMessageW(treeView1, WM_SETFONT, (WPARAM)hFont, TRUE);

    button2 = CreateWindowExW(0, L"BUTTON", L"Çıkış",
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 460, 364, 120, 34,
        hWnd, (HMENU)(INT_PTR)IDC_BUTTON2, hInst, NULL);
    SendMessageW(button2, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Timer: SetTimer(hWnd, TIMER_timer1, 1000, NULL);

}
//== M2GOD:CREATE-END

//== M2GOD:EVENTS-BEGIN  (olay işleyicileri; mevcut gövdeler senkronda korunur)
void textBox1_TextChanged(HWND hWnd) {
    // TODO: textBox1_TextChanged — kodunuzu buraya yazın
}

void button1_Click(HWND hWnd) {
    // TODO: button1_Click — kodunuzu buraya yazın
}

void Form1_Load(HWND hWnd) {
    // TODO: Form1_Load — kodunuzu buraya yazın
}

void Form1_Paint(HWND hWnd) {
    // TODO: Form1_Paint — kodunuzu buraya yazın
}

//== M2GOD:EVENTS-END

} // namespace M2GodForm

//== M2GOD:WNDPROC-BEGIN
static LRESULT CALLBACK Form1_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        M2GodForm::InitializeControls(hWnd);
        M2GodForm::Form1_Load(hWnd);
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wParam), ev = HIWORD(wParam);
        switch (id) {
        case M2GodForm::IDC_TEXTBOX1:
            if (ev == EN_CHANGE) M2GodForm::textBox1_TextChanged(hWnd);
            break;

        case M2GodForm::IDC_BUTTON1:
            if (ev == BN_CLICKED) M2GodForm::button1_Click(hWnd);
            break;

        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    M2GodForm::EnsureStatics(hInst);
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc  = Form1_WndProc;
    wc.hInstance    = hInst;
    wc.hCursor      = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(240,240,240));
    wc.lpszClassName = L"M2GodForm1";
    RegisterClassW(&wc);

    // Form1 boyutu (ClientSize: 760 × 520)
    RECT rc = { 0, 0, 760, 520 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindowW(L"M2GodForm1", L"M2GodWin32 — God Mode MVP", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);
    if (!hWnd) return 0;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
//== M2GOD:WNDPROC-END
