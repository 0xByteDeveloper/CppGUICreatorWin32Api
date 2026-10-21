// ============================================================================
//  Store.cpp — persistence implementation
// ============================================================================
#include "Store.h"

namespace Store
{
    static std::wstring LocalAppData()
    {
        wchar_t buf[MAX_PATH] = { 0 };
        DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
        if (n > 0 && n < MAX_PATH) return buf;
        return std::wstring(L"C:\\ProgramData");
    }

    std::wstring BaseDir()
    {
        std::wstring p = LocalAppData() + L"\\CppGUICreatorWin32Api";
        CreateDirDeepW(p);
        return p;
    }
    std::wstring ProjectsFile() { return BaseDir() + L"\\projects.dat"; }
    std::wstring LayoutDir()    { std::wstring p = BaseDir() + L"\\layouts"; CreateDirDeepW(p); return p; }

    std::vector<ProjectRecord> LoadProjects()
    {
        std::string bytes;
        std::vector<ProjectRecord> v;
        if (ReadBinFile(ProjectsFile(), bytes))
            ParseProjects(Utf8ToW(bytes), v);
        return v;
    }

    void SaveProjects(const std::vector<ProjectRecord>& v)
    {
        WriteBinFile(ProjectsFile(), WToUtf8(SerializeProjects(v)), false);
    }

    void UpsertProject(std::vector<ProjectRecord>& v, const ProjectRecord& p)
    {
        for (auto& e : v) {
            if (LowerW(e.name) == LowerW(p.name)) {
                e = p;
                return;
            }
        }
        v.push_back(p);
    }

    ProjectRecord* FindProject(std::vector<ProjectRecord>& v, const std::wstring& name)
    {
        for (auto& e : v)
            if (LowerW(e.name) == LowerW(name)) return &e;
        return nullptr;
    }

    bool LoadLayout(const std::wstring& project, FormInfo& f, std::vector<ControlInstance>& c)
    {
        std::string bytes;
        if (!ReadBinFile(LayoutDir() + L"\\" + project + L".layout", bytes)) return false;
        return ParseLayout(Utf8ToW(bytes), f, c);
    }

    void SaveLayout(const std::wstring& project, const FormInfo& f, const std::vector<ControlInstance>& c)
    {
        WriteBinFile(LayoutDir() + L"\\" + project + L".layout",
                     WToUtf8(SerializeLayout(f, c)), false);
    }

    bool LoadBundledLayout(const std::wstring& project, FormInfo& f, std::vector<ControlInstance>& c)
    {
        std::wstring p = ExeDirW() + L"\\assets\\default_" + project + L".layout";
        std::string bytes;
        if (!ReadBinFile(p, bytes)) return false;
        return ParseLayout(Utf8ToW(bytes), f, c);
    }

    void SortRecents(std::vector<ProjectRecord>& v)
    {
        SortProjectsNewestFirst(v);
    }
}
