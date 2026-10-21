// ============================================================================
//  Store.h — persistence: recent projects + per-project layouts
// ============================================================================
#pragma once
#include "Model.h"

namespace Store
{
    // %LOCALAPPDATA%\CppGUICreatorWin32Api
    std::wstring BaseDir();
    std::wstring ProjectsFile();
    std::wstring LayoutDir();

    std::vector<ProjectRecord> LoadProjects();
    void SaveProjects(const std::vector<ProjectRecord>& v);
    void UpsertProject(std::vector<ProjectRecord>& v, const ProjectRecord& p); // by name
    ProjectRecord* FindProject(std::vector<ProjectRecord>& v, const std::wstring& name);

    bool LoadLayout(const std::wstring& project, FormInfo& f, std::vector<ControlInstance>& c);
    void SaveLayout(const std::wstring& project, const FormInfo& f, const std::vector<ControlInstance>& c);

    // bundled demo layout from assets/default_<name>.layout (old-project experience)
    bool LoadBundledLayout(const std::wstring& project, FormInfo& f, std::vector<ControlInstance>& c);

    // newest-first sort for the Welcome Page
    void SortRecents(std::vector<ProjectRecord>& v);
}
