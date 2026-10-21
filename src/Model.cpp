// ============================================================================
//  Model.cpp — catalog loading + serialization
// ============================================================================
#include "Model.h"

#ifndef _WIN32
// glibc shims so the portable core unit-tests compile natively on Linux
static inline int        _wtoi(const wchar_t* s)   { return (int)wcstol(s, nullptr, 10); }
static inline long long  _wtoi64(const wchar_t* s) { return wcstoll(s, nullptr, 10); }
#endif

// ===========================================================================
// Catalog
// ===========================================================================
namespace Catalog
{
    static std::vector<ControlMeta> g_all;
    static bool g_loaded = false;

    static ControlMeta FallbackMeta(const wchar_t* type, const wchar_t* cat,
                                    const wchar_t* defEv, const wchar_t* wcls)
    {
        ControlMeta m;
        m.type = type; m.category = cat; m.defaultEvent = defEv;
        m.win32Class = wcls; m.kind = L"control";
        m.events = { L"Click", L"DoubleClick", L"Paint", L"MouseMove", L"KeyDown" };
        m.props  = { L"BackColor", L"ForeColor", L"Enabled", L"Visible" };
        return m;
    }

    static void LoadBuiltins()
    {
        g_all.clear();
        g_all.push_back(FallbackMeta(L"Button",     L"Ortak Denetimler", L"Click",  L"BUTTON"));
        g_all.push_back(FallbackMeta(L"Label",      L"Ortak Denetimler", L"Click",  L"STATIC"));
        g_all.push_back(FallbackMeta(L"TextBox",    L"Ortak Denetimler", L"TextChanged", L"EDIT"));
        g_all.push_back(FallbackMeta(L"CheckBox",   L"Ortak Denetimler", L"CheckedChanged", L"BUTTON"));
        g_all.push_back(FallbackMeta(L"RadioButton",L"Ortak Denetimler", L"CheckedChanged", L"BUTTON"));
        g_all.push_back(FallbackMeta(L"ListBox",    L"Ortak Denetimler", L"SelectedIndexChanged", L"LISTBOX"));
        g_all.push_back(FallbackMeta(L"ComboBox",   L"Ortak Denetimler", L"SelectedIndexChanged", L"COMBOBOX"));
        g_all.push_back(FallbackMeta(L"PictureBox", L"Ortak Denetimler", L"Paint",  L"STATIC"));
        ControlMeta gb = FallbackMeta(L"GroupBox",  L"Kapsayıcılar",     L"Paint",  L"BUTTON");
        gb.kind = L"container"; g_all.push_back(gb);
        g_all.push_back(FallbackMeta(L"ProgressBar",L"Ortak Denetimler", L"Click",  L"msctls_progress32"));
        g_all.push_back(FallbackMeta(L"TrackBar",   L"Ortak Denetimler", L"Scroll", L"msctls_trackbar32"));
        g_all.push_back(FallbackMeta(L"DateTimePicker", L"Ortak Denetimler", L"ValueChanged", L"SysDateTimePick32"));
        ControlMeta pl = FallbackMeta(L"Panel",     L"Kapsayıcılar",     L"Paint",  L"custom");
        pl.kind = L"container"; g_all.push_back(pl);
        g_all.push_back(FallbackMeta(L"MenuStrip",  L"Menüler ve Araç Çubukları", L"ItemClicked", L"HMENU"));
        g_loaded = true;
    }

    bool LoadFromAssets()
    {
        // search order: exe_dir/assets/catalog.cfg, ./assets/catalog.cfg
        std::wstring candidates[3] = {
            ExeDirW() + L"\\assets\\catalog.cfg",
            ExeDirW() + L"\\..\\assets\\catalog.cfg",
            L"assets\\catalog.cfg"
        };
        std::string bytes;
        for (int i = 0; i < 3; i++) {
            if (ReadBinFile(candidates[i], bytes)) break;
        }
        if (bytes.empty()) { LoadBuiltins(); return false; }

        std::wstring text = Utf8ToW(bytes);
        std::vector<ControlMeta> all;
        std::map<std::wstring, int> idx;
        for (const std::wstring& lineRaw : SplitW(text, L'\n')) {
            std::wstring line = TrimW(lineRaw);
            if (line.empty() || StartsWithW(line, L"#")) continue;
            std::vector<std::wstring> f = SplitW(line, L'|');
            if (f.empty()) continue;
            for (auto& x : f) x = TrimW(x);
            if (f[0] == L"EL" && f.size() >= 8) {
                ControlMeta m;
                m.type = f[1]; m.category = f[2]; m.defaultEvent = f[3];
                m.defaultProperty = f[4]; m.win32Class = f[5]; m.win32Note = f[6];
                m.kind = f[7];
                idx[m.type] = (int)all.size();
                all.push_back(m);
            } else if (f[0] == L"PR" && f.size() >= 4) {
                auto it = idx.find(f[1]);
                if (it != idx.end()) all[it->second].props.push_back(f[2]);
            } else if (f[0] == L"EV" && f.size() >= 3) {
                auto it = idx.find(f[1]);
                if (it != idx.end()) all[it->second].events.push_back(f[2]);
            }
        }
        if (all.empty()) { LoadBuiltins(); return false; }
        g_all = std::move(all);
        g_loaded = true;
        return true;
    }

    bool IsLoaded() { return g_loaded; }

    const std::vector<ControlMeta>& All() { return g_all; }

    const ControlMeta* Find(const std::wstring& type)
    {
        for (const auto& m : g_all) if (m.type == type) return &m;
        return nullptr;
    }

    std::vector<std::wstring> Categories()
    {
        std::vector<std::wstring> out;
        for (const auto& m : g_all) {
            if (std::find(out.begin(), out.end(), m.category) == out.end())
                out.push_back(m.category);
        }
        return out;
    }

    std::vector<const ControlMeta*> ByCategory(const std::wstring& cat)
    {
        std::vector<const ControlMeta*> out;
        for (const auto& m : g_all) if (m.category == cat) out.push_back(&m);
        return out;
    }

    std::vector<std::wstring> EventListFor(const std::wstring& type)
    {
        const ControlMeta* m = Find(type);
        if (m && !m->events.empty()) return m->events;
        // generic fallback
        std::vector<std::wstring> ev = { L"Click", L"DoubleClick", L"Paint", L"MouseMove", L"KeyDown" };
        return ev;
    }

    std::vector<std::wstring> PropListFor(const std::wstring& type)
    {
        const ControlMeta* m = Find(type);
        if (m) return m->props;
        return { L"BackColor", L"ForeColor", L"Enabled", L"Visible" };
    }

    std::wstring DefaultDblEventFor(const std::wstring& type, bool isForm)
    {
        if (isForm) return L"Load";
        // user explicitly asked Paint for PictureBox / GroupBox / Panel
        if (type == L"PictureBox" || type == L"GroupBox" || type == L"Panel") return L"Paint";
        const ControlMeta* m = Find(type);
        if (m && !m->defaultEvent.empty()) return m->defaultEvent;
        return L"Click";
    }
}

// ===========================================================================
// Serialization — line based, '|' separated (paths never contain '|')
// ===========================================================================
static std::wstring EscPipe(std::wstring s)
{
    return ReplaceW(s, L"|", L"/");
}

std::wstring SerializeLayout(const FormInfo& f, const std::vector<ControlInstance>& ctrls)
{
    std::wstring out;
    out += L"FORM|" + f.name + L"|" + EscPipe(f.text) + L"|" +
           std::to_wstring(f.w) + L"|" + std::to_wstring(f.h) + L"|" + f.backColor + L"\n";
    for (const auto& c : ctrls) {
        out += L"CTRL|" + c.type + L"|" + c.name + L"|" + EscPipe(c.text) + L"|" +
               std::to_wstring(c.x) + L"|" + std::to_wstring(c.y) + L"|" +
               std::to_wstring(c.w) + L"|" + std::to_wstring(c.h) + L"|" +
               std::to_wstring(c.idc) + L"|" + (c.visible ? L"1" : L"0") + L"|" +
               (c.enabled ? L"1" : L"0") + L"\n";
        for (const auto& kv : c.events)
            out += L"EVT|" + c.name + L"|" + kv.first + L"|" + kv.second + L"\n";
        for (const auto& kv : c.props)
            out += L"PROP|" + c.name + L"|" + kv.first + L"|" + kv.second + L"\n";
    }
    return out;
}

bool ParseLayout(const std::wstring& text, FormInfo& f, std::vector<ControlInstance>& ctrls)
{
    f = FormInfo();
    ctrls.clear();
    std::map<std::wstring, int> idx;
    for (const std::wstring& lineRaw : SplitW(text, L'\n')) {
        std::wstring line = lineRaw;
        while (!line.empty() && (line.back() == L'\r' || line.back() == L' ')) line.pop_back();
        if (line.empty()) continue;
        std::vector<std::wstring> t = SplitW(line, L'|');
        if (t.empty()) continue;
        if (t[0] == L"FORM" && t.size() >= 6) {
            f.name = t[1]; f.text = t[2];
            f.w = _wtoi(t[3].c_str()); f.h = _wtoi(t[4].c_str());
            f.backColor = t[5];
        } else if (t[0] == L"CTRL" && t.size() >= 8) {
            ControlInstance c;
            c.type = t[1]; c.name = t[2]; c.text = t[3];
            c.x = _wtoi(t[4].c_str()); c.y = _wtoi(t[5].c_str());
            c.w = _wtoi(t[6].c_str()); c.h = _wtoi(t[7].c_str());
            if (t.size() > 8)  c.idc     = _wtoi(t[8].c_str());
            if (t.size() > 9)  c.visible = t[9] != L"0";
            if (t.size() > 10) c.enabled = t[10] != L"0";
            idx[c.name] = (int)ctrls.size();
            ctrls.push_back(c);
        } else if (t[0] == L"EVT" && t.size() >= 4) {
            auto it = idx.find(t[1]);
            if (it != idx.end()) ctrls[it->second].events[t[2]] = t[3];
        } else if (t[0] == L"PROP" && t.size() >= 4) {
            auto it = idx.find(t[1]);
            if (it != idx.end()) ctrls[it->second].props[t[2]] = t[3];
        }
    }
    return !f.name.empty();
}

std::wstring SerializeProjects(const std::vector<ProjectRecord>& v)
{
    std::wstring out = L"PROJECTS|v1\n";
    for (const auto& p : v) {
        out += L"P|" + p.name + L"|" + std::to_wstring(p.created) + L"|" +
               std::to_wstring(p.lastOpened) + L"|" + p.sourceDir + L"|" +
               p.cppPath + L"|" + p.slnPath + L"|" + (p.fromVS ? L"1" : L"0") + L"|" +
               std::to_wstring(p.pid) + L"\n";
    }
    return out;
}

bool ParseProjects(const std::wstring& text, std::vector<ProjectRecord>& v)
{
    v.clear();
    for (const std::wstring& lineRaw : SplitW(text, L'\n')) {
        std::wstring line = lineRaw;
        while (!line.empty() && (line.back() == L'\r' || line.back() == L' ')) line.pop_back();
        if (!StartsWithW(line, L"P|")) continue;
        std::vector<std::wstring> t = SplitW(line, L'|');
        if (t.size() < 9) continue;
        ProjectRecord p;
        p.name       = t[1];
        p.created    = _wtoi64(t[2].c_str());
        p.lastOpened = _wtoi64(t[3].c_str());
        p.sourceDir  = t[4];
        p.cppPath    = t[5];
        p.slnPath    = t[6];
        p.fromVS     = t[7] == L"1";
        p.pid        = (int)_wtoi(t[8].c_str());
        v.push_back(p);
    }
    return true;
}

// ===========================================================================
// Helpers
// ===========================================================================
std::wstring MakeUniqueName(const std::wstring& type,
                            const std::vector<ControlInstance>& existing)
{
    // VS style: "PictureBox" -> "pictureBox1", "GroupBox" -> "groupBox1".
    // keep camel case, map Turkish/other non-ASCII per char, first letter lower.
    std::wstring base = type;
    if (base.empty()) base = L"Control";
    for (auto& ch : base) {
        switch (ch) {
            case 0xE7: ch = L'c'; break;  // ç
            case 0x11F: ch = L'g'; break; // ğ
            case 0x131: ch = L'i'; break; // ı
            case 0xF6: ch = L'o'; break;  // ö
            case 0x15F: ch = L's'; break; // ş
            case 0xFC: ch = L'u'; break;  // ü
            case 0xC7: ch = L'C'; break;  // Ç
            case 0x11E: ch = L'G'; break; // Ğ
            case 0x130: ch = L'I'; break; // İ
            case 0xD6: ch = L'O'; break;  // Ö
            case 0x15E: ch = L'S'; break; // Ş
            case 0xDC: ch = L'U'; break;  // Ü
            default:
                if ((unsigned)ch >= 0x80) ch = L'_';
                else if (!iswalnum((wint_t)ch) && ch != L'_') ch = L'_';
                break;
        }
    }
    base[0] = (wchar_t)towlower((wint_t)base[0]);
    for (int i = 1; ; i++) {
        std::wstring cand = base + std::to_wstring(i);
        bool used = false;
        for (const auto& c : existing) if (LowerW(c.name) == LowerW(cand)) { used = true; break; }
        if (!used) return cand;
    }
}

int NextControlId(const std::vector<ControlInstance>& existing)
{
    int mx = 1000;
    for (const auto& c : existing) if (c.idc > mx) mx = c.idc;
    return mx + 1;
}

void SortProjectsNewestFirst(std::vector<ProjectRecord>& v)
{
    std::stable_sort(v.begin(), v.end(),
                     [](const ProjectRecord& a, const ProjectRecord& b)
                     { return a.lastOpened > b.lastOpened; });
}
