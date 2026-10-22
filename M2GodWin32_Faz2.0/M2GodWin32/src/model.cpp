// model.cpp — tasarım modeli gerçeklemesi
#include "model.h"
#include "app.h"
#include "json.h"

Model g_model;

UINT Model::NextCid() {
    UINT id = 3001;
    bool clash = true;
    while (clash) {
        clash = false;
        for (auto* c : ctrls) if (c->cid == id) { clash = true; id++; break; }
    }
    return id;
}

std::wstring Model::UniqueName(const std::wstring& type) {
    std::wstring base = type;
    if (!base.empty()) base[0] = (wchar_t)towlower(base[0]);
    for (int k = 1; k < 1000; k++) {
        wchar_t buf[32]; swprintf(buf, 32, L"%d", k);
        std::wstring cand = base + buf;
        if (!FindByName(cand)) return cand;
    }
    return base + L"_x";
}

Ctrl* Model::Add(const std::wstring& type, int x, int y) {
    const ElementDef* def = g_catalog.Find(type);
    if (!def) return nullptr;
    Ctrl* c = new Ctrl();
    c->name = UniqueName(type);
    c->type = type;
    c->x = x; c->y = y; c->w = 100; c->h = 30;
    c->cid = NextCid();
    c->nonVisual = def->winClass.empty();
    // katalog varsayılanlarını doldur
    for (auto& p : def->props) {
        if (!p.canSet || !p.browsable) continue;
        std::wstring n = p.name;
        if (n == L"Name") c->props[L"Name"] = c->name;
        else if (n == L"Location") { wchar_t b[64]; swprintf(b, 64, L"%d, %d", x, y); c->props[L"Location"] = b; }
        else if (n == L"Size") c->props[L"Size"] = L"100, 30";
        else if (n == L"Text") {
            std::wstring txt = c->name;
            if (!txt.empty()) txt[0] = (wchar_t)towupper(txt[0]);
            c->props[L"Text"] = txt;
        }
        else if (n == L"TabStop" || n == L"Enabled" || n == L"Visible") c->props[n] = L"True";
        else if (n == L"Font") c->props[L"Font"] = L"Segoe UI, 9pt";
        else if (n == L"ForeColor") c->props[L"ForeColor"] = L"ControlText";
        else if (n == L"BackColor") c->props[L"BackColor"] = L"Control";
        else if (!p.def.empty()) c->props[n] = p.def;
    }
    ctrls.push_back(c);
    return c;
}

void Model::Delete(Ctrl* c) {
    if (!c) return;
    if (selected == c) { selected = nullptr; formSelected = true; }
    for (size_t i = 0; i < ctrls.size(); i++)
        if (ctrls[i] == c) { ctrls.erase(ctrls.begin() + i); break; }
    delete c;
}

Ctrl* Model::Clone(Ctrl* src, int dx, int dy) {
    const ElementDef* def = g_catalog.Find(src->type);
    if (!def) return nullptr;
    Ctrl* c = new Ctrl(*src);
    c->name = UniqueName(src->type);
    c->props[L"Name"] = c->name;
    c->x = src->x + dx; c->y = src->y + dy;
    c->cid = NextCid();
    c->hwnd = nullptr;
    c->events.clear();                       // klon olayları devralmasın
    wchar_t b[64]; swprintf(b, 64, L"%d, %d", c->x, c->y);
    c->props[L"Location"] = b;
    ctrls.push_back(c);
    return c;
}

Ctrl* Model::FindByName(const std::wstring& name) {
    for (auto* c : ctrls) if (_wcsicmp(c->name.c_str(), name.c_str()) == 0) return c;
    return nullptr;
}
Ctrl* Model::FindByHwnd(HWND h) {
    for (auto* c : ctrls) if (c->hwnd == h) return c;
    return nullptr;
}
Ctrl* Model::FindByCid(UINT cid) {
    for (auto* c : ctrls) if (c->cid == cid) return c;
    return nullptr;
}

void Model::Rename(Ctrl* c, const std::wstring& newName) {
    if (!c || newName.empty() || FindByName(newName)) return;
    std::wstring old = c->name;
    c->name = newName;
    c->props[L"Name"] = newName;
    // olay işleyici adlarını da güncelle
    std::wstring oldU = old; if (!oldU.empty()) oldU[0] = (wchar_t)towlower(oldU[0]);
    std::wstring newU = newName; if (!newU.empty()) newU[0] = (wchar_t)towlower(newU[0]);
    for (auto& kv : c->events) {
        std::wstring h = kv.second;
        if (h.size() > old.size() && _wcsnicmp(h.c_str(), old.c_str(), old.size()) == 0)
            kv.second = newU + h.substr(old.size());
    }
}

void Model::BringToFront(Ctrl* c) {
    for (size_t i = 0; i < ctrls.size(); i++)
        if (ctrls[i] == c) { ctrls.erase(ctrls.begin() + i); ctrls.push_back(c); break; }
    if (c->hwnd) SetWindowPos(c->hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}
void Model::SendToBack(Ctrl* c) {
    for (size_t i = 0; i < ctrls.size(); i++)
        if (ctrls[i] == c) { ctrls.erase(ctrls.begin() + i); ctrls.insert(ctrls.begin(), c); break; }
    if (c->hwnd) SetWindowPos(c->hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}
void Model::ClearAll() {
    for (auto* c : ctrls) delete c;
    ctrls.clear();
    selected = nullptr;
    formSelected = true;
}

std::wstring Model::DefaultEventOf(const Ctrl* c) const {
    const ElementDef* def = g_catalog.Find(c->type);
    return def ? def->defaultEvent : L"Click";
}
std::wstring Model::HandlerNameOf(const Ctrl* c, const std::wstring& ev) const {
    std::wstring n = c->name;
    if (!n.empty()) n[0] = (wchar_t)towlower(n[0]);
    return n + L"_" + ev;
}

// ---------------------------------------------------------------------------
void Model::PushUndo() {
    undo.push_back(json::Stringify(ToJson(), false));
    if (undo.size() > 100) undo.pop_front();
    redo.clear();
}
bool Model::Undo() {
    if (undo.empty()) return false;
    redo.push_back(json::Stringify(ToJson(), false));
    std::string st = undo.back(); undo.pop_back();
    bool ok = false; json::Value v = json::Parse(st, &ok);
    if (ok) { HWND surf = GetDlgItem(g_hMain, IDC_SURFACE); for (auto* c : ctrls) if (c->hwnd) { DestroyWindow(c->hwnd); c->hwnd = nullptr; } FromJson(v); if (surf) InvalidateRect(surf, nullptr, TRUE); }
    return ok;
}
bool Model::Redo() {
    if (redo.empty()) return false;
    undo.push_back(json::Stringify(ToJson(), false));
    std::string st = redo.back(); redo.pop_back();
    bool ok = false; json::Value v = json::Parse(st, &ok);
    if (ok) { HWND surf = GetDlgItem(g_hMain, IDC_SURFACE); for (auto* c : ctrls) if (c->hwnd) { DestroyWindow(c->hwnd); c->hwnd = nullptr; } FromJson(v); if (surf) InvalidateRect(surf, nullptr, TRUE); }
    return ok;
}
void Model::ClearUndo() { undo.clear(); redo.clear(); }

// ---------------------------------------------------------------------------
json::Value Model::ToJson() const {
    json::Value root = json::Value::MakeObj();
    root.Set(L"format", L"m2god-form");
    root.Set(L"version", 2.0);
    json::Value form = json::Value::MakeObj();
    for (auto& kv : formProps) form.Set(kv.first, kv.second);
    json::Value fe = json::Value::MakeObj();
    for (auto& kv : formEvents) fe.Set(kv.first, kv.second);
    form.Set(L"Events", fe);
    root.Set(L"Form", form);
    json::Value arr = json::Value::MakeArr();
    for (auto* c : ctrls) {
        json::Value o = json::Value::MakeObj();
        o.Set(L"name", c->name);
        o.Set(L"type", c->type);
        json::Value p = json::Value::MakeObj();
        for (auto& kv : c->props) p.Set(kv.first, kv.second);
        o.Set(L"Props", p);
        json::Value ev = json::Value::MakeObj();
        for (auto& kv : c->events) ev.Set(kv.first, kv.second);
        o.Set(L"Events", ev);
        arr.Push(o);
    }
    root.Set(L"Controls", arr);
    return root;
}
bool Model::FromJson(const json::Value& v) {
    ClearAll();
    const json::Value& form = v.Get(L"Form");
    if (form.IsObj()) {
        for (auto& kv : form.obj)
            if (kv.first != L"Events") formProps[kv.first] = kv.second.AsStr();
        const json::Value& fe = form.Get(L"Events");
        if (fe.IsObj()) for (auto& kv : fe.obj) formEvents[kv.first] = kv.second.AsStr();
    }
    const json::Value& arr = v.Get(L"Controls");
    if (arr.IsArr()) {
        for (auto& o : arr.arr) {
            std::wstring type = o.Get(L"type").AsStr();
            if (!g_catalog.Find(type)) continue;
            Ctrl* c = Add(type, 20, 20);
            if (!c) continue;
            c->name = o.Get(L"name").AsStr(c->name);
            const json::Value& p = o.Get(L"Props");
            if (p.IsObj()) {
                c->props.clear();
                for (auto& kv : p.obj) c->props[kv.first] = kv.second.AsStr();
            }
            const json::Value& ev = o.Get(L"Events");
            if (ev.IsObj()) for (auto& kv : ev.obj) c->events[kv.first] = kv.second.AsStr();
            // Location/Size → geometri
            auto loc = Split(c->Get(L"Location", L"20, 20"), L',');
            if (loc.size() == 2) { c->x = _wtoi(Trim(loc[0]).c_str()); c->y = _wtoi(Trim(loc[1]).c_str()); }
            auto sz = Split(c->Get(L"Size", L"100, 30"), L',');
            if (sz.size() == 2) { c->w = _wtoi(Trim(sz[0]).c_str()); c->h = _wtoi(Trim(sz[1]).c_str()); }
            if (c->w < 4) c->w = 4; if (c->h < 4) c->h = 4;
        }
    }
    auto cs = Split(formProps.count(L"ClientSize") ? formProps[L"ClientSize"] : L"800, 600", L',');
    if (cs.size() == 2) { formW = _wtoi(Trim(cs[0]).c_str()); formH = _wtoi(Trim(cs[1]).c_str()); }
    if (formW < 100) formW = 100; if (formH < 100) formH = 100;
    if (!formProps.count(L"Text")) formProps[L"Text"] = L"Form1";
    if (!formProps.count(L"Name")) formProps[L"Name"] = L"Form1";
    return true;
}
bool Model::Save(const std::wstring& path) { return WriteFileText(path, json::Stringify(ToJson(), true)); }
bool Model::Load(const std::wstring& path) {
    std::string data;
    if (!ReadFileText(path, data)) return false;
    bool ok = false; json::Value v = json::Parse(data, &ok);
    if (!ok) return false;
    return FromJson(v);
}
