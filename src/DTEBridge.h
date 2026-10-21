// ============================================================================
//  DTEBridge.h — VS2019 automation via EnvDTE late binding (pure COM, no .NET)
//
//  Flow (see PLAN.md §7.3):
//    1. Find DTE for the devenv pid in the Running Object Table (ROT)
//    2. DTE.ItemOperations.OpenFile(<cpp>)
//    3. doc -> TextDocument -> Selection.MoveToLineAndOffset(line, 1, false)
//    4. DTE.MainWindow.Activate() + SetForegroundWindow
//  All calls are best-effort; on failure the caller falls back to
//  ShellExecuteW("open", cpp).
// ============================================================================
#pragma once
#include "stdafx.h"

namespace DTE
{
    // open <file> inside the VS instance owning <pid> (or any VS if pid==0),
    // jump to <line> (1-based). Returns false + error text when automation
    // is unavailable (VS busy / DTE missing).
    bool OpenFileAtLine(HWND vsMainWnd, DWORD pid,
                        const std::wstring& file, int line,
                        std::wstring* err);

    // try to bring the VS main window to the foreground
    void ActivateVS(HWND vsMainWnd);
}
