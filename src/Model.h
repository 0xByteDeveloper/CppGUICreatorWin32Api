// ============================================================================
//  Model.h — data model: control catalog (from Config_info), instances, projects
// ============================================================================
#pragma once
#include "stdafx.h"

// ---------------------------------------------------------------------------
// Control catalog entry (baked from Config_info.json -> assets/catalog.cfg)
// ---------------------------------------------------------------------------
struct ControlMeta
{
    std::wstring type;              // "Button", "PictureBox", ...
    std::wstring category;          // toolbox category (TR)
    std::wstring defaultEvent;      // VS default (bold) event
    std::wstring defaultProperty;
    std::wstring win32Class;        // win32_mapping.window_class
    std::wstring win32Note;
    std::wstring kind;              // control | container
    std::vector<std::wstring> props;   // curated browsable set (Name/Text/XYWH excluded)
    std::vector<std::wstring> events;  // curated event set
};

namespace Catalog
{
    bool  LoadFromAssets();                    // search exe_dir/assets + CWD
    bool  IsLoaded();
    const std::vector<ControlMeta>& All();
    const ControlMeta* Find(const std::wstring& type);
    std::vector<std::wstring> Categories();    // in first-seen order
    std::vector<const ControlMeta*> ByCategory(const std::wstring& cat);
    // events the properties panel shows for a type (catalog order, curated)
    std::vector<std::wstring> EventListFor(const std::wstring& type);
    std::vector<std::wstring> PropListFor(const std::wstring& type);
    // default double-click event (user asked Paint for PictureBox/GroupBox/Panel)
    std::wstring DefaultDblEventFor(const std::wstring& type, bool isForm);
}

// ---------------------------------------------------------------------------
// Designer instances
// ---------------------------------------------------------------------------
struct ControlInstance
{
    std::wstring type, name, text;
    int  x = 0, y = 0, w = 100, h = 30;
    std::map<std::wstring, std::wstring> props;    // extra properties (BackColor...)
    std::map<std::wstring, std::wstring> events;   // event -> handler name ("" = none)
    int  idc = 0;                                   // runtime control id (1000+)
    bool visible = true, enabled = true;

    bool IsContainer() const { return type == L"GroupBox" || type == L"Panel" || type == L"FlowLayoutPanel"; }
};

struct FormInfo
{
    std::wstring name;          // project/form name, e.g. "MyForm"
    std::wstring text;          // caption
    int  w = 760, h = 480;
    std::wstring backColor = L"FFFFFF";
    std::map<std::wstring, std::wstring> props;  // extra catalog props (runtime)
};

// ---------------------------------------------------------------------------
// Projects
// ---------------------------------------------------------------------------
struct ProjectRecord
{
    std::wstring name;
    long long created = 0, lastOpened = 0;
    std::wstring sourceDir, cppPath, slnPath;
    bool fromVS = false;         // discovered from a running devenv.exe
    int  pid = 0;                // devenv pid when fromVS
};

struct OptionItem
{
    std::wstring text;           // label
    int  tag = 0;                // app-defined
    bool isPlus = false;         // renders as accent "+" pill
    bool isAction = false;       // plain action (vs project/selection entry)
};

// ---------------------------------------------------------------------------
// Serialization (pure string functions — unit-testable, see tests/test_core.cpp)
// ---------------------------------------------------------------------------
std::wstring SerializeLayout(const FormInfo& f, const std::vector<ControlInstance>& ctrls);
bool  ParseLayout(const std::wstring& text, FormInfo& f, std::vector<ControlInstance>& ctrls);
std::wstring SerializeProjects(const std::vector<ProjectRecord>& v);
bool  ParseProjects(const std::wstring& text, std::vector<ProjectRecord>& v);

// unique-name generator ("button1", "button2", ...)
std::wstring MakeUniqueName(const std::wstring& type,
                            const std::vector<ControlInstance>& existing);
int  NextControlId(const std::vector<ControlInstance>& existing); // 1000+

// newest-first sort (portable; used by Store::SortRecents and tests)
void SortProjectsNewestFirst(std::vector<ProjectRecord>& v);
