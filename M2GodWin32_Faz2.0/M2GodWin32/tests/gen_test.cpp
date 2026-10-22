// gen_test.cpp — Faz 2.4 üretilen kod uçtan uca testi:
// Global.h nesne stili + Timer + WM_TIMER + renk + Items üretimi ve derleme doğrulaması
#include "app.h"
#include "catalog.h"
#include "model.h"
#include "codegen.h"

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    g_catalog.Load(L"Config_info.json");

    g_model.formW = 800; g_model.formH = 600;
    g_model.formProps[L"Text"] = L"TimerTest";

    // label1 — tick sayacı (BackColor=Red → WM_CTLCOLORSTATIC testi)
    Ctrl* lb = g_model.Add(L"Label", 20, 20);
    lb->w = 300; lb->h = 23;
    lb->props[L"Text"] = L"Tick -";
    lb->props[L"BackColor"] = L"Red";
    lb->props[L"ForeColor"] = L"White";

    // button1 — timer'ı durduran buton (kullanıcının senaryosu)
    Ctrl* btn = g_model.Add(L"Button", 20, 60);
    btn->w = 160; btn->h = 32;
    btn->props[L"Text"] = L"Durdur";
    btn->events[L"Click"] = L"button1_Click";

    // groupBox1 + checkBox1 içine
    Ctrl* gb = g_model.Add(L"GroupBox", 20, 110);
    gb->w = 220; gb->h = 120;
    gb->props[L"Text"] = L"Ayarlar";
    Ctrl* cb = g_model.Add(L"CheckBox", 24, 36, gb);
    cb->w = 170; cb->h = 24;
    cb->props[L"Text"] = L"Fast (200ms)";
    cb->events[L"CheckedChanged"] = L"checkBox1_CheckedChanged";

    // comboBox1 — Items + SelectedIndexChanged
    Ctrl* cmb = g_model.Add(L"ComboBox", 300, 110);
    cmb->w = 160; cmb->h = 28;
    cmb->props[L"Items"] = L"Kirmizi\nYesil\nMavi";
    cmb->props[L"SelectedIndex"] = L"0";
    cmb->events[L"SelectedIndexChanged"] = L"comboBox1_SelectedIndexChanged";

    // timer1 — Enabled=True → üretilen kod start eder
    Ctrl* tm = g_model.Add(L"Timer", 0, 0);
    tm->props[L"Interval"] = L"500";
    tm->props[L"Enabled"] = L"True";
    tm->events[L"Tick"] = L"timer1_Tick";

    if (!codegen::EmitProject(L"C:\\gtest")) return 1;
    MessageBoxW(nullptr, L"OK: C:\\gtest yazildi", L"gen_test", MB_OK);
    return 0;
}
