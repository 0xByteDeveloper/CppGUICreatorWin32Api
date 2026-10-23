// model.cpp — tasarım modeli gerçeklemesi
#include "model.h"
#include "app.h"
#include "json.h"
#include "surface.h"

Model g_model;

// Konteyner tipleri: içine kontrol bırakılabilir
bool IsContainerType(const std::wstring& type) {
    static const wchar_t* k[] = {
        L"Panel", L"FlowLayoutPanel", L"TableLayoutPanel", L"SplitContainer",
        L"GroupBox", L"TabControl", L"TabPage"
    };
    for (auto* s : k) if (_wcsicmp(s, type.c_str()) == 0) return true;
    return false;
}

// FAZ 2.6 — TabControl'ün sayfaları (Tip==TabPage çocukları, listedeki sıra = sekme sırası)
std::vector<Ctrl*> TabPagesOf(const Ctrl* tab) {
    std::vector<Ctrl*> out;
    if (!tab) return out;
    for (auto* c : g_model.ctrls)
        if (c->parent == tab && _wcsicmp(c->type.c_str(), L"TabPage") == 0) out.push_back(c);
    return out;
}
// Sayfa başlıklarını TabControl'ün Text özelliğine yaz (geriye dönük uyum + kayıt)
void SyncTabText(Ctrl* tab) {
    if (!tab || _wcsicmp(tab->type.c_str(), L"TabControl") != 0) return;
    std::wstring t;
    for (auto* p : TabPagesOf(tab)) { if (!t.empty()) t += L"\n"; t += p->Get(L"Text"); }
    if (t.empty()) t = tab->Get(L"Text");
    tab->props[L"Text"] = t;
}

std::vector<Ctrl*> Model::ChildrenOf(Ctrl* p) const {
    std::vector<Ctrl*> out;
    for (auto* c : ctrls) if (c->parent == p) out.push_back(c);
    return out;   // listedeki sıra = oluşturma sırası = z-sırası (son = üstte)
}

bool Model::IsAncestorOf(Ctrl* a, Ctrl* c) const {
    for (Ctrl* p = c ? c->parent : nullptr; p; p = p->parent)
        if (p == a) return true;
    return false;
}

int Model::DepthOf(const Ctrl* c) const {
    int d = 0;
    for (Ctrl* p = c ? c->parent : nullptr; p; p = p->parent) d++;
    return d;
}

POINT Model::AbsPos(const Ctrl* c) const {
    POINT p = { 0, 0 };
    for (Ctrl* q = (Ctrl*)c; q; q = q->parent) { p.x += q->x; p.y += q->y; }
    return p;
}

void Model::Reparent(Ctrl* c, Ctrl* newParent, int px, int py) {
    if (!c || c == newParent || IsAncestorOf(c, newParent)) return;
    if (c->parent == newParent) { c->x = px; c->y = py; }
    else {
        c->parent = newParent;
        c->x = px; c->y = py;
        if (c->hwnd) {
            HWND ph = newParent ? newParent->hwnd : GetDlgItem(g_hMain, IDC_SURFACE);
            if (ph) SetParent(c->hwnd, ph);
        }
    }
    wchar_t b[64]; swprintf(b, 64, L"%d, %d", c->x, c->y);
    c->props[L"Location"] = b;
}

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

Ctrl* Model::Add(const std::wstring& type, int x, int y, Ctrl* parent, bool autoPages) {
    const ElementDef* def = g_catalog.Find(type);
    if (!def) return nullptr;
    Ctrl* c = new Ctrl();
    c->name = UniqueName(type);
    c->type = type;
    c->parent = parent;
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
    // FAZ 2.6: TabControl eklendi → VS gibi otomatik tabPage1 + tabPage2
    if (_wcsicmp(type.c_str(), L"TabControl") == 0 && autoPages) {
        AddPage(c, L"tabPage1");
        AddPage(c, L"tabPage2");
    }
    return c;
}

// TabControl'e yeni sayfa (VS "Sekme Ekle"): Ctrl olarak ekler + Text'i senkronlar
Ctrl* Model::AddPage(Ctrl* tab, const std::wstring& title) {
    if (!tab || _wcsicmp(tab->type.c_str(), L"TabControl") != 0) return nullptr;
    Ctrl* pg = Add(L"TabPage", 4, 30, tab, false);
    if (!pg) return nullptr;
    if (!title.empty()) pg->props[L"Text"] = title;
    pg->w = max(10, tab->w - 8);
    pg->h = max(10, tab->h - 34);
    SyncTabText(tab);
    return pg;
}

// Sayfayı kaldır (VS "Sekme Kaldır") — çocuklarıyla birlikte; kalan sayfa Text'i senkron
bool Model::RemovePage(Ctrl* tab, Ctrl* page) {
    if (!tab || !page || page->parent != tab) return false;
    Delete(page);          // Delete alt kontrolleri de siler
    SyncTabText(tab);
    return true;
}

void Model::Delete(Ctrl* c) {
    if (!c) return;
    bool wasPage = _wcsicmp(c->type.c_str(), L"TabPage") == 0;
    Ctrl* par = c->parent;
    // alt kontrolleri de sil (derinlemesine)
    for (auto* k : ChildrenOf(c)) Delete(k);
    if (selected == c || IsAncestorOf(c, selected)) { selected = nullptr; formSelected = true; }
    // HAYALET DÜZELTMESİ: gerçek HWND YOK EDİLMEDEN modelden çıkarılmaz —
    // (eski kodda HWND kalıyor, silinen kontrol ekranda duruyordu)
    if (c->hwnd) {
        surface::OnModelDelete(c);          // aktif sürükleme durumunu güvenle kapat
        if (IsWindow(c->hwnd)) DestroyWindow(c->hwnd);
        c->hwnd = nullptr;
    }
    for (size_t i = 0; i < ctrls.size(); i++)
        if (ctrls[i] == c) { ctrls.erase(ctrls.begin() + i); break; }
    delete c;
    // FAZ 2.6: sayfa silindiyse sekme başlık Text'ini senkronla
    if (wasPage && par) SyncTabText(par);
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
    if (!c) return;
    // kardeşler arasında en üste taşı (düz listede son kardeşin arkasına)
    for (size_t i = 0; i < ctrls.size(); i++) {
        if (ctrls[i] == c) {
            ctrls.erase(ctrls.begin() + i);
            size_t last = ctrls.size();
            for (size_t j = 0; j < ctrls.size(); j++)
                if (ctrls[j]->parent == c->parent) last = j + 1;
            ctrls.insert(ctrls.begin() + last, c);
            break;
        }
    }
    if (c->hwnd) SetWindowPos(c->hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
void Model::SendToBack(Ctrl* c) {
    if (!c) return;
    for (size_t i = 0; i < ctrls.size(); i++) {
        if (ctrls[i] == c) {
            ctrls.erase(ctrls.begin() + i);
            size_t first = 0;
            for (size_t j = 0; j < ctrls.size(); j++)
                if (ctrls[j]->parent == c->parent) { first = j; break; }
            ctrls.insert(ctrls.begin() + first, c);
            break;
        }
    }
    if (c->hwnd) SetWindowPos(c->hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
void Model::ClearAll() {
    // HWND'leri önce yok et (yoksa eski "Yeni Tasarım"da tüm kontroller ekranda kalırdı)
    for (auto* c : ctrls) {
        if (c->hwnd) {
            surface::OnModelDelete(c);
            if (IsWindow(c->hwnd)) DestroyWindow(c->hwnd);
            c->hwnd = nullptr;
        }
        delete c;
    }
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
        o.Set(L"Parent", c->parent ? c->parent->name : L"");
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
        // 1. geçiş: tüm kontrolleri oluştur (parent geçici olarak isimle saklanır)
        std::map<std::wstring, std::wstring> parentNameOf;
        for (auto& o : arr.arr) {
            std::wstring type = o.Get(L"type").AsStr();
            if (!g_catalog.Find(type)) continue;
            Ctrl* c = Add(type, 20, 20);
            if (!c) continue;
            c->name = o.Get(L"name").AsStr(c->name);
            parentNameOf[c->name] = o.Get(L"Parent").AsStr();
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
        // 2. geçiş: parent bağlantılarını kur
        for (auto& kv : parentNameOf) {
            if (kv.second.empty()) continue;
            Ctrl* c = FindByName(kv.first);
            Ctrl* p = FindByName(kv.second);
            if (c && p && c != p && !IsAncestorOf(c, p)) c->parent = p;
        }
        // FAZ 2.6 MİGRASYON: eski tasarımlarda sayfalar Text satırlarıydı; sayfa
        // çocuğu yoksa Text satırlarından gerçek TabPage Ctrl'leri üret
        for (Ctrl* tc : ctrls) {
            if (_wcsicmp(tc->type.c_str(), L"TabControl") != 0) continue;
            if (!TabPagesOf(tc).empty()) { SyncTabText(tc); continue; }
            auto lines = Split(tc->Get(L"Text"), L'\n');
            std::vector<std::wstring> titles;
            for (auto& l : lines) { std::wstring t = Trim(l); if (!t.empty()) titles.push_back(t); }
            if (titles.empty()) titles = { L"tabPage1", L"tabPage2" };
            for (auto& t : titles) {
                Ctrl* pg = Add(L"TabPage", 4, 30, tc, false);
                if (pg) { pg->props[L"Text"] = t; pg->w = max(10, tc->w - 8); pg->h = max(10, tc->h - 34); }
            }
            SyncTabText(tc);
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
