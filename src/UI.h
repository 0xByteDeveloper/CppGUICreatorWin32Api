// ============================================================================
//  UI.h — widget front-ends (implementation in Bars/WelcomePage/Designer/...)
//
//  Widget -> MainWindow communication:
//    - children receive MainWindow* g_mw at creation and call its methods
//    - MainWindow repaints/relayouts via the setters below
// ============================================================================
#pragma once
#include "stdafx.h"
#include "Model.h"
#include "MainWindow.h"

// ---------------------------------------------------------------------------
// TopBar — app name + menu item selector (Apple segmented pills) + VS chip
// ---------------------------------------------------------------------------
namespace TopBarNS
{
    ATOM  RegisterClass1(HINSTANCE);
    HWND  Create(HWND parent);
    void  SetActiveMenu(HWND h, int idx);
    void  SetVSCount(HWND h, int count);
}

// ---------------------------------------------------------------------------
// OptionsStrip — contextual options row; last pill is the accent "+"
// ---------------------------------------------------------------------------
namespace OptionsNS
{
    ATOM  RegisterClass1(HINSTANCE);
    HWND  Create(HWND parent);
    void  SetOptions(HWND h, const std::vector<OptionItem>& opts, int selected);
}

// ---------------------------------------------------------------------------
// TabBar — Apple-style closable tab strip
// ---------------------------------------------------------------------------
namespace TabBarNS
{
    ATOM  RegisterClass1(HINSTANCE);
    HWND  Create(HWND parent);
    void  SetTabs(HWND h, const std::vector<MainWindow::Tab>& tabs, int active);
}

// ---------------------------------------------------------------------------
// StatusStrip — bottom one-line status
// ---------------------------------------------------------------------------
namespace StatusNS
{
    ATOM  RegisterClass1(HINSTANCE);
    HWND  Create(HWND parent);
    void  SetText(HWND h, const std::wstring& s);
}

// ---------------------------------------------------------------------------
// WelcomePage — hero + running VS chips + recent projects (newest first)
// ---------------------------------------------------------------------------
namespace WelcomeNS
{
    ATOM  RegisterClass1(HINSTANCE);
    HWND  Create(HWND parent);
    void  Refresh(HWND h);   // pulls data from g_mw
}

// ---------------------------------------------------------------------------
// DesignerHost — canvas (form designer) + properties/events panel
// ---------------------------------------------------------------------------
namespace DesignerNS
{
    ATOM  RegisterClass1(HINSTANCE);
    HWND  Create(HWND parent, const std::wstring& project);

    // API for MainWindow
    std::wstring ProjectName(HWND host);
    void  AddControl(HWND host, const std::wstring& type);
    void  RefreshProps(HWND host);
    void  SetPropsVisible(HWND host, bool v);
    bool  SaveNow(HWND host);
    void  FocusCanvas(HWND host);
    // export via CodeGen (writes main_w32.cpp + runtime header into project dir)
    bool  Export(HWND host, std::wstring* err);
}

// ---------------------------------------------------------------------------
// Dialogs
// ---------------------------------------------------------------------------
struct NameDialogResult { bool ok = false; std::wstring text; };
NameDialogResult ShowNameDialog(HWND parent, const std::wstring& title,
                                const std::wstring& subtitle,
                                const std::wstring& initial,
                                const std::wstring& placeholder);
// tour/demo-only: show the same name dialog WITHOUT its modal loop so a host
// script can screenshot it; text is typed programmatically, then hide it.
HWND TourShowNameDialogPeek(HWND parent, const std::wstring& typedText);
void TourHideNameDialogPeek();
void ShowAboutDialog(HWND parent);
void ShowDependencyReport(HWND parent);
