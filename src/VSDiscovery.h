// ============================================================================
//  VSDiscovery.h — find running devenv.exe instances + on-disk VS projects
// ============================================================================
#pragma once
#include "stdafx.h"
#include "Model.h"

struct VSInstance
{
    DWORD        pid = 0;
    HWND         hwnd = nullptr;      // main window
    std::wstring title;               // raw window title
    std::wstring solutionName;        // parsed from title
    std::wstring exePath;
    long long    startedUnix = 0;
    std::wstring sourceDir;           // guessed project directory
    std::wstring slnPath;             // found .sln
    std::wstring cppPath;             // best .cpp for code generation
};

namespace VSDiscovery
{
    // running devenv.exe processes (window title parse + process props)
    std::vector<VSInstance> ScanRunning();

    // refresh file info (sln/cpp) for an instance after it is known
    void Enrich(VSInstance& v);

    // on-disk projects from known VS roots (source\repos, Documents\VS20xx\Projects)
    std::vector<ProjectRecord> ScanDiskProjects();

    // helpers
    std::wstring FindProjectDir(const std::wstring& name);
    std::wstring FindCppForProject(const std::wstring& name, const std::wstring& sourceDir);
    std::wstring FindSlnForProject(const std::wstring& name, const std::wstring& sourceDir);

    // merge VS running instances into a ProjectRecord list (for options / welcome)
    void MergeRunning(std::vector<ProjectRecord>& recents);
}
