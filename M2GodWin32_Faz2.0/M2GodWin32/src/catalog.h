// catalog.h — Config_info.json (.NET WinForms kataloğu) yükleyici
// Şema: { meta, toolbox_layout{ Kategori:{category_tr, elements[]} }, elements[], enums{} }
#pragma once
#include <string>
#include <vector>
#include <map>

struct PropDef {
    std::wstring name;        // örn "Text"
    std::wstring type;        // "System.String"
    std::wstring category;    // "Behavior", "Appearance", "Layout"...
    std::wstring def;         // varsayılan değer (string form)
    std::wstring enumType;    // enum ise "System.Windows.Forms.BorderStyle"
    bool browsable = false;
    bool canSet = false;
};
struct EventDef {
    std::wstring name;        // "Click"
    std::wstring handlerType; // "System.EventHandler"
    std::wstring category;
    bool browsable = true;
};
struct ElementDef {
    std::wstring name;            // "Button"
    std::wstring kind;            // control/container/component/dialog/form...
    std::wstring base;
    std::wstring defaultEvent;    // "Click"
    std::wstring defaultProperty; // "Text"
    std::wstring categoryTr;      // "Ortak Denetimler"
    bool toolboxVisible = true;
    std::wstring winClass;        // win32_mapping.window_class
    std::vector<PropDef> props;
    std::vector<EventDef> events;
    // yardımcılar
    const PropDef* FindProp(const std::wstring& n) const {
        for (auto& p : props) if (_wcsicmp(p.name.c_str(), n.c_str()) == 0) return &p;
        return nullptr;
    }
    bool HasEvent(const std::wstring& n) const {
        for (auto& e : events) if (_wcsicmp(e.name.c_str(), n.c_str()) == 0) return true;
        return false;
    }
};

// enum değerleri: "BorderStyle" → { {"None",0},{"FixedSingle",1},{"Fixed3D",2} }
using EnumMap = std::map<std::wstring, std::vector<std::pair<std::wstring, int>>>;

class Catalog {
public:
    std::vector<std::wstring> categories;                 // kategori sırası (toolbox_layout)
    std::map<std::wstring, std::vector<std::wstring>> catElements;
    std::map<std::wstring, ElementDef> elements;          // name → def
    EnumMap enums;

    // configPath boş/okunamazsa yerleşik katalog kullanılır
    bool Load(const std::wstring& configPath);
    const ElementDef* Find(const std::wstring& name) const {
        auto it = elements.find(name);
        return it == elements.end() ? nullptr : &it->second;
    }
    const std::vector<std::pair<std::wstring, int>>* EnumValues(const std::wstring& enumTypeShort) const {
        auto it = enums.find(enumTypeShort);
        return it == enums.end() ? nullptr : &it->second;
    }
    // "System.Windows.Forms.BorderStyle" → "BorderStyle"
    static std::wstring EnumShortName(const std::wstring& full);

    size_t ElementCount() const { return elements.size(); }

private:
    void AddBuiltin();                                     // config yoksa minimum katalog
    void MergeElement(const ElementDef& e);
};

extern Catalog g_catalog;
