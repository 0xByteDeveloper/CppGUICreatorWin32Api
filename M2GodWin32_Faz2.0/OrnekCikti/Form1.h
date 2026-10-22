// Form1.h — M2GodWin32 UI Designer (Faz 2.0) tarafından üretilmiştir.
#pragma once
#include <windows.h>

namespace M2GodForm {

//== M2GOD:IDS-BEGIN
enum {
    IDC_LABEL1 = 3001,
    IDC_TEXTBOX1 = 3002,
    IDC_BUTTON1 = 3003,
    IDC_CHECKBOX1 = 3004,
    IDC_RADIOBUTTON1 = 3005,
    IDC_COMBOBOX1 = 3006,
    IDC_GROUPBOX1 = 3007,
    IDC_PROGRESSBAR1 = 3008,
    IDC_LISTBOX1 = 3009,
    IDC_LISTVIEW1 = 3010,
    IDC_TREEVIEW1 = 3011,
    IDC_BUTTON2 = 3012
};
//== M2GOD:IDS-END

//== M2GOD:HANDLES-BEGIN
extern HWND label1;
extern HWND textBox1;
extern HWND button1;
extern HWND checkBox1;
extern HWND radioButton1;
extern HWND comboBox1;
extern HWND groupBox1;
extern HWND progressBar1;
extern HWND listBox1;
extern HWND listView1;
extern HWND treeView1;
extern HWND button2;
//== M2GOD:HANDLES-END

//== M2GOD:PROTO-BEGIN
void InitializeControls(HWND hWnd);
//== M2GOD:PROTO-END

//== M2GOD:EVENTDECL-BEGIN
void textBox1_TextChanged(HWND hWnd);
void button1_Click(HWND hWnd);
void Form1_Load(HWND hWnd);
void Form1_Paint(HWND hWnd);
//== M2GOD:EVENTDECL-END

} // namespace M2GodForm
