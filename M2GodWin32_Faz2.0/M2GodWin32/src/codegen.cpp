// codegen.cpp — kod üretici (Bölüm 1: Form1.h/cpp + Global.h üretimi)
// Faz 2.4: M2God nesne sarmalayıcı kütüphanesi (Global.h) + gerçek Timer
//   - timer1->Enabled = false;  button1->Text = L"Merhaba";  sözdizimi
//   - WM_TIMER gerçek dispatch (SetTimer/KillTimer)
//   - Enabled/Visible/Font/BackColor/ForeColor/Items üretilen koda taşınır
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

// metin kaçışı: C++ geniş dize sabiti içinde güvenli hale getirir
static std::wstring EscCppStr(const std::wstring& s) {
    std::wstring e;
    for (wchar_t ch : s) {
        if (ch == L'\\') e += L"\\\\";
        else if (ch == L'"') e += L"\\\"";
        else if (ch == L'\n') e += L"\\n";
        else if (ch == L'\r') e += L"\\r";
        else if (ch == L'\t') e += L"\\t";
        else e += ch;
    }
    return e;
}

// tip → Global.h sarmalayıcı sınıf adı
static std::wstring M2TypeOf(const std::wstring& type) {
    struct M { const wchar_t* a; const wchar_t* b; };
    static const M map[] = {
        { L"Button", L"Button" }, { L"CheckBox", L"CheckBox" }, { L"RadioButton", L"RadioButton" },
        { L"GroupBox", L"GroupBox" }, { L"Label", L"Label" }, { L"LinkLabel", L"Label" },
        { L"TextBox", L"TextBox" }, { L"MaskedTextBox", L"TextBox" }, { L"NumericUpDown", L"NumericUpDown" },
        { L"RichTextBox", L"RichTextBox" }, { L"ComboBox", L"ComboBox" },
        { L"ListBox", L"ListBox" }, { L"CheckedListBox", L"CheckedListBox" },
        { L"ListView", L"ListView" }, { L"TreeView", L"TreeView" },
        { L"ProgressBar", L"ProgressBar" }, { L"TrackBar", L"TrackBar" },
        { L"HScrollBar", L"HScrollBar" }, { L"VScrollBar", L"VScrollBar" },
        { L"DateTimePicker", L"DateTimePicker" }, { L"MonthCalendar", L"MonthCalendar" },
        { L"TabControl", L"TabControl" }, { L"StatusStrip", L"StatusStrip" },
        { L"Panel", L"Panel" }, { L"FlowLayoutPanel", L"Panel" }, { L"TableLayoutPanel", L"Panel" },
        { L"SplitContainer", L"Panel" }, { L"PictureBox", L"PictureBox" },
        { L"LinkLabel", L"LinkLabel" }, { L"MenuStrip", L"MenuStrip" },
        { L"ToolStrip", L"MenuStrip" }, { L"ContextMenuStrip", L"ContextMenuStrip" },
    };
    for (auto& m : map) if (_wcsicmp(m.a, type.c_str()) == 0) return m.b;
    return L"Placeholder";
}

// Faz 2.5: görsel olmayan bileşenler için Global.h sarmalayıcısı (yoksa boş)
static std::wstring M2ComponentTypeOf(const std::wstring& type) {
    struct M { const wchar_t* a; const wchar_t* b; };
    static const M map[] = {
        { L"NotifyIcon", L"NotifyIcon" }, { L"ToolTip", L"ToolTip" },
        { L"BackgroundWorker", L"BackgroundWorker" }, { L"Process", L"Process" },
        { L"SerialPort", L"SerialPort" },
        { L"OpenFileDialog", L"OpenFileDialog" }, { L"SaveFileDialog", L"SaveFileDialog" },
        { L"ColorDialog", L"ColorDialog" }, { L"FontDialog", L"FontDialog" },
        { L"FolderBrowserDialog", L"FolderBrowserDialog" },
    };
    for (auto& m : map) if (_wcsicmp(m.a, type.c_str()) == 0) return m.b;
    return L"";
}

const char* GlobalVersionMark() { return "M2GOD-GLOBAL v2501"; }

// ============================================================================
//  Global.h şablonu — M2God TAM KAPSAMLI SDK (Faz 2.5)
//  Şablon ayrı dosyada: src/global_template.inc (M2GOD-GLOBAL v2501, C++14 uyumlu)
//  Config_info.json'daki tüm kontroller/bileşenler/diyaloglar + enumlar + Color
// ============================================================================
std::wstring GenerateGlobalH() {
    static const char kTemplate[] =
#include "global_template.inc"
    ;
    return Utf8ToWide(kTemplate);
}

// kontrol için CREATE satırı üretimi (PARENT-AWARE, FAZ 2.5 TAM KAPSAMLI)
static std::wstring BoolLit(const std::wstring& v, bool def = false) {
    if (v.empty()) return def ? L"true" : L"false";
    return _wcsicmp(v.c_str(), L"true") == 0 ? L"true" : L"false";
}

// görsel olmayan SDK bileşenleri (Faz 2.5)
static std::wstring EmitComponentCreate(const Ctrl* c) {
    std::wostringstream o;
    const std::wstring& t = c->type;
    if (t == L"OpenFileDialog" || t == L"SaveFileDialog") {
        o << L"    " << c->name << L" = new M2God::" << t << L"();\n";
        std::wstring f = c->Get(L"Filter");   if (!f.empty())  o << L"    " << c->name << L"->Filter = L\"" << EscCppStr(f) << L"\";\n";
        std::wstring fn = c->Get(L"FileName"); if (!fn.empty()) o << L"    " << c->name << L"->FileName = L\"" << EscCppStr(fn) << L"\";\n";
        std::wstring ti = c->Get(L"Title");   if (!ti.empty()) o << L"    " << c->name << L"->Title = L\"" << EscCppStr(ti) << L"\";\n";
        std::wstring id = c->Get(L"InitialDirectory"); if (!id.empty()) o << L"    " << c->name << L"->InitialDirectory = L\"" << EscCppStr(id) << L"\";\n";
        if (BoolLit(c->Get(L"Multiselect")) != L"false")
            o << L"    " << c->name << L"->Multiselect = " << BoolLit(c->Get(L"Multiselect")) << L";\n";
        if (t == L"SaveFileDialog") {
            std::wstring de = c->Get(L"DefaultExt"); if (!de.empty()) o << L"    " << c->name << L"->DefaultExt = L\"" << EscCppStr(de) << L"\";\n";
            if (_wcsicmp(c->Get(L"OverwritePrompt", L"True").c_str(), L"false") == 0)
                o << L"    " << c->name << L"->OverwritePrompt = false;\n";
        }
        return o.str();
    }
    if (t == L"ColorDialog") {
        o << L"    " << c->name << L" = new M2God::ColorDialog();\n";
        if (_wcsicmp(c->Get(L"FullOpen", L"False").c_str(), L"true") == 0)
            o << L"    " << c->name << L"->FullOpen = true;\n";
        return o.str();
    }
    if (t == L"FontDialog") {
        o << L"    " << c->name << L" = new M2God::FontDialog();\n";
        return o.str();
    }
    if (t == L"FolderBrowserDialog") {
        o << L"    " << c->name << L" = new M2God::FolderBrowserDialog();\n";
        std::wstring d = c->Get(L"Description"); if (!d.empty()) o << L"    " << c->name << L"->Description = L\"" << EscCppStr(d) << L"\";\n";
        return o.str();
    }
    if (t == L"BackgroundWorker") {
        o << L"    " << c->name << L" = new M2God::BackgroundWorker();\n";
        o << L"    " << c->name << L"->Owner = hWnd;\n";
        if (_wcsicmp(c->Get(L"WorkerReportsProgress", L"False").c_str(), L"true") == 0)
            o << L"    " << c->name << L"->WorkerReportsProgress = true;\n";
        if (_wcsicmp(c->Get(L"WorkerSupportsCancellation", L"False").c_str(), L"true") == 0)
            o << L"    " << c->name << L"->WorkerSupportsCancellation = true;\n";
        auto ev = c->events.find(L"DoWork");
        if (ev != c->events.end() && !ev->second.empty())
            o << L"    " << c->name << L"->DoWork = [](){ " << ev->second << L"(M2God::Form::Handle); };\n";
        auto evp = c->events.find(L"ProgressChanged");
        if (evp != c->events.end() && !evp->second.empty())
            o << L"    " << c->name << L"->ProgressChanged = [](int p){ " << evp->second << L"(M2God::Form::Handle); (void)p; };\n";
        auto evc = c->events.find(L"RunWorkerCompleted");
        if (evc != c->events.end() && !evc->second.empty())
            o << L"    " << c->name << L"->RunWorkerCompleted = [](){ " << evc->second << L"(M2God::Form::Handle); };\n";
        return o.str();
    }
    if (t == L"NotifyIcon") {
        o << L"    " << c->name << L" = new M2God::NotifyIcon(hWnd);\n";
        std::wstring tx = c->Get(L"Text"); if (!tx.empty()) o << L"    " << c->name << L"->Text = L\"" << EscCppStr(tx) << L"\";\n";
        if (_wcsicmp(c->Get(L"Visible", L"True").c_str(), L"true") == 0)
            o << L"    " << c->name << L"->Visible = true;\n";
        std::wstring bt = c->Get(L"BalloonTipTitle"); if (!bt.empty()) o << L"    " << c->name << L"->BalloonTipTitle = L\"" << EscCppStr(bt) << L"\";\n";
        std::wstring bx = c->Get(L"BalloonTipText");  if (!bx.empty()) o << L"    " << c->name << L"->BalloonTipText = L\"" << EscCppStr(bx) << L"\";\n";
        return o.str();
    }
    if (t == L"ToolTip") {
        o << L"    " << c->name << L" = new M2God::ToolTip(hWnd);\n";
        if (_wcsicmp(c->Get(L"Active", L"True").c_str(), L"false") == 0)
            o << L"    " << c->name << L"->Active = false;\n";
        return o.str();
    }
    if (t == L"Process") {
        o << L"    " << c->name << L" = new M2God::Process();\n";
        std::wstring fn = c->Get(L"FileName");  if (!fn.empty()) o << L"    " << c->name << L"->FileName = L\"" << EscCppStr(fn) << L"\";\n";
        std::wstring ar = c->Get(L"Arguments"); if (!ar.empty()) o << L"    " << c->name << L"->Arguments = L\"" << EscCppStr(ar) << L"\";\n";
        return o.str();
    }
    if (t == L"SerialPort") {
        o << L"    " << c->name << L" = new M2God::SerialPort();\n";
        std::wstring pn = c->Get(L"PortName"); if (!pn.empty()) o << L"    " << c->name << L"->PortName = L\"" << EscCppStr(pn) << L"\";\n";
        std::wstring br = c->Get(L"BaudRate"); if (!br.empty() && iswdigit(br[0])) o << L"    " << c->name << L"->BaudRate = " << _wtoi(br.c_str()) << L";\n";
        return o.str();
    }
    o << L"    // " << t << L" bileşeni (Global.h sarmalayıcısı yok — katalog bilgi amaçlı)\n";
    return o.str();
}

// enum değeri → ContentAlignment sabiti (tasarım metni WinForms adı)
static std::wstring EmitContentAlignment(const std::wstring& v) {
    if (_wcsicmp(v.c_str(), L"TopLeft") == 0) return L"M2God::ContentAlignment::TopLeft";
    if (_wcsicmp(v.c_str(), L"TopCenter") == 0) return L"M2God::ContentAlignment::TopCenter";
    if (_wcsicmp(v.c_str(), L"TopRight") == 0) return L"M2God::ContentAlignment::TopRight";
    if (_wcsicmp(v.c_str(), L"BottomLeft") == 0) return L"M2God::ContentAlignment::BottomLeft";
    if (_wcsicmp(v.c_str(), L"BottomCenter") == 0) return L"M2God::ContentAlignment::BottomCenter";
    if (_wcsicmp(v.c_str(), L"BottomRight") == 0) return L"M2God::ContentAlignment::BottomRight";
    if (_wcsicmp(v.c_str(), L"Left") == 0 || _wcsicmp(v.c_str(), L"MiddleLeft") == 0) return L"M2God::ContentAlignment::MiddleLeft";
    if (_wcsicmp(v.c_str(), L"Right") == 0 || _wcsicmp(v.c_str(), L"MiddleRight") == 0) return L"M2God::ContentAlignment::MiddleRight";
    return L"M2God::ContentAlignment::MiddleCenter";
}

static std::wstring EmitCreateCall(const Ctrl* c) {
    std::wostringstream o;
    if (c->type == L"Timer") {
        o << L"    " << c->name << L" = new M2God::Timer(hWnd, " << IdMacroOf(c->name) << L");\n";
        o << L"    " << c->name << L"->Interval = " << c->Get(L"Interval", L"100") << L";\n";
        if (_wcsicmp(c->Get(L"Enabled", L"False").c_str(), L"true") == 0)
            o << L"    " << c->name << L"->Enabled = true;   // tasarımda Enabled=True\n";
        else
            o << L"    // " << c->name << L"->Enabled = true;   // Timer'ı başlatmak için\n";
        return o.str();
    }
    if (!M2ComponentTypeOf(c->type).empty()) return EmitComponentCreate(c);
    if (c->nonVisual) return EmitComponentCreate(c);
    factory::Win32Class m = factory::MapClass(c->type);
    if (m.cls.empty()) return EmitComponentCreate(c);
    std::wstring parentVar = c->parent ? (c->parent->name + L"->Handle") : std::wstring(L"hWnd");
    if (c->parent)
        o << L"    // " << c->name << L" → " << c->parent->name << L" içine (child)\n";
    std::wstring txt = c->Get(L"Text");
    std::wstring esc = EscCppStr(txt);
    bool isTabCtrl = (c->type == L"TabControl");
    std::wstring ctorText = isTabCtrl ? L"" : esc;   // TabControl sayfaları Pages->Add ile eklenir
    std::wstring ctorArgs = parentVar + L", " + IdMacroOf(c->name) + L", L\"" + ctorText + L"\", " +
                            std::to_wstring(c->x) + L", " + std::to_wstring(c->y) + L", " +
                            std::to_wstring(c->w) + L", " + std::to_wstring(c->h);
    // TextBox ailesi: multiline parametresi
    if (c->type == L"TextBox" || c->type == L"MaskedTextBox" || c->type == L"NumericUpDown") {
        bool ml = (c->type == L"TextBox") && _wcsicmp(c->Get(L"MultiLine", L"False").c_str(), L"true") == 0;
        ctorArgs += ml ? L", true" : L", false";
    }
    o << L"    " << c->name << L" = new M2God::" << M2TypeOf(c->type) << L"(" << ctorArgs << L");\n";
    // --- özellik emisyonu ---
    auto itItems = c->props.find(L"Items");
    if (itItems != c->props.end() && !itItems->second.empty()) {
        o << L"    " << c->name << L"->Items->Clear();\n";
        for (auto& line : Split(itItems->second, L'\n')) {
            std::wstring t = Trim(line);
            if (!t.empty())
                o << L"    " << c->name << L"->Items->Add(L\"" << EscCppStr(t) << L"\");\n";
        }
    }
    {
        std::wstring si = c->Get(L"SelectedIndex");
        if (!si.empty() && iswdigit(si[0]) && _wtoi(si.c_str()) >= 0)
            o << L"    " << c->name << L"->Items->SelectedIndex = " << _wtoi(si.c_str()) << L";\n";
    }
    // BackColor / ForeColor
    {
        std::wstring bv = c->Get(L"BackColor");
        if (!bv.empty() && _wcsicmp(bv.c_str(), L"Control") != 0) {
            COLORREF cr;
            if (ParseColor(bv, cr))
                o << L"    " << c->name << L"->BackColor = RGB(" << (int)GetRValue(cr) << L", "
                  << (int)GetGValue(cr) << L", " << (int)GetBValue(cr) << L");\n";
        }
        std::wstring fv = c->Get(L"ForeColor");
        if (!fv.empty() && _wcsicmp(fv.c_str(), L"ControlText") != 0) {
            COLORREF cr;
            if (ParseColor(fv, cr))
                o << L"    " << c->name << L"->ForeColor = RGB(" << (int)GetRValue(cr) << L", "
                  << (int)GetGValue(cr) << L", " << (int)GetBValue(cr) << L");\n";
        }
    }
    // Font
    {
        std::wstring fnt = c->Get(L"Font", L"Segoe UI, 9pt");
        std::wstring fn; int pt; bool bo, it, un;
        if (ParseFontValue(fnt, fn, pt, bo, it, un)) {
            bool isDefault = (_wcsicmp(fn.c_str(), L"Segoe UI") == 0 && pt == 9 && !bo && !it && !un);
            if (!isDefault) {
                o << L"    " << c->name << L"->Font = M2God::MakeFont(L\"" << EscCppStr(fn) << L"\", "
                  << pt << L", " << (bo ? L"true" : L"false") << L", " << (it ? L"true" : L"false")
                  << L", " << (un ? L"true" : L"false") << L");\n";
            }
        }
    }
    // Checked (CheckBox/RadioButton)
    if ((c->type == L"CheckBox" || c->type == L"RadioButton") && _wcsicmp(c->Get(L"Checked", L"False").c_str(), L"true") == 0)
        o << L"    " << c->name << L"->Checked = true;\n";
    // Enabled / Visible
    if (_wcsicmp(c->Get(L"Enabled", L"True").c_str(), L"false") == 0)
        o << L"    " << c->name << L"->Enabled = false;\n";
    if (_wcsicmp(c->Get(L"Visible", L"True").c_str(), L"false") == 0)
        o << L"    " << c->name << L"->Visible = false;\n";

    // ===================== FAZ 2.5: KAPSAMLI ÖZELLİK EMİSYONU =====================
    const std::wstring& T = c->type;
    std::wstring N = c->name;
    // TextBox / MaskedTextBox
    if (T == L"TextBox" || T == L"MaskedTextBox") {
        if (_wcsicmp(c->Get(L"ReadOnly", L"False").c_str(), L"true") == 0)
            o << L"    " << N << L"->ReadOnly = true;\n";
        { std::wstring v = c->Get(L"MaxLength"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 0)
            o << L"    " << N << L"->MaxLength = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"PasswordChar");
          if (!v.empty() && v[0]) o << L"    " << N << L"->PasswordChar = L'" << v[0] << L"';\n"; }
        { std::wstring v = c->Get(L"CharacterCasing");
          if (_wcsicmp(v.c_str(), L"Upper") == 0) o << L"    " << N << L"->CharacterCasing = M2God::CharacterCasing::Upper;\n";
          else if (_wcsicmp(v.c_str(), L"Lower") == 0) o << L"    " << N << L"->CharacterCasing = M2God::CharacterCasing::Lower;\n"; }
        { std::wstring v = c->Get(L"TextAlign");
          if (_wcsicmp(v.c_str(), L"Center") == 0) o << L"    " << N << L"->TextAlign = M2God::HorizontalAlignment::Center;\n";
          else if (_wcsicmp(v.c_str(), L"Right") == 0) o << L"    " << N << L"->TextAlign = M2God::HorizontalAlignment::Right;\n"; }
        if (T == L"MaskedTextBox") {
            std::wstring mk = c->Get(L"Mask"); if (!mk.empty()) o << L"    " << N << L"->Mask = L\"" << EscCppStr(mk) << L"\";   // bilgi amaçlı\n";
        }
    }
    // NumericUpDown
    if (T == L"NumericUpDown") {
        { std::wstring v = c->Get(L"Minimum"); if (!v.empty() && iswdigit(v[0])) o << L"    " << N << L"->Minimum = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Maximum"); if (!v.empty() && iswdigit(v[0])) o << L"    " << N << L"->Maximum = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Value");   if (!v.empty() && iswdigit(v[0])) o << L"    " << N << L"->Value = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Increment"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 0) o << L"    " << N << L"->Increment = " << _wtoi(v.c_str()) << L";\n"; }
        if (_wcsicmp(c->Get(L"ReadOnly", L"False").c_str(), L"true") == 0)
            o << L"    " << N << L"->ReadOnly = true;\n";
    }
    // RichTextBox
    if (T == L"RichTextBox") {
        if (_wcsicmp(c->Get(L"ReadOnly", L"False").c_str(), L"true") == 0)
            o << L"    " << N << L"->ReadOnly = true;\n";
        { std::wstring v = c->Get(L"MaxLength"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 0)
            o << L"    " << N << L"->MaxLength = " << _wtoi(v.c_str()) << L";\n"; }
    }
    // ComboBox
    if (T == L"ComboBox") {
        { std::wstring v = c->Get(L"DropDownStyle");
          if (_wcsicmp(v.c_str(), L"DropDown") == 0) o << L"    " << N << L"->DropDownStyle = M2God::ComboBoxStyle::DropDown;\n";
          else if (_wcsicmp(v.c_str(), L"Simple") == 0) o << L"    " << N << L"->DropDownStyle = M2God::ComboBoxStyle::Simple;\n"; }
        { std::wstring v = c->Get(L"MaxLength"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 0)
            o << L"    " << N << L"->MaxLength = " << _wtoi(v.c_str()) << L";\n"; }
    }
    // ListView
    if (T == L"ListView") {
        { std::wstring v = c->Get(L"View");
          if (_wcsicmp(v.c_str(), L"List") == 0) o << L"    " << N << L"->View = M2God::View::List;\n";
          else if (_wcsicmp(v.c_str(), L"LargeIcon") == 0) o << L"    " << N << L"->View = M2God::View::LargeIcon;\n";
          else if (_wcsicmp(v.c_str(), L"SmallIcon") == 0) o << L"    " << N << L"->View = M2God::View::SmallIcon;\n"; }
        if (_wcsicmp(c->Get(L"CheckBoxes", L"False").c_str(), L"true") == 0)
            o << L"    " << N << L"->CheckBoxes = true;\n";
        if (_wcsicmp(c->Get(L"GridLines", L"False").c_str(), L"true") == 0)
            o << L"    " << N << L"->GridLines = true;\n";
        if (_wcsicmp(c->Get(L"FullRowSelect", L"True").c_str(), L"false") == 0)
            o << L"    " << N << L"->FullRowSelect = false;\n";
        { std::wstring v = c->Get(L"HeaderStyle");
          if (_wcsicmp(v.c_str(), L"None") == 0) o << L"    " << N << L"->HeaderStyle = M2God::HeaderStyle::None;\n";
          else if (_wcsicmp(v.c_str(), L"NonClickable") == 0) o << L"    " << N << L"->HeaderStyle = M2God::HeaderStyle::NonClickable;\n"; }
    }
    // TreeView
    if (T == L"TreeView") {
        if (_wcsicmp(c->Get(L"CheckBoxes", L"False").c_str(), L"true") == 0)
            o << L"    " << N << L"->CheckBoxes = true;   // ogeler eklenmeden uygulanır\n";
        if (_wcsicmp(c->Get(L"ShowLines", L"True").c_str(), L"false") == 0)
            o << L"    " << N << L"->ShowLines = false;\n";
        if (_wcsicmp(c->Get(L"ShowPlusMinus", L"True").c_str(), L"false") == 0)
            o << L"    " << N << L"->ShowPlusMinus = false;\n";
        if (_wcsicmp(c->Get(L"ShowRootLines", L"True").c_str(), L"false") == 0)
            o << L"    " << N << L"->ShowRootLines = false;\n";
        if (_wcsicmp(c->Get(L"HotTracking", L"False").c_str(), L"true") == 0)
            o << L"    " << N << L"->HotTracking = true;\n";
    }
    // ProgressBar
    if (T == L"ProgressBar") {
        { std::wstring v = c->Get(L"Minimum"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 0)
            o << L"    " << N << L"->Minimum = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Maximum"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 100)
            o << L"    " << N << L"->Maximum = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Value");   if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 0)
            o << L"    " << N << L"->Value = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Step");    if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 10)
            o << L"    " << N << L"->Step = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Style");
          if (_wcsicmp(v.c_str(), L"Marquee") == 0) o << L"    " << N << L"->Style = M2God::ProgressBarStyle::Marquee;\n";
          else if (_wcsicmp(v.c_str(), L"Continuous") == 0) o << L"    " << N << L"->Style = M2God::ProgressBarStyle::Continuous;\n"; }
    }
    // TrackBar
    if (T == L"TrackBar") {
        { std::wstring v = c->Get(L"Minimum"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 0)
            o << L"    " << N << L"->Minimum = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Maximum"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 100)
            o << L"    " << N << L"->Maximum = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Value");   if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 0)
            o << L"    " << N << L"->Value = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"TickFrequency"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 0)
            o << L"    " << N << L"->TickFrequency = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"SmallChange"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 1)
            o << L"    " << N << L"->SmallChange = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"LargeChange"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 10)
            o << L"    " << N << L"->LargeChange = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Orientation");
          if (_wcsicmp(v.c_str(), L"Vertical") == 0) o << L"    // " << N << L"->Orientation = Vertical (yaratma stili TBS_VERT gerektirir)\n"; }
    }
    // ScrollBar'lar
    if (T == L"HScrollBar" || T == L"VScrollBar") {
        { std::wstring v = c->Get(L"Minimum"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 0)
            o << L"    " << N << L"->Minimum = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Maximum"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 100)
            o << L"    " << N << L"->Maximum = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"Value");   if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 0)
            o << L"    " << N << L"->Value = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"SmallChange"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 1)
            o << L"    " << N << L"->SmallChange = " << _wtoi(v.c_str()) << L";\n"; }
        { std::wstring v = c->Get(L"LargeChange"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) != 10)
            o << L"    " << N << L"->LargeChange = " << _wtoi(v.c_str()) << L";\n"; }
    }
    // DateTimePicker
    if (T == L"DateTimePicker") {
        { std::wstring v = c->Get(L"Format");
          if (_wcsicmp(v.c_str(), L"Long") == 0) o << L"    " << N << L"->Format = M2God::DateTimePickerFormat::Long;\n";
          else if (_wcsicmp(v.c_str(), L"Time") == 0) o << L"    " << N << L"->Format = M2God::DateTimePickerFormat::Time;\n";
          else if (_wcsicmp(v.c_str(), L"Custom") == 0) o << L"    " << N << L"->Format = M2God::DateTimePickerFormat::Custom;\n"; }
        { std::wstring v = c->Get(L"CustomFormat"); if (!v.empty())
            o << L"    " << N << L"->CustomFormat = L\"" << EscCppStr(v) << L"\";\n"; }
    }
    // MonthCalendar
    if (T == L"MonthCalendar") {
        { std::wstring v = c->Get(L"MaxSelectionCount"); if (!v.empty() && iswdigit(v[0]) && _wtoi(v.c_str()) > 1)
            o << L"    " << N << L"->MaxSelectionCount = " << _wtoi(v.c_str()) << L";\n"; }
    }
    // Panel
    if (T == L"Panel") {
        { std::wstring v = c->Get(L"BorderStyle");
          if (_wcsicmp(v.c_str(), L"FixedSingle") == 0) o << L"    " << N << L"->BorderStyle = M2God::BorderStyle::FixedSingle;\n";
          else if (_wcsicmp(v.c_str(), L"Fixed3D") == 0) o << L"    " << N << L"->BorderStyle = M2God::BorderStyle::Fixed3D;\n";
          else if (_wcsicmp(v.c_str(), L"None") == 0) o << L"    " << N << L"->BorderStyle = M2God::BorderStyle::None;\n"; }
    }
    // PictureBox
    if (T == L"PictureBox") {
        { std::wstring v = c->Get(L"SizeMode");
          if (_wcsicmp(v.c_str(), L"StretchImage") == 0) o << L"    " << N << L"->SizeMode = M2God::PictureBoxSizeMode::StretchImage;\n";
          else if (_wcsicmp(v.c_str(), L"AutoSize") == 0) o << L"    " << N << L"->SizeMode = M2God::PictureBoxSizeMode::AutoSize;\n";
          else if (_wcsicmp(v.c_str(), L"CenterImage") == 0) o << L"    " << N << L"->SizeMode = M2God::PictureBoxSizeMode::CenterImage;\n";
          else if (_wcsicmp(v.c_str(), L"Zoom") == 0) o << L"    " << N << L"->SizeMode = M2God::PictureBoxSizeMode::Zoom;\n"; }
        { std::wstring v = c->Get(L"ImageLocation"); if (!v.empty())
            o << L"    " << N << L"->ImageLocation = L\"" << EscCppStr(v) << L"\";\n"; }
    }
    // Label / LinkLabel
    if (T == L"Label" || T == L"LinkLabel") {
        { std::wstring v = c->Get(L"TextAlign");
          if (!v.empty() && _wcsicmp(v.c_str(), L"TopLeft") != 0)
            o << L"    " << N << L"->TextAlign = " << EmitContentAlignment(v) << L";\n"; }
        { std::wstring v = c->Get(L"BorderStyle");
          if (_wcsicmp(v.c_str(), L"FixedSingle") == 0) o << L"    " << N << L"->BorderStyle = M2God::BorderStyle::FixedSingle;\n";
          else if (_wcsicmp(v.c_str(), L"Fixed3D") == 0) o << L"    " << N << L"->BorderStyle = M2God::BorderStyle::Fixed3D;\n"; }
        if (_wcsicmp(c->Get(L"AutoEllipsis", L"False").c_str(), L"true") == 0)
            o << L"    " << N << L"->AutoEllipsis = true;\n";
    }
    // Button
    if (T == L"Button") {
        { std::wstring v = c->Get(L"TextAlign");
          if (!v.empty() && _wcsicmp(v.c_str(), L"MiddleCenter") != 0)
            o << L"    " << N << L"->TextAlign = " << EmitContentAlignment(v) << L";\n"; }
    }
    // TabControl: Text satırları → Pages
    if (isTabCtrl) {
        int pages = 0;
        for (auto& line : Split(txt, L'\n')) {
            std::wstring t = Trim(line);
            if (!t.empty()) { o << L"    " << N << L"->Pages->Add(L\"" << EscCppStr(t) << L"\");\n"; pages++; }
        }
        if (pages == 0) o << L"    " << N << L"->Pages->Add(L\"tabPage1\");\n";
    }
    // StatusStrip: Text satırları → Parts
    if (T == L"StatusStrip") {
        for (auto& line : Split(txt, L'\n')) {
            std::wstring t = Trim(line);
            if (!t.empty()) o << L"    " << N << L"->Parts->Add(L\"" << EscCppStr(t) << L"\");\n";
        }
    }
    // MenuStrip/ContextMenuStrip: Text satırları → Items (üst düzey)
    if (T == L"MenuStrip" || T == L"ToolStrip" || T == L"ContextMenuStrip") {
        for (auto& line : Split(txt, L'\n')) {
            std::wstring t = Trim(line);
            if (!t.empty()) o << L"    " << N << L"->Items->Add(L\"" << EscCppStr(t) << L"\");\n";
        }
        if (T == L"MenuStrip" || T == L"ToolStrip")
            o << L"    " << N << L"->Apply();\n";
    }
    // konteyner bildirim köprüsü
    if (IsContainerType(c->type))
        o << L"    SetWindowSubclass(" << N << L"->Handle, ContainerBridgeProc, 1, 0);\n";
    return o.str();
}

// HANDLES bloğu (cpp tanımları) — hem üretimde hem senkronda kullanılır
// Not: Timer her koşulda M2God::Timer; winClass'ı boş (sarmalayıcısı olmayan) bileşenler atlanır
static std::wstring EmitHandlesBlock(bool asExtern) {
    std::wostringstream h;
    for (auto* c : g_model.ctrls) {
        if (c->type == L"Timer") {
            h << (asExtern ? L"extern " : L"") << L"M2God::Timer* " << c->name << (asExtern ? L";\n" : L" = nullptr;\n");
            continue;
        }
        std::wstring wrapper = M2ComponentTypeOf(c->type);
        if (!wrapper.empty()) {
            // SDK sarmalayıcısı olan bileşen/diyalog (Faz 2.5)
        } else if (c->nonVisual) {
            continue;                        // sarmalayıcısı olmayan bileşen
        } else {
            factory::Win32Class mc = factory::MapClass(c->type);
            if (mc.cls.empty()) continue;
            wrapper = M2TypeOf(c->type);
        }
        h << (asExtern ? L"extern " : L"") << L"M2God::" << wrapper
          << L"* " << c->name << (asExtern ? L";\n" : L" = nullptr;\n");
    }
    return h.str();
}

// WM_COMMAND bildirim eşlemesi (Faz 2.5 — boş dönerse WM_NOTIFY/WM_HSCROLL'a bakılır)
static std::wstring CmdNotifyOf(const std::wstring& type, const std::wstring& evName) {
    if (evName == L"Click" || evName == L"CheckedChanged") {
        if (type == L"Label" || type == L"LinkLabel" || type == L"PictureBox") return L"STN_CLICKED";
        return L"BN_CLICKED";
    }
    if (evName == L"TextChanged") {
        if (type == L"ComboBox") return L"CBN_EDITCHANGE";
        return L"EN_CHANGE";
    }
    if (evName == L"SelectedIndexChanged") {
        if (type == L"ComboBox") return L"CBN_SELCHANGE";
        if (type == L"ListBox") return L"LBN_SELCHANGE";
        return L"";   // ListView/CheckedListBox/TabControl → WM_NOTIFY
    }
    if (evName == L"DoubleClick") {
        if (type == L"ListBox") return L"LBN_DBLCLK";
        if (type == L"ComboBox") return L"CBN_DBLCLK";
        if (type == L"Label" || type == L"LinkLabel" || type == L"PictureBox") return L"STN_DBLCLK";
        return L"BN_DOUBLECLICKED";
    }
    if (evName == L"ValueChanged") {
        if (type == L"NumericUpDown") return L"EN_CHANGE";
        return L"";
    }
    return L"";
}

std::wstring GenerateForm1Cpp() {
    std::wostringstream o;
    o << L"// Form1.cpp — M2GodWin32 UI Designer (Faz 2.5) tarafından üretilmiştir.\n";
    o << L"// Nesne sözdizimi için Global.h: timer1->Enabled = false; button1->Text = L\"...\";\n";
    o << L"// Kapsam: tüm kontroller + bileşenler + diyaloglar (M2God SDK v2501, C++14 uyumlu)\n";
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
    o << L"    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_DATE_CLASSES };\n";
    o << L"    InitCommonControlsEx(&icc);\n";
    o << L"}\n\n";
    // HANDLES
    o << L"//== M2GOD:HANDLES-BEGIN\n";
    o << L"// Not: kontroller M2God nesneleridir (Global.h). HWND için: button1->Handle\n";
    o << EmitHandlesBlock(false);
    o << L"//== M2GOD:HANDLES-END\n\n";
    // CREATE
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
            if (c->nonVisual || c->type == L"Timer") continue; // bileşenler sonda üretilir
            o << EmitCreateCall(c) << L"\n";
            emitBranch(c);
        }
    };
    emitBranch(nullptr);
    for (auto* c : g_model.ctrls) if (c->nonVisual && c->type != L"Timer") o << EmitCreateCall(c) << L"\n";
    for (auto* c : g_model.ctrls) if (c->type == L"Timer") o << EmitCreateCall(c) << L"\n";
    o << L"    M2God::InitDone() = true;   // bildirim yonlendirme artik acik\n";
    o << L"}\n\n";
    // CleanupControls — WM_DESTROY'da timer durdurma + nesne serbest bırakma
    o << L"void CleanupControls() {\n";
    o << L"    // Timer'lar önce durdurulur\n";
    for (auto* c : g_model.ctrls)
        if (c->type == L"Timer")
            o << L"    if (" << c->name << L") { " << c->name << L"->Stop(); delete " << c->name << L"; " << c->name << L" = nullptr; }\n";
    for (int i = (int)g_model.ctrls.size() - 1; i >= 0; i--) {
        Ctrl* c = g_model.ctrls[i];
        if (c->type == L"Timer") continue;
        if (M2ComponentTypeOf(c->type).empty()) {
            factory::Win32Class mc = factory::MapClass(c->type);
            if (mc.cls.empty() || c->nonVisual) continue;
        }
        o << L"    if (" << c->name << L") { delete " << c->name << L"; " << c->name << L" = nullptr; }\n";
    }
    o << L"}\n";
    o << L"//== M2GOD:CREATE-END\n\n";
    // EVENTS
    o << L"//== M2GOD:EVENTS-BEGIN  (olay işleyicileri; mevcut gövdeler senkronda korunur)\n";
    std::vector<std::wstring> emitted;
    auto emitStub = [&](const std::wstring& hname) {
        for (auto& e : emitted) if (_wcsicmp(e.c_str(), hname.c_str()) == 0) return;
        emitted.push_back(hname);
        o << L"void " << hname << L"(HWND hWnd) {\n";
        o << L"    // TODO: " << hname << L" — kodunuzu buraya yazın (timer1->Enabled = false; gibi)\n";
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
    o << L"    static bool s_m2Shown = false;\n";
    o << L"    switch (message) {\n";
    o << L"    case WM_CREATE:\n";
    o << L"        M2GodForm::InitializeControls(hWnd);\n";
    if (g_model.formEvents.count(L"Load"))
        o << L"        M2GodForm::" << g_model.formEvents[L"Load"] << L"(hWnd);\n";
    else
        o << L"        // Form1_Load burada çağrılır (Olaylar sekmesinden ekleyin)\n";
    o << L"        return 0;\n";
    // WM_COMMAND router (Faz 2.5 genişletilmiş bildirimler)
    o << L"    case WM_COMMAND: {\n";
    o << L"        if (!M2God::InitDone()) break;   // yaratim sirasinda bildirim yok\n";
    o << L"        int id = LOWORD(wParam), ev = HIWORD(wParam);\n";
    o << L"        switch (id) {\n";
    for (auto* c : g_model.ctrls) {
        if (c->events.empty() || c->type == L"Timer") continue;
        if (c->nonVisual) continue;
        bool routed = false;
        std::wostringstream br;
        for (auto& kv : c->events) {
            std::wstring notify = CmdNotifyOf(c->type, kv.first);
            if (notify.empty()) continue;
            routed = true;
            br << L"            if (ev == " << notify << L" && M2GodForm::" << c->name << L") M2GodForm::" << kv.second << L"(hWnd);\n";
        }
        if (routed) {
            o << L"        case M2GodForm::" << IdMacroOf(c->name) << L":\n";
            o << br.str();
            o << L"            break;\n";
        }
    }
    o << L"        }\n";
    // MenuStrip/ToolStrip komut dağıtımı
    for (auto* c : g_model.ctrls)
        if (c->type == L"MenuStrip" || c->type == L"ToolStrip")
            o << L"        if (M2GodForm::" << c->name << L") M2GodForm::" << c->name << L"->DispatchCommand((UINT)id);\n";
    o << L"        break;\n";
    o << L"    }\n";
    // WM_TIMER — gerçek timer dispatch (çoklu timer tek case içinde)
    {
        std::wostringstream timerCases;
        for (auto* c : g_model.ctrls) {
            if (c->type != L"Timer") continue;
            auto ev = c->events.find(L"Tick");
            if (ev == c->events.end()) continue;
            timerCases << L"        if (wParam == M2GodForm::" << IdMacroOf(c->name) << L") {\n";
            timerCases << L"            if (M2GodForm::" << c->name << L" && M2GodForm::" << c->name << L"->Enabled) M2GodForm::" << ev->second << L"(hWnd);\n";
            timerCases << L"            return 0;\n";
            timerCases << L"        }\n";
        }
        if (!timerCases.str().empty()) {
            o << L"    case WM_TIMER:\n";
            o << L"        if (M2God::InitDone()) {\n";
            o << timerCases.str();
            o << L"        }\n";
            o << L"        break;\n";
        }
    }
    // WM_NOTIFY — ListView/CheckedListBox/TreeView/TabControl/DateTimePicker (Faz 2.5)
    {
        std::wostringstream nt;
        bool any = false;
        for (auto* c : g_model.ctrls) {
            if (c->nonVisual || c->events.empty()) continue;
            std::vector<std::pair<std::wstring, std::wstring>> lines; // (koşul-ek, handler)
            auto has = [&](const wchar_t* ev) -> std::wstring {
                auto it = c->events.find(ev);
                return it == c->events.end() ? std::wstring() : it->second;
            };
            if (c->type == L"ListView" || c->type == L"CheckedListBox") {
                if (has(L"SelectedIndexChanged").size()) lines.push_back({ L"LVN_ITEMCHANGED", has(L"SelectedIndexChanged") });
                if (has(L"ItemCheck").size())            lines.push_back({ L"LVN_ITEMCHANGED", has(L"ItemCheck") });
                if (has(L"Click").size())                lines.push_back({ L"NM_CLICK", has(L"Click") });
                if (has(L"DoubleClick").size())          lines.push_back({ L"NM_DBLCLK", has(L"DoubleClick") });
            } else if (c->type == L"TreeView") {
                if (has(L"AfterSelect").size())          lines.push_back({ L"TVN_SELCHANGED", has(L"AfterSelect") });
                if (has(L"SelectedIndexChanged").size()) lines.push_back({ L"TVN_SELCHANGED", has(L"SelectedIndexChanged") });
                if (has(L"Click").size())                lines.push_back({ L"NM_CLICK", has(L"Click") });
                if (has(L"DoubleClick").size())          lines.push_back({ L"NM_DBLCLK", has(L"DoubleClick") });
            } else if (c->type == L"TabControl") {
                if (has(L"SelectedIndexChanged").size()) lines.push_back({ L"TCN_SELCHANGE", has(L"SelectedIndexChanged") });
            } else if (c->type == L"DateTimePicker") {
                if (has(L"ValueChanged").size())         lines.push_back({ L"DTN_DATETIMECHANGE", has(L"ValueChanged") });
            }
            if (lines.empty()) continue;
            any = true;
            nt << L"        case M2GodForm::" << IdMacroOf(c->name) << L":\n";
            for (auto& ln : lines) {
                if (ln.first == L"LVN_ITEMCHANGED")
                    nt << L"            if (nm->code == LVN_ITEMCHANGED && (((LPNMLISTVIEW)lParam)->uChanged & LVIF_STATE) && M2GodForm::" << c->name << L") M2GodForm::" << ln.second << L"(hWnd);\n";
                else
                    nt << L"            if (nm->code == " << ln.first << L" && M2GodForm::" << c->name << L") M2GodForm::" << ln.second << L"(hWnd);\n";
            }
            nt << L"            break;\n";
        }
        if (any) {
            o << L"    case WM_NOTIFY: {\n";
            o << L"        if (!M2God::InitDone()) break;\n";
            o << L"        NMHDR* nm = (NMHDR*)lParam;\n";
            o << L"        switch (nm->idFrom) {\n";
            o << nt.str();
            o << L"        }\n";
            o << L"        break;\n";
            o << L"    }\n";
        }
    }
    // WM_HSCROLL / WM_VSCROLL — TrackBar ve ScrollBar'lar (Faz 2.5)
    {
        std::wostringstream hs;
        bool any = false;
        for (auto* c : g_model.ctrls) {
            if (c->nonVisual || c->events.empty()) continue;
            if (c->type != L"TrackBar" && c->type != L"HScrollBar" && c->type != L"VScrollBar") continue;
            if (c->type == L"VScrollBar") continue;   // dikey olanlar VSCROLL'a
            auto sc = c->events.find(L"Scroll");
            auto vc = c->events.find(L"ValueChanged");
            if (sc == c->events.end() && vc == c->events.end()) continue;
            any = true;
            hs << L"        if ((HWND)lParam == (HWND)(M2GodForm::" << c->name << L" ? M2GodForm::" << c->name << L"->Handle : nullptr)) {\n";
            if (vc != c->events.end()) hs << L"            M2GodForm::" << vc->second << L"(hWnd);\n";
            if (sc != c->events.end()) hs << L"            M2GodForm::" << sc->second << L"(hWnd);\n";
            hs << L"            return 0;\n";
            hs << L"        }\n";
        }
        if (any) {
            o << L"    case WM_HSCROLL:\n";
            o << L"        if (M2God::InitDone()) {\n" << hs.str();
            o << L"        }\n";
            o << L"        break;\n";
        }
    }
    {
        std::wostringstream vs;
        bool any = false;
        for (auto* c : g_model.ctrls) {
            if (c->nonVisual || c->events.empty()) continue;
            if (c->type != L"VScrollBar") continue;
            auto sc = c->events.find(L"Scroll");
            auto vc = c->events.find(L"ValueChanged");
            if (sc == c->events.end() && vc == c->events.end()) continue;
            any = true;
            vs << L"        if ((HWND)lParam == (HWND)(M2GodForm::" << c->name << L" ? M2GodForm::" << c->name << L"->Handle : nullptr)) {\n";
            if (vc != c->events.end()) vs << L"            M2GodForm::" << vc->second << L"(hWnd);\n";
            if (sc != c->events.end()) vs << L"            M2GodForm::" << sc->second << L"(hWnd);\n";
            vs << L"            return 0;\n";
            vs << L"        }\n";
        }
        if (any) {
            o << L"    case WM_VSCROLL:\n";
            o << L"        if (M2God::InitDone()) {\n" << vs.str();
            o << L"        }\n";
            o << L"        break;\n";
        }
    }
    // Form düzeyi olaylar (Faz 2.5) — sadece ekliyse üretilir
    auto hasForm = [&](const wchar_t* ev) -> std::wstring {
        auto it = g_model.formEvents.find(ev);
        return it == g_model.formEvents.end() ? std::wstring() : it->second;
    };
    if (hasForm(L"Shown").size()) {
        o << L"    case WM_SHOWWINDOW:\n";
        o << L"        if (wParam && !s_m2Shown) { s_m2Shown = true; M2GodForm::" << hasForm(L"Shown") << L"(hWnd); }\n";
        o << L"        break;\n";
    }
    if (hasForm(L"Resize").size()) {
        o << L"    case WM_SIZE:\n";
        o << L"        if (wParam != SIZE_MINIMIZED) M2GodForm::" << hasForm(L"Resize") << L"(hWnd);\n";
        o << L"        break;\n";
    }
    if (hasForm(L"FormClosing").size()) {
        o << L"    case WM_CLOSE:\n";
        o << L"        M2GodForm::" << hasForm(L"FormClosing") << L"(hWnd);\n";
        o << L"        break;\n";
    }
    if (hasForm(L"Activated").size()) {
        o << L"    case WM_ACTIVATE:\n";
        o << L"        if (LOWORD(wParam) != WA_INACTIVE) M2GodForm::" << hasForm(L"Activated") << L"(hWnd);\n";
        o << L"        break;\n";
    }
    if (hasForm(L"MouseMove").size()) {
        o << L"    case WM_MOUSEMOVE:\n        M2GodForm::" << hasForm(L"MouseMove") << L"(hWnd);\n        break;\n";
    }
    if (hasForm(L"MouseDown").size()) {
        o << L"    case WM_LBUTTONDOWN:\n        M2GodForm::" << hasForm(L"MouseDown") << L"(hWnd);\n        break;\n";
    }
    if (hasForm(L"MouseUp").size() || hasForm(L"Click").size()) {
        o << L"    case WM_LBUTTONUP:\n";
        if (hasForm(L"MouseUp").size())  o << L"        M2GodForm::" << hasForm(L"MouseUp") << L"(hWnd);\n";
        if (hasForm(L"Click").size())    o << L"        M2GodForm::" << hasForm(L"Click") << L"(hWnd);\n";
        o << L"        break;\n";
    }
    if (hasForm(L"KeyDown").size()) {
        o << L"    case WM_KEYDOWN:\n        M2GodForm::" << hasForm(L"KeyDown") << L"(hWnd);\n        break;\n";
    }
    if (hasForm(L"KeyUp").size()) {
        o << L"    case WM_KEYUP:\n        M2GodForm::" << hasForm(L"KeyUp") << L"(hWnd);\n        break;\n";
    }
    if (hasForm(L"KeyPress").size()) {
        o << L"    case WM_CHAR:\n        M2GodForm::" << hasForm(L"KeyPress") << L"(hWnd);\n        break;\n";
    }
    // NotifyIcon / BackgroundWorker dağıtımı (Faz 2.5)
    for (auto* c : g_model.ctrls)
        if (c->type == L"NotifyIcon")
            o << L"    case WM_APP + 0x4E1:\n        if (M2GodForm::" << c->name << L") M2GodForm::" << c->name << L"->Dispatch(lParam);\n        return 0;\n";
    for (auto* c : g_model.ctrls)
        if (c->type == L"BackgroundWorker") {
            o << L"    case WM_APP + 0x4E2:\n        if (M2GodForm::" << c->name << L") M2GodForm::" << c->name << L"->DispatchProgress((UINT)wParam);\n        return 0;\n";
            o << L"    case WM_APP + 0x4E3:\n        if (M2GodForm::" << c->name << L") M2GodForm::" << c->name << L"->DispatchCompleted();\n        return 0;\n";
        }
    // WM_CTLCOLOR* — BackColor/ForeColor uygulanır
    o << L"    case WM_CTLCOLORSTATIC:\n";
    o << L"    case WM_CTLCOLOREDIT:\n";
    o << L"    case WM_CTLCOLORLISTBOX:\n";
    o << L"    case WM_CTLCOLORBTN:\n";
    o << L"    case WM_CTLCOLORSCROLLBAR: {\n";
    o << L"        LRESULT r = M2God::HandleCtlColor((HWND)lParam, (HDC)wParam);\n";
    o << L"        if (r) return r;\n";
    o << L"        break;\n";
    o << L"    }\n";
    o << L"    case WM_DESTROY:\n";
    if (hasForm(L"FormClosed").size())
        o << L"        M2GodForm::" << hasForm(L"FormClosed") << L"(hWnd);\n";
    o << L"        M2GodForm::CleanupControls();\n";
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
    o << L"    M2God::Form::Handle = hWnd;\n";
    // Faz 2.5: form özellikleri (tasarımdan)
    {
        auto fp = [&](const wchar_t* k) -> std::wstring {
            auto it = g_model.formProps.find(k);
            return it == g_model.formProps.end() ? std::wstring() : it->second;
        };
        if (_wcsicmp(fp(L"MaximizeBox").c_str(), L"false") == 0)
            o << L"    M2God::Form::MaximizeBox = false;\n";
        if (_wcsicmp(fp(L"MinimizeBox").c_str(), L"false") == 0)
            o << L"    M2God::Form::MinimizeBox = false;\n";
        if (_wcsicmp(fp(L"ControlBox").c_str(), L"false") == 0)
            o << L"    M2God::Form::ControlBox = false;\n";
        {
            std::wstring v = fp(L"FormBorderStyle");
            const wchar_t* val = nullptr;
            if (_wcsicmp(v.c_str(), L"None") == 0) val = L"M2God::FormBorderStyle::None";
            else if (_wcsicmp(v.c_str(), L"FixedSingle") == 0) val = L"M2God::FormBorderStyle::FixedSingle";
            else if (_wcsicmp(v.c_str(), L"Fixed3D") == 0) val = L"M2God::FormBorderStyle::Fixed3D";
            else if (_wcsicmp(v.c_str(), L"FixedDialog") == 0) val = L"M2God::FormBorderStyle::FixedDialog";
            else if (_wcsicmp(v.c_str(), L"FixedToolWindow") == 0) val = L"M2God::FormBorderStyle::FixedToolWindow";
            else if (_wcsicmp(v.c_str(), L"SizableToolWindow") == 0) val = L"M2God::FormBorderStyle::SizableToolWindow";
            if (val) o << L"    M2God::Form::FormBorderStyle = " << val << L";\n";
        }
        if (_wcsicmp(fp(L"TopMost").c_str(), L"true") == 0)
            o << L"    M2God::Form::TopMost = true;\n";
        {
            std::wstring v = fp(L"WindowState");
            if (_wcsicmp(v.c_str(), L"Maximized") == 0) o << L"    M2God::Form::WindowState = M2God::FormWindowState::Maximized;\n";
            else if (_wcsicmp(v.c_str(), L"Minimized") == 0) o << L"    M2God::Form::WindowState = M2God::FormWindowState::Minimized;\n";
        }
        if (_wcsicmp(fp(L"StartPosition").c_str(), L"CenterScreen") == 0)
            o << L"    M2God::Form::CenterToScreen();\n";
    }
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
    o << L"// Form1.h — M2GodWin32 UI Designer (Faz 2.5) tarafından üretilmiştir.\n";
    o << L"#pragma once\n";
    o << L"#include <windows.h>\n";
    o << L"//== M2GOD:INCLUDE-BEGIN\n";
    o << L"#include \"Global.h\"\n";
    o << L"//== M2GOD:INCLUDE-END\n\n";
    o << L"namespace M2GodForm {\n\n";
    // IDS — Timer bileşenleri dahil (WM_TIMER dispatch id'si olarak kullanılır)
    o << L"//== M2GOD:IDS-BEGIN\n";
    o << L"enum {\n";
    bool first = true;
    for (auto* c : g_model.ctrls) {
        if (c->nonVisual && c->type != L"Timer") continue;
        if (!first) o << L",\n";
        o << L"    " << IdMacroOf(c->name) << L" = " << (3000 + (UINT)(c->cid - 3000));
        first = false;
    }
    o << L"\n};\n";
    o << L"//== M2GOD:IDS-END\n\n";
    o << L"//== M2GOD:HANDLES-BEGIN\n";
    o << L"// Not: kontroller M2God nesneleridir (Global.h). HWND için: button1->Handle\n";
    o << EmitHandlesBlock(true);
    o << L"//== M2GOD:HANDLES-END\n\n";
    o << L"//== M2GOD:PROTO-BEGIN\n";
    o << L"void InitializeControls(HWND hWnd);\n";
    o << L"void CleanupControls();\n";
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

// Global.h dosyasını garanti et: yoksa veya sürüm eskiyse üzerine yaz
static void EnsureGlobalHFile(const std::wstring& cppPath) {
    std::wstring dir = GetFileDir(cppPath);
    if (dir.empty()) return;
    std::wstring gpath = dir + L"\\Global.h";
    std::string cur;
    if (ReadFileText(gpath, cur)) {
        if (cur.find(GlobalVersionMark()) != std::string::npos) return; // güncel
    }
    WriteFileText(gpath, WideToUtf8(GenerateGlobalH()));
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
    // HANDLES bölgesi: M2God nesne tanımları (Faz 2.4 stili — eski HWND stili burada onarılır)
    ReplaceRegion(cpp, L"//== M2GOD:HANDLES-BEGIN", L"//== M2GOD:HANDLES-END", EmitHandlesBlock(false));
    // CREATE bölgesi: fonksiyon gövdesini yeniden üret
    std::wstring fresh = GenerateForm1Cpp();
    size_t cb = fresh.find(L"//== M2GOD:CREATE-BEGIN");
    size_t ce = fresh.find(L"//== M2GOD:CREATE-END");
    if (cb != std::wstring::npos && ce != std::wstring::npos) {
        std::wstring createBody = fresh.substr(cb + wcslen(L"//== M2GOD:CREATE-BEGIN"), ce - (cb + wcslen(L"//== M2GOD:CREATE-BEGIN")));
        ReplaceRegion(cpp, L"//== M2GOD:CREATE-BEGIN", L"//== M2GOD:CREATE-END", createBody);
    }
    // WNDPROC bölgesi de tazelenir: WM_TIMER/WM_CTLCOLOR/WM_COMMAND yönlendirmesi güncel
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
        // eski (Faz 2.3 ve öncesi) header'da INCLUDE bölgesi yoksa Global.h include'unu garanti et
        if (h.find(L"#include \"Global.h\"") == std::wstring::npos) {
            size_t p = h.find(L"#pragma once");
            if (p != std::wstring::npos) {
                p += wcslen(L"#pragma once");
                h.insert(p, L"\n#include \"Global.h\"");
            }
        }
        WriteFileText(hPath, WideToUtf8(h));
    }
    // Global.h: yoksa veya sürüm eskiyse yaz (Faz 2.4)
    EnsureGlobalHFile(cppPath);
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
    std::wstring gh = folder + L"\\Global.h";
    if (!WriteFileText(cpp, WideToUtf8(GenerateForm1Cpp()))) return false;
    if (!WriteFileText(h, WideToUtf8(GenerateForm1H()))) return false;
    WriteFileText(gh, WideToUtf8(GenerateGlobalH()));
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
