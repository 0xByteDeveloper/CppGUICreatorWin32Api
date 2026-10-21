// ============================================================================
//  MainWindow.h — application shell: menus, options strip, tabs, actions
// ============================================================================
#pragma once
#include "stdafx.h"
#include "Model.h"
#include "Store.h"
#include "VSDiscovery.h"

enum TopMenu
{
    MENU_WELCOME = 0,   // Welcome
    MENU_PROJELER,      // Projeler
    MENU_ARAC,          // Araç Kutusu
    MENU_GORUNUM,       // Görünüm
    MENU_BAGLANTI,      // Bağlantı
    MENU_YARDIM,        // Yardım
    MENU_COUNT
};

struct MainWindow
{
    HWND hwnd = nullptr;
    HWND hTop = nullptr, hOpts = nullptr, hTabs = nullptr, hStatus = nullptr;

    struct Tab
    {
        HWND         host = nullptr;   // WelcomePage or DesignerHost child window
        std::wstring project;          // "" for welcome
        bool         welcome = false;
    };
    std::vector<Tab> tabs;
    int  active = -1;

    std::vector<ProjectRecord> recents;
    std::vector<VSInstance>    vs;
    std::vector<OptionItem>    options;
    int  selectedOption = -1;
    int  currentMenu = MENU_WELCOME;

    bool showGrid = true, snap = true, propsVisible = true;

    // ---- lifecycle ----
    bool Create(HINSTANCE inst);
    void LayoutChildren();

    // ---- menus & options ----
    void SetMenu(int which);          // rebuild options strip
    void RebuildOptions();
    void OnOptionClick(int index);    // includes the "+" (isPlus)
    void OnTopBarVSChip();

    // ---- tabs ----
    void ActivateTab(int idx);
    void CloseTab(int idx);
    int  FindTabByProject(const std::wstring& name);
    void OpenWelcomeTab();
    bool OpenProjectTab(const std::wstring& name, std::wstring* err = nullptr);

    // ---- data flows ----
    void RefreshVS(bool notify = true);
    void RefreshRecents();
    void AddControlToActive(const std::wstring& type);
    void CreateNewProjectFlow();      // dialog then tab
    void Status(const std::wstring& s);

    // ---- designer callbacks (children call these) ----
    void OnControlDblClick(const std::wstring& project, const std::wstring& ctrlName,
                           const std::wstring& ctrlType, const std::wstring& ev);
    void OnFormDblClick(const std::wstring& project);
    void OnLayoutChanged(const std::wstring& project);   // autosave

    // ---- misc commands ----
    void ExportActiveProject();
};

extern MainWindow* g_mw;          // global singleton (children call back)
extern HWND        g_hMain;       // main hwnd (children notifications)
