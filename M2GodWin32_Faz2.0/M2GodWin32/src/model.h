// model.h — tasarım modeli: form + kontroller + undo
#pragma once
#include "app.h"
#include "json.h"
#include "catalog.h"
#include <deque>

struct Ctrl {
    std::wstring name;             // "button1"
    std::wstring type;             // "Button"
    int x = 20, y = 20, w = 100, h = 32;   // PARENT-GÖRELİ koordinat (parent=null → form koordinatı)
    UINT cid = 0;                  // WM_COMMAND id
    bool nonVisual = false;        // bileşen tepsisinde (Timer vs.)
    Ctrl* parent = nullptr;        // konteyner içi ise üst kontrol (GroupBox/Panel/TabControl...)
    std::map<std::wstring, std::wstring> props;   // özellik → değer (string form)
    std::map<std::wstring, std::wstring> events;  // olay → handler adı ("button1_Click")
    HWND hwnd = nullptr;           // tasarım yüzeyindeki gerçek pencere

    std::wstring Get(const wchar_t* k, const wchar_t* def = L"") const {
        auto it = props.find(k);
        return it == props.end() ? std::wstring(def) : it->second;
    }
    void Set(const std::wstring& k, const std::wstring& v) { props[k] = v; }
};

// Konteyner tipleri: içine kontrol bırakılabilir
bool IsContainerType(const std::wstring& type);

class Model {
public:
    // Form (kök) özellikleri
    std::map<std::wstring, std::wstring> formProps;   // Text, Name, ClientSize, BackColor...
    std::map<std::wstring, std::wstring> formEvents;  // Load → Form1_Load vb.
    std::vector<Ctrl*> ctrls;                          // düz liste: aynı parent içinde son = üstte
    int formW = 800, formH = 600;

    Ctrl* selected = nullptr;      // tek seçim (form için nullptr + formSelected=true)
    bool  formSelected = false;

    // --- parent-child yardımcıları ----------------------------------------
    std::vector<Ctrl*> ChildrenOf(Ctrl* p) const;      // p=null → form seviyesi
    bool  IsAncestorOf(Ctrl* a, Ctrl* c) const;        // a, c'nin atası mı (a!=c)
    int   DepthOf(const Ctrl* c) const;                // kök=0
    POINT AbsPos(const Ctrl* c) const;                 // form koordinatında sol üst
    void  Reparent(Ctrl* c, Ctrl* newParent, int px, int py);  // HWND + model, px/py: yeni parent görelis

    // --- işlem işaretleri -------------------------------------------------
    void PushUndo();               // her değişiklik öncesi çağır
    bool Undo();
    bool Redo();
    void ClearUndo();
    bool CanUndo() const { return !undo.empty(); }
    bool CanRedo() const { return !redo.empty(); }

    // --- kontroller --------------------------------------------------------
    std::wstring UniqueName(const std::wstring& type);   // "button3"
    Ctrl* Add(const std::wstring& type, int x, int y, Ctrl* parent = nullptr);  // varsayılan boyut/props ile
    void  Delete(Ctrl* c);                                // alt kontrollerle birlikte siler
    Ctrl* Clone(Ctrl* src, int dx, int dy);
    Ctrl* FindByName(const std::wstring& name);
    Ctrl* FindByHwnd(HWND h);
    Ctrl* FindByCid(UINT cid);
    void  Rename(Ctrl* c, const std::wstring& newName);
    void  BringToFront(Ctrl* c);
    void  SendToBack(Ctrl* c);
    void  ClearAll();
    std::wstring DefaultEventOf(const Ctrl* c) const;    // katalog default_event
    std::wstring HandlerNameOf(const Ctrl* c, const std::wstring& ev) const;

    // --- kaydet / yükle (.m2god JSON) --------------------------------------
    json::Value ToJson() const;
    bool FromJson(const json::Value& v);
    bool Save(const std::wstring& path);
    bool Load(const std::wstring& path);

private:
    std::deque<std::string> undo, redo;
    UINT NextCid();
    std::map<std::wstring, int> nameCounters;
};

extern Model g_model;
