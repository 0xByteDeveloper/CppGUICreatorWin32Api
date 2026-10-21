// ============================================================================
//  VSDiscovery.cpp — devenv.exe enumeration & VS project discovery
// ============================================================================
#include "VSDiscovery.h"

// ===========================================================================
// helpers
// ===========================================================================
static std::wstring TrimDots(const std::wstring& s)
{
    return TrimW(s);
}

// Parse the VS window title: "SolutionName - Microsoft Visual Studio"
// also tolerate "(Running)", "(Debugging)", "(Building)" markers.
static std::wstring SolutionFromTitle(const std::wstring& title)
{
    std::wstring t = title;
    size_t p = t.find(L" - Microsoft Visual Studio");
    if (p != std::wstring::npos) t = t.substr(0, p);
    // strip trailing state markers
    const wchar_t* marks[] = { L" (Running)", L" (Debugging)", L" (Building)", L" (Designing)" };
    bool changed = true;
    while (changed) {
        changed = false;
        for (const wchar_t* m : marks) {
            if (EndsWithW(t, m)) { t = t.substr(0, t.size() - wcslen(m)); changed = true; }
        }
    }
    // "Solution (1 of 2)" -> Solution   (multi-solution tab marker)
    size_t par = t.rfind(L" (1 of");
    if (par != std::wstring::npos && EndsWithW(t, L")")) t = t.substr(0, par);
    return TrimDots(t);
}

static std::wstring ProcessExePath(DWORD pid)
{
    std::wstring out;
#ifdef _WIN32
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (h) {
        wchar_t buf[MAX_PATH * 2] = { 0 };
        DWORD sz = MAX_PATH * 2;
        if (QueryFullProcessImageNameW(h, 0, buf, &sz))
            out = buf;
        CloseHandle(h);
    }
#endif
    return out;
}

static long long ProcessStartTime(DWORD pid)
{
#ifdef _WIN32
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return 0;
    FILETIME c, e, k, u;
    long long r = 0;
    if (GetProcessTimes(h, &c, &e, &k, &u)) {
        ULARGE_INTEGER li;
        li.HighPart = c.dwHighDateTime;
        li.LowPart = c.dwLowDateTime;
        r = (long long)((li.QuadPart - 116444736000000000ULL) / 10000000ULL);
    }
    CloseHandle(h);
    return r;
#else
    (void)pid; return 0;
#endif
}

static HWND MainWndForPid(DWORD pid)
{
    // collect all visible top-level windows of this pid, choose largest area
    // (prefer the classic VS main window class)
    struct Pass { DWORD pid; std::vector<HWND>* out; } p = { pid, nullptr };
    std::vector<HWND> candidates;
    p.out = &candidates;
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* pp = (Pass*)lp;
        DWORD wpid = 0;
        GetWindowThreadProcessId(h, &wpid);
        if (wpid != pp->pid) return TRUE;
        if (!IsWindowVisible(h)) return TRUE;
        if (GetWindow(h, GW_OWNER)) return TRUE;
        wchar_t title[64] = { 0 };
        if (!GetWindowTextW(h, title, 64)) return TRUE;
        pp->out->push_back(h);
        return TRUE;
    }, (LPARAM)&p);
    HWND best = nullptr;
    long bestArea = -1;
    for (HWND h : candidates) {
        RECT rc;
        if (!GetWindowRect(h, &rc)) continue;
        long area = (rc.right - rc.left) * (rc.bottom - rc.top);
        wchar_t cls[64] = { 0 };
        GetClassNameW(h, cls, 64);
        bool isMain = wcscmp(cls, L"windowsApp") == 0 ||
                      StartsWithW(cls, L"HwndWrapper[DefaultDomain");
        if (isMain) area += 1 << 30;
        if (area > bestArea) { bestArea = area; best = h; }
    }
    return best;
}

// ===========================================================================
// Known VS roots
// ===========================================================================
static std::vector<std::wstring> KnownRoots()
{
    std::vector<std::wstring> roots;
    wchar_t buf[MAX_PATH] = { 0 };
    DWORD n = GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
    std::wstring home = (n > 0 && n < MAX_PATH) ? buf : L"C:\\Users\\Public";

    std::wstring docs = home + L"\\Documents";
    roots.push_back(home + L"\\source\\repos");
    roots.push_back(docs + L"\\Visual Studio 2019\\Projects");
    roots.push_back(docs + L"\\Visual Studio 2022\\Projects");
    roots.push_back(docs + L"\\Visual Studio 2017\\Projects");
    roots.push_back(home + L"\\Desktop");
    roots.push_back(docs);
    return roots;
}

static void FindFilesRecursive(const std::wstring& dir, const std::wstring& ext,
                               int depth, std::vector<std::wstring>& out)
{
    if (depth > 4) return;
#ifdef _WIN32
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") continue;
        std::wstring full = dir + L"\\" + name;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            FindFilesRecursive(full, ext, depth + 1, out);
        } else if (EndsWithW(LowerW(name), ext)) {
            out.push_back(full);
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
#else
    (void)dir; (void)ext; (void)depth; (void)out;
#endif
}

std::wstring VSDiscovery::FindSlnForProject(const std::wstring& name, const std::wstring& sourceDir)
{
    std::vector<std::wstring> dirs;
    if (!sourceDir.empty()) dirs.push_back(sourceDir);
    else dirs.push_back(FindProjectDir(name));
    for (const auto& d : dirs) {
        if (d.empty()) continue;
        std::vector<std::wstring> found;
        FindFilesRecursive(d, L".sln", 0, found);
        for (const auto& f : found)
            if (LowerW(NameFromPathW(f)) == LowerW(name + L".sln")) return f;
        if (!found.empty()) return found[0];
    }
    return L"";
}

std::wstring VSDiscovery::FindProjectDir(const std::wstring& name)
{
    for (const std::wstring& root : KnownRoots()) {
        std::wstring cand = root + L"\\" + name;
        if (DirExistsW(cand)) return cand;
    }
    return L"";
}

std::wstring VSDiscovery::FindCppForProject(const std::wstring& name, const std::wstring& sourceDir)
{
    std::wstring dir = sourceDir.empty() ? FindProjectDir(name) : sourceDir;
    if (dir.empty()) return L"";
    std::vector<std::wstring> cpps;
    FindFilesRecursive(dir, L".cpp", 0, cpps);
    if (cpps.empty()) return L"";
    // prefer: exact "<name>.cpp" or "MyForm.cpp" or main.cpp
    for (const auto& f : cpps) {
        std::wstring n = LowerW(NameFromPathW(f));
        if (n == LowerW(name + L".cpp")) return f;
    }
    for (const auto& f : cpps) {
        std::wstring n = LowerW(NameFromPathW(f));
        if (n == L"myform.cpp" || n == L"main.cpp" || n == L"stdafx.cpp") return f;
    }
    // prefer one in the shallowest folder
    std::wstring best = cpps[0];
    size_t bestDepth = std::count(best.begin(), best.end(), L'\\');
    for (const auto& f : cpps) {
        size_t d = std::count(f.begin(), f.end(), L'\\');
        if (d < bestDepth) { bestDepth = d; best = f; }
    }
    return best;
}

// ===========================================================================
// ScanRunning
// ===========================================================================
std::vector<VSInstance> VSDiscovery::ScanRunning()
{
    std::vector<VSInstance> out;
#ifdef _WIN32
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return out;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (LowerW(pe.szExeFile) == L"devenv.exe") {
                VSInstance v;
                v.pid = pe.th32ProcessID;
                v.hwnd = MainWndForPid(v.pid);
                if (v.hwnd) {
                    wchar_t title[1024] = { 0 };
                    GetWindowTextW(v.hwnd, title, 1024);
                    v.title = title;
                    v.solutionName = SolutionFromTitle(v.title);
                }
                v.exePath = ProcessExePath(v.pid);
                v.startedUnix = ProcessStartTime(v.pid);
                Enrich(v);
                out.push_back(v);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
#endif
    return out;
}

void VSDiscovery::Enrich(VSInstance& v)
{
    if (v.solutionName.empty()) return;
    v.sourceDir = FindProjectDir(v.solutionName);
    v.slnPath   = FindSlnForProject(v.solutionName, v.sourceDir);
    if (!v.slnPath.empty() && v.sourceDir.empty())
        v.sourceDir = DirFromPathW(v.slnPath);
    v.cppPath   = FindCppForProject(v.solutionName, v.sourceDir);
}

// ===========================================================================
// ScanDiskProjects
// ===========================================================================
std::vector<ProjectRecord> VSDiscovery::ScanDiskProjects()
{
    std::vector<ProjectRecord> out;
    for (const std::wstring& root : KnownRoots()) {
        if (!DirExistsW(root)) continue;
        std::vector<std::wstring> slns;
        FindFilesRecursive(root, L".sln", 1, slns);
        for (const auto& s : slns) {
            ProjectRecord p;
            std::wstring fn = NameFromPathW(s);
            p.name = fn.substr(0, fn.size() - 4);
            p.slnPath = s;
            p.sourceDir = DirFromPathW(s);
            p.lastOpened = FileMtimeUnix(s);
            p.created = p.lastOpened;
            p.cppPath = FindCppForProject(p.name, p.sourceDir);
            out.push_back(p);
        }
    }
    return out;
}

// ===========================================================================
// MergeRunning — upsert running VS instances into recents
// ===========================================================================
void VSDiscovery::MergeRunning(std::vector<ProjectRecord>& recents)
{
    auto running = ScanRunning();
    for (const auto& v : running) {
        if (v.solutionName.empty()) continue;
        ProjectRecord p;
        p.name = v.solutionName;
        p.fromVS = true;
        p.pid = (int)v.pid;
        p.slnPath = v.slnPath;
        p.sourceDir = v.sourceDir;
        p.cppPath = v.cppPath;
        p.lastOpened = NowUnix();
        p.created = v.startedUnix ? v.startedUnix : NowUnix();
        // upsert preserving older created
        bool found = false;
        for (auto& e : recents) {
            if (LowerW(e.name) == LowerW(p.name)) {
                e.fromVS = true; e.pid = p.pid;
                if (p.slnPath.size() > e.slnPath.size()) e.slnPath = p.slnPath;
                if (p.sourceDir.size() > e.sourceDir.size()) e.sourceDir = p.sourceDir;
                if (p.cppPath.size() > e.cppPath.size()) e.cppPath = p.cppPath;
                e.lastOpened = NowUnix();
                found = true;
                break;
            }
        }
        if (!found) recents.push_back(p);
    }
}
