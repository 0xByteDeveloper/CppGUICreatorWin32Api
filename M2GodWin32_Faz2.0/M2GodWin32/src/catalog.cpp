// catalog.cpp — Config_info.json yükleyici + yerleşik yedek katalog
#include "catalog.h"
#include "app.h"
#include "json.h"

Catalog g_catalog;

std::wstring Catalog::EnumShortName(const std::wstring& full) {
    size_t p = full.find_last_of(L'.');
    return p == std::wstring::npos ? full : full.substr(p + 1);
}

void Catalog::MergeElement(const ElementDef& e) {
    elements[e.name] = e;
}

// Config_info.json yüklenemezse çalışan minimum yerleşik katalog
void Catalog::AddBuiltin() {
    struct B { const wchar_t *name, *cls, *cat, *dev, *txt; int w, h; };
    static const B builtins[] = {
        { L"Button", L"BUTTON", L"Ortak Denetimler", L"Click", L"button1", 100, 32 },
        { L"Label", L"STATIC", L"Ortak Denetimler", L"Click", L"label1", 100, 23 },
        { L"TextBox", L"EDIT", L"Ortak Denetimler", L"TextChanged", L"textBox1", 140, 26 },
        { L"CheckBox", L"BUTTON", L"Ortak Denetimler", L"CheckedChanged", L"checkBox1", 120, 28 },
        { L"RadioButton", L"BUTTON", L"Ortak Denetimler", L"CheckedChanged", L"radioButton1", 130, 28 },
        { L"ComboBox", L"COMBOBOX", L"Ortak Denetimler", L"SelectedIndexChanged", L"comboBox1", 150, 28 },
        { L"ListBox", L"LISTBOX", L"Ortak Denetimler", L"SelectedIndexChanged", L"listBox1", 140, 120 },
        { L"CheckedListBox", L"LISTBOX", L"Ortak Denetimler", L"SelectedIndexChanged", L"checkedListBox1", 140, 120 },
        { L"GroupBox", L"BUTTON", L"Kapsayıcılar", L"Click", L"groupBox1", 200, 120 },
        { L"Panel", L"M2GOD_PANEL", L"Kapsayıcılar", L"Click", L"panel1", 180, 120 },
        { L"PictureBox", L"M2GOD_PICTURE", L"Ortak Denetimler", L"Click", L"pictureBox1", 120, 100 },
        { L"ProgressBar", L"msctls_progress32", L"Ortak Denetimler", L"Click", L"progressBar1", 160, 26 },
        { L"TrackBar", L"msctls_trackbar32", L"Ortak Denetimler", L"Scroll", L"trackBar1", 160, 40 },
        { L"NumericUpDown", L"EDIT", L"Ortak Denetimler", L"ValueChanged", L"numericUpDown1", 100, 26 },
        { L"DateTimePicker", L"SysDateTimePick32", L"Ortak Denetimler", L"ValueChanged", L"dateTimePicker1", 160, 28 },
        { L"MonthCalendar", L"SysMonthCal32", L"Ortak Denetimler", L"DateChanged", L"monthCalendar1", 230, 180 },
        { L"ListView", L"SysListView32", L"Ortak Denetimler", L"SelectedIndexChanged", L"listView1", 180, 120 },
        { L"TreeView", L"SysTreeView32", L"Ortak Denetimler", L"AfterSelect", L"treeView1", 180, 140 },
        { L"TabControl", L"SysTabControl32", L"Kapsayıcılar", L"SelectedIndexChanged", L"tabControl1", 220, 150 },
        { L"RichTextBox", L"RichEdit20W", L"Ortak Denetimler", L"TextChanged", L"richTextBox1", 180, 100 },
        { L"MaskedTextBox", L"EDIT", L"Ortak Denetimler", L"TextChanged", L"maskedTextBox1", 140, 26 },
        { L"WebBrowser", L"M2GOD_PLACEHOLDER", L"Ortak Denetimler", L"Click", L"webBrowser1", 220, 150 },
        { L"MenuStrip", L"M2GOD_PLACEHOLDER", L"Menüler ve Araç Çubukları", L"Click", L"menuStrip1", 200, 28 },
        { L"StatusStrip", L"msctls_statusbar32", L"Menüler ve Araç Çubukları", L"Click", L"statusStrip1", 200, 24 },
        { L"Timer", L"", L"Bileşenler", L"Tick", L"timer1", 32, 32 },
        { L"BackgroundWorker", L"", L"Bileşenler", L"DoWork", L"backgroundWorker1", 32, 32 },
    };
    for (auto& b : builtins) {
        ElementDef e;
        e.name = b.name; e.kind = (b.cls[0] == 0) ? L"component" : L"control";
        e.defaultEvent = b.dev; e.categoryTr = b.cat; e.winClass = b.cls;
        e.toolboxVisible = true; e.defaultProperty = L"Text";
        // temel özellikler
        PropDef p;
        p.name = L"Name"; p.type = L"System.String"; p.category = L"Design"; p.canSet = true; p.browsable = true; p.def = b.txt;
        e.props.push_back(p);
        p.name = L"Text"; p.def = b.txt; p.category = L"Appearance"; e.props.push_back(p);
        p.name = L"Location"; p.type = L"System.Drawing.Point"; p.category = L"Layout"; p.def = L"20, 20"; e.props.push_back(p);
        p.name = L"Size"; p.type = L"System.Drawing.Size"; p.category = L"Layout"; p.def = L"100, 23"; e.props.push_back(p);
        p.name = L"Enabled"; p.type = L"System.Boolean"; p.def = L"True"; e.props.push_back(p);
        p.name = L"Visible"; p.type = L"System.Boolean"; p.def = L"True"; e.props.push_back(p);
        p.name = L"Font"; p.type = L"System.Drawing.Font"; p.def = L"Segoe UI, 9pt"; e.props.push_back(p);
        p.name = L"ForeColor"; p.def = L"ControlText"; e.props.push_back(p);
        p.name = L"BackColor"; p.def = L"Control"; e.props.push_back(p);
        e.events.push_back({ L"Click", L"System.EventHandler", L"Action", true });
        e.events.push_back({ b.dev, L"System.EventHandler", L"Action", true });
        elements[e.name] = e;
    }
    // FAZ 2.6: TabPage — AddBuiltin yolunda da gerekli (Config yoksa)
    if (!elements.count(L"TabPage")) {
        ElementDef tp;
        tp.name = L"TabPage"; tp.kind = L"container"; tp.base = L"Panel";
        tp.defaultEvent = L"Click"; tp.defaultProperty = L"Text";
        tp.categoryTr = L"Kapsayıcılar"; tp.toolboxVisible = false; tp.winClass = L"M2GOD_TABPAGE";
        tp.props.push_back({ L"Name", L"System.String", L"Design", L"tabPage1", L"", true, true });
        tp.props.push_back({ L"Text", L"System.String", L"Appearance", L"tabPage1", L"", true, true });
        tp.props.push_back({ L"Location", L"System.Drawing.Point", L"Layout", L"0, 0", L"", true, false });
        tp.props.push_back({ L"Size", L"System.Drawing.Size", L"Layout", L"0, 0", L"", true, false });
        tp.props.push_back({ L"BackColor", L"System.Drawing.Color", L"Appearance", L"Control", L"", true, true });
        tp.props.push_back({ L"ForeColor", L"System.Drawing.Color", L"Appearance", L"ControlText", L"", true, true });
        tp.props.push_back({ L"Font", L"System.Drawing.Font", L"Appearance", L"Segoe UI, 9pt", L"", true, true });
        tp.props.push_back({ L"Enabled", L"System.Boolean", L"Behavior", L"True", L"", true, true });
        tp.props.push_back({ L"Visible", L"System.Boolean", L"Behavior", L"True", L"", true, true });
        tp.props.push_back({ L"Tag", L"System.String", L"Data", L"", L"", true, true });
        tp.props.push_back({ L"ToolTipText", L"System.String", L"Behavior", L"", L"", true, true });
        for (auto& evn : { L"Click", L"TextChanged" })
            tp.events.push_back({ evn, L"System.EventHandler", L"", true });
        elements[L"TabPage"] = tp;
    }

    categories = { L"Ortak Denetimler", L"Kapsayıcılar", L"Bileşenler" };
    for (auto& kv : elements) catElements[kv.second.categoryTr].push_back(kv.first);
}

bool Catalog::Load(const std::wstring& configPath) {
    std::string data;
    bool loaded = false;
    if (!configPath.empty() && ReadFileText(configPath, data)) {
        bool ok = false;
        json::Value root = json::Parse(data, &ok);
        if (ok && root.IsObj()) {
            // 1) kategoriler
            const json::Value& tl = root.Get(L"toolbox_layout");
            if (tl.IsObj()) {
                for (auto& kv : tl.obj) {
                    categories.push_back(kv.first);
                    std::wstring tr = kv.first;
                    if (kv.second.IsObj() && kv.second.Has(L"category_tr")) tr = kv.second.Get(L"category_tr").AsStr();
                    if (kv.second.IsObj()) {
                        const json::Value& els = kv.second.Get(L"elements");
                        std::vector<std::wstring> list;
                        if (els.IsArr()) for (auto& e : els.arr) list.push_back(e.AsStr());
                        catElements[tr] = list;
                    }
                }
            }
            // 2) elemanlar
            const json::Value& els = root.Get(L"elements");
            if (els.IsArr()) {
                for (auto& el : els.arr) {
                    ElementDef e;
                    e.name = el.Get(L"name").AsStr();
                    if (e.name.empty()) continue;
                    e.kind = el.Get(L"kind").AsStr(L"control");
                    e.base = el.Get(L"base").AsStr();
                    e.defaultEvent = el.Get(L"default_event").AsStr(L"Click");
                    e.defaultProperty = el.Get(L"default_property").AsStr(L"Text");
                    const json::Value& tb = el.Get(L"toolbox");
                    e.toolboxVisible = tb.Get(L"visible").AsBool(true);
                    e.categoryTr = tb.Get(L"category_tr").AsStr(L"Diğer");
                    const json::Value& wm = el.Get(L"win32_mapping");
                    e.winClass = wm.Get(L"window_class").AsStr();
                    // özellikler
                    const json::Value& pr = el.Get(L"properties");
                    if (pr.IsArr()) {
                        for (auto& pj : pr.arr) {
                            PropDef p;
                            p.name = pj.Get(L"name").AsStr();
                            if (p.name.empty()) continue;
                            p.type = pj.Get(L"type").AsStr();
                            p.category = pj.Get(L"category").AsStr(L"Misc");
                            p.def = pj.Get(L"default").AsStr();
                            p.enumType = pj.Get(L"enum_type").AsStr();
                            p.browsable = pj.Get(L"browsable").AsBool(false);
                            p.canSet = pj.Get(L"can_set").AsBool(false);
                            e.props.push_back(p);
                        }
                    }
                    // olaylar
                    const json::Value& ev = el.Get(L"events");
                    if (ev.IsArr()) {
                        for (auto& ej : ev.arr) {
                            EventDef d;
                            if (ej.IsStr()) { d.name = ej.AsStr(); d.browsable = true; }
                            else {
                                d.name = ej.Get(L"name").AsStr();
                                d.handlerType = ej.Get(L"handler_type").AsStr();
                                d.category = ej.Get(L"category").AsStr();
                                d.browsable = ej.Get(L"browsable").AsBool(true);
                            }
                            if (!d.name.empty() && !e.HasEvent(d.name)) e.events.push_back(d);
                        }
                    }
                    MergeElement(e);
                    loaded = true;
                }
            }
            // 3) enumlar
            const json::Value& en = root.Get(L"enums");
            if (en.IsObj()) {
                for (auto& kv : en.obj) {
                    std::vector<std::pair<std::wstring, int>> vals;
                    const json::Value& vs = kv.second.Get(L"values");
                    if (vs.IsObj())
                        for (auto& vp : vs.obj) vals.push_back({ vp.first, (int)vp.second.AsNum() });
                    enums[kv.first] = vals;
                }
            }
        }
    }
    if (!loaded) { AddBuiltin(); return false; }

    // Faz 2.5: bileşenlerin winClass'ını temizle — nonVisual=true olur
    // (tasarımda tepsiye düşer; üretilen kodda SDK sarmalayıcısı kullanılır)
    static const wchar_t* kComps[] = {
        L"Timer", L"BackgroundWorker", L"BindingSource", L"BindingNavigator",
        L"FileSystemWatcher", L"SerialPort", L"Process", L"DirectoryEntry",
        L"DirectorySearcher", L"EventLog", L"MessageQueue", L"PerformanceCounter",
        L"ServiceController", L"ErrorProvider", L"HelpProvider", L"ImageList",
        L"NotifyIcon", L"OpenFileDialog", L"SaveFileDialog", L"FolderBrowserDialog",
        L"FontDialog", L"ColorDialog", L"PrintDialog", L"PrintPreviewDialog",
        L"PageSetupDialog", L"DataSet", L"DataView", L"DataColumn", L"DataTable",
    };
    for (auto& kv : elements) {
        for (auto* cp : kComps) {
            if (_wcsicmp(kv.first.c_str(), cp) == 0) { kv.second.winClass.clear(); break; }
        }
    }

    // Timer bileşenine Interval/Enabled özellikleri garanti edilir (Faz 2.4:
    // üretilen kod SetTimer/WM_TIMER kullanır; panelde Interval/Enabled görünür)
    auto tit = elements.find(L"Timer");
    if (tit != elements.end()) {
        bool hasI = false, hasE = false;
        for (auto& p : tit->second.props) {
            if (_wcsicmp(p.name.c_str(), L"Interval") == 0) hasI = true;
            if (_wcsicmp(p.name.c_str(), L"Enabled") == 0) hasE = true;
        }
        if (!hasI) tit->second.props.push_back({ L"Interval", L"System.Int32", L"Behavior", L"1000", L"", true, true });
        if (!hasE) tit->second.props.push_back({ L"Enabled", L"System.Boolean", L"Behavior", L"False", L"", true, true });
    }

    // Timer bileşenine Interval/Enabled özellikleri garanti edilir (Faz 2.4:
    // üretilen kod SetTimer/WM_TIMER kullanır; panelde Interval/Enabled görünür)

    // FAZ 2.6: TabPage yerleşik öğesi — TabControl sayfaları artık GERÇEK Ctrl
    // girişleridir (VS davranışı: sekme şeridine tıkla → sayfa seçilir, panelde
    // sayfa özellikleri görünür; sayfalar .m2god'a kaydedilir, runtime'da
    // M2God::TabPage gerçek pencere + TabPages->Add koleksiyonu olur)
    if (!elements.count(L"TabPage")) {
        ElementDef tp;
        tp.name = L"TabPage"; tp.kind = L"container"; tp.base = L"Panel";
        tp.defaultEvent = L"Click"; tp.defaultProperty = L"Text";
        tp.categoryTr = L"Kapsayıcılar"; tp.toolboxVisible = false; tp.winClass = L"M2GOD_TABPAGE";
        tp.props.push_back({ L"Name", L"System.String", L"Design", L"tabPage1", L"", true, true });
        tp.props.push_back({ L"Text", L"System.String", L"Appearance", L"tabPage1", L"", true, true });
        tp.props.push_back({ L"Location", L"System.Drawing.Point", L"Layout", L"0, 0", L"", true, false });
        tp.props.push_back({ L"Size", L"System.Drawing.Size", L"Layout", L"0, 0", L"", true, false });
        tp.props.push_back({ L"BackColor", L"System.Drawing.Color", L"Appearance", L"Control", L"", true, true });
        tp.props.push_back({ L"ForeColor", L"System.Drawing.Color", L"Appearance", L"ControlText", L"", true, true });
        tp.props.push_back({ L"Font", L"System.Drawing.Font", L"Appearance", L"Segoe UI, 9pt", L"", true, true });
        tp.props.push_back({ L"Enabled", L"System.Boolean", L"Behavior", L"True", L"", true, true });
        tp.props.push_back({ L"Visible", L"System.Boolean", L"Behavior", L"True", L"", true, true });
        tp.props.push_back({ L"Tag", L"System.String", L"Data", L"", L"", true, true });
        tp.props.push_back({ L"ToolTipText", L"System.String", L"Behavior", L"", L"", true, true });
        tp.props.push_back({ L"UseVisualStyleBackColor", L"System.Boolean", L"Appearance", L"True", L"", true, true });
        for (auto& evn : { L"Click", L"TextChanged", L"Enter", L"Leave" })
            tp.events.push_back({ evn, L"System.EventHandler", L"", true });
        MergeElement(tp);
    }

    // Form + temel kapsayıcı elemanlarının katalogda garanti edilmesi
    if (!elements.count(L"Form")) {
        ElementDef f;
        f.name = L"Form"; f.kind = L"form"; f.defaultEvent = L"Load"; f.categoryTr = L"Form";
        f.winClass = L"M2GOD_FORM"; f.toolboxVisible = false;
        f.props.push_back({ L"Name", L"System.String", L"Design", L"Form1", L"", true, true });
        f.props.push_back({ L"Text", L"System.String", L"Appearance", L"Form1", L"", true, true });
        f.props.push_back({ L"ClientSize", L"System.Drawing.Size", L"Layout", L"800, 600", L"", true, true });
        f.props.push_back({ L"BackColor", L"System.Drawing.Color", L"Appearance", L"Control", L"", true, true });
        f.props.push_back({ L"Font", L"System.Drawing.Font", L"Appearance", L"Segoe UI, 9pt", L"", true, true });
        f.props.push_back({ L"StartPosition", L"System.Windows.Forms.FormStartPosition", L"Layout", L"WindowsDefaultLocation", L"System.Windows.Forms.FormStartPosition", true, true });
        f.props.push_back({ L"MaximizeBox", L"System.Boolean", L"Window Style", L"True", L"", true, true });
        f.props.push_back({ L"MinimizeBox", L"System.Boolean", L"Window Style", L"True", L"", true, true });
        f.props.push_back({ L"FormBorderStyle", L"System.Windows.Forms.FormBorderStyle", L"Window Style", L"Sizable", L"System.Windows.Forms.FormBorderStyle", true, true });
        for (auto& evn : { L"Load", L"Paint", L"Resize", L"FormClosing", L"FormClosed", L"Shown", L"Activated", L"Click", L"MouseMove", L"MouseUp", L"MouseDown", L"KeyDown", L"KeyUp", L"KeyPress" })
            f.events.push_back({ evn, L"System.EventHandler", L"", true });
        elements[L"Form"] = f;
    }
    return true;
}
