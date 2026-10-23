// gen_test.cpp — Faz 2.5 üretilen kod uçtan uca testi:
// TAM KAPSAMLI M2God SDK (v2500): tüm kontroller + diyaloglar + bileşenler +
// WM_NOTIFY/WM_HSCROLL yönlendirmesi + form olayları + kapsamlı özellik emisyonu
#include "app.h"
#include "catalog.h"
#include "model.h"
#include "codegen.h"

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    g_catalog.Load(L"Config_info.json");

    g_model.formW = 900; g_model.formH = 640;
    g_model.formProps[L"Text"] = L"SDK25Test";

    // label1 — tick sayacı (BackColor=Red)
    Ctrl* lb = g_model.Add(L"Label", 20, 20);
    lb->w = 300; lb->h = 23;
    lb->props[L"Text"] = L"Tick -";
    lb->props[L"BackColor"] = L"Red";
    lb->props[L"ForeColor"] = L"White";

    // button1 — durdur (Click)
    Ctrl* btn = g_model.Add(L"Button", 20, 60);
    btn->w = 160; btn->h = 32;
    btn->props[L"Text"] = L"Durdur";
    btn->events[L"Click"] = L"button1_Click";
    btn->events[L"DoubleClick"] = L"button1_DoubleClick";

    // groupBox1 + checkBox1
    Ctrl* gb = g_model.Add(L"GroupBox", 20, 110);
    gb->w = 220; gb->h = 120;
    gb->props[L"Text"] = L"Ayarlar";
    Ctrl* cb = g_model.Add(L"CheckBox", 24, 36, gb);
    cb->w = 170; cb->h = 24;
    cb->props[L"Text"] = L"Fast (200ms)";
    cb->events[L"CheckedChanged"] = L"checkBox1_CheckedChanged";

    // textBox1 — ReadOnly + MaxLength + CharacterCasing emisyonu
    Ctrl* tb = g_model.Add(L"TextBox", 300, 60);
    tb->w = 160; tb->h = 26;
    tb->props[L"Text"] = L"buraya yaz";
    tb->props[L"CharacterCasing"] = L"Upper";
    tb->events[L"TextChanged"] = L"textBox1_TextChanged";

    // comboBox1 — Items + DropDownStyle
    Ctrl* cmb = g_model.Add(L"ComboBox", 300, 110);
    cmb->w = 160; cmb->h = 28;
    cmb->props[L"Items"] = L"Kirmizi\nYesil\nMavi";
    cmb->props[L"SelectedIndex"] = L"0";
    cmb->events[L"SelectedIndexChanged"] = L"comboBox1_SelectedIndexChanged";

    // listBox1 — DoubleClick (LBN_DBLCLK)
    Ctrl* lsb = g_model.Add(L"ListBox", 300, 160);
    lsb->w = 160; lsb->h = 110;
    lsb->props[L"Items"] = L"Elma\nArmut\nKiraz";
    lsb->events[L"DoubleClick"] = L"listBox1_DoubleClick";

    // checkedListBox1 — ItemCheck (LVN_ITEMCHANGED)
    Ctrl* clb = g_model.Add(L"CheckedListBox", 20, 250);
    clb->w = 180; clb->h = 100;
    clb->props[L"Items"] = L"Madde1\nMadde2";
    clb->events[L"ItemCheck"] = L"checkedListBox1_ItemCheck";

    // listView1 — View=Details + SelectedIndexChanged + Click
    Ctrl* lv = g_model.Add(L"ListView", 220, 250);
    lv->w = 240; lv->h = 100;
    lv->props[L"View"] = L"Details";
    lv->events[L"SelectedIndexChanged"] = L"listView1_SelectedIndexChanged";

    // treeView1 — AfterSelect (TVN_SELCHANGED)
    Ctrl* tv = g_model.Add(L"TreeView", 480, 250);
    tv->w = 180; tv->h = 100;
    tv->events[L"AfterSelect"] = L"treeView1_AfterSelect";

    // progressBar1 — Maximum 200 + Step 5
    Ctrl* pb = g_model.Add(L"ProgressBar", 680, 250);
    pb->w = 180; pb->h = 24;
    pb->props[L"Maximum"] = L"200";
    pb->props[L"Step"] = L"5";

    // trackBar1 — Scroll + ValueChanged (WM_HSCROLL)
    Ctrl* trk = g_model.Add(L"TrackBar", 680, 290);
    trk->w = 180; trk->h = 40;
    trk->props[L"Maximum"] = L"50";
    trk->events[L"Scroll"] = L"trackBar1_Scroll";
    trk->events[L"ValueChanged"] = L"trackBar1_ValueChanged";

    // numericUpDown1 — ValueChanged (EN_CHANGE) + Min/Max
    Ctrl* nud = g_model.Add(L"NumericUpDown", 480, 370);
    nud->w = 100; nud->h = 26;
    nud->props[L"Minimum"] = L"0";
    nud->props[L"Maximum"] = L"10";
    nud->props[L"Value"] = L"3";
    nud->events[L"ValueChanged"] = L"numericUpDown1_ValueChanged";

    // dateTimePicker1 — ValueChanged (DTN_DATETIMECHANGE)
    Ctrl* dtp = g_model.Add(L"DateTimePicker", 620, 370);
    dtp->w = 160; dtp->h = 26;
    dtp->events[L"ValueChanged"] = L"dateTimePicker1_ValueChanged";

    // tabControl1 — SelectedIndexChanged (TCN_SELCHANGE) + sayfalar Text'ten
    Ctrl* tc = g_model.Add(L"TabControl", 20, 370);
    tc->w = 220; tc->h = 120;
    tc->props[L"Text"] = L"Sayfa1\nSayfa2";
    tc->events[L"SelectedIndexChanged"] = L"tabControl1_SelectedIndexChanged";

    // statusStrip1 — Parts
    Ctrl* ss = g_model.Add(L"StatusStrip", 0, 0);
    ss->x = 0; ss->y = 580; ss->w = 900; ss->h = 24;
    ss->props[L"Text"] = L"Hazir";

    // menuStrip1 — Items + Apply (üst düzey menü)
    Ctrl* ms = g_model.Add(L"MenuStrip", 0, 0);
    ms->x = 0; ms->y = 0; ms->w = 900; ms->h = 28;
    ms->props[L"Text"] = L"Dosya\nDuzen";

    // pictureBox1 — ImageLocation (tasarimci emisyonu: yaratimda JPG yukler) + SizeMode
    Ctrl* pbx = g_model.Add(L"PictureBox", 20, 510);
    pbx->w = 100; pbx->h = 60;
    pbx->props[L"SizeMode"] = L"StretchImage";
    pbx->props[L"ImageLocation"] = L"C:\\gtest\\m2foto.jpg";

    // pictureBox2 — resmi Form1_Shown icinde M2God::Image::FromFile ile yuklenir (Zoom)
    Ctrl* pbx2 = g_model.Add(L"PictureBox", 140, 510);
    pbx2->w = 100; pbx2->h = 60;
    pbx2->props[L"SizeMode"] = L"Zoom";

    // bileşenler: timer + backgroundWorker + openFileDialog + notifyIcon
    Ctrl* tm = g_model.Add(L"Timer", 0, 0);
    tm->props[L"Interval"] = L"500";
    tm->props[L"Enabled"] = L"True";
    tm->events[L"Tick"] = L"timer1_Tick";

    Ctrl* bw = g_model.Add(L"BackgroundWorker", 0, 0);
    bw->props[L"WorkerReportsProgress"] = L"True";
    bw->events[L"DoWork"] = L"backgroundWorker1_DoWork";
    bw->events[L"ProgressChanged"] = L"backgroundWorker1_ProgressChanged";

    Ctrl* ofd = g_model.Add(L"OpenFileDialog", 0, 0);
    ofd->props[L"Filter"] = L"Metin (*.txt)|*.txt|Tumu (*.*)|*.*";

    Ctrl* ni = g_model.Add(L"NotifyIcon", 0, 0);
    ni->props[L"Text"] = L"SDK25 Test";

    // form olayları: Shown + Resize
    g_model.formEvents[L"Shown"] = L"Form1_Shown";
    g_model.formEvents[L"Resize"] = L"Form1_Resize";

    if (!codegen::EmitProject(L"C:\\gtest")) return 1;

    // kullanıcının yazacağı olay gövdelerini EKLE (EVENTS bölgesine işlenir)
    // — bu satırlar senkron korunur; test amaçlı dosyaya işleniyor
    MessageBoxW(nullptr, L"OK: C:\\gtest yazildi", L"gen_test", MB_OK);
    return 0;
}
