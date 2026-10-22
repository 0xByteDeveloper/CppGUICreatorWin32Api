// vsint.cpp — Visual Studio entegrasyonu (Bölüm 1: DTE late-binding + ROT)
#include "vsint.h"
#include "app.h"
#include "codegen.h"
#include <objbase.h>
#include <shlobj.h>
#include <tlhelp32.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shell32.lib")

namespace vsint {

// ---------------------------------------------------------------------------
// IDispatch geç bağlama yardımcıları
// ---------------------------------------------------------------------------
struct Disp {
    IDispatch* d = nullptr;
    Disp() {}
    explicit Disp(IDispatch* p) : d(p) {}
    operator bool() const { return d != nullptr; }

    DISPID IdOf(const wchar_t* name) {
        if (!d) return DISPID_UNKNOWN;
        DISPID id = DISPID_UNKNOWN;
        LPOLESTR n = (LPOLESTR)name;
        if (FAILED(d->GetIDsOfNames(IID_NULL, &n, 1, LOCALE_USER_DEFAULT, &id))) return DISPID_UNKNOWN;
        return id;
    }
    HRESULT Invoke(DISPID id, WORD kind, VARIANT* args, int argc, VARIANT* ret) {
        if (!d) return E_FAIL;
        DISPPARAMS dp = { args, nullptr, (UINT)max(0, argc), 0 };
        // Invoke ters sırada ister
        if (argc > 1) {
            std::vector<VARIANT> rev(args, args + argc);
            std::reverse(rev.begin(), rev.end());
            dp.rgvarg = rev.data();
        }
        EXCEPINFO ex = { 0 };
        UINT err = 0;
        HRESULT hr = d->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, kind, &dp, ret, &ex, &err);
        if (ex.bstrDescription) SysFreeString(ex.bstrDescription);
        if (ex.bstrSource) SysFreeString(ex.bstrSource);
        if (ex.bstrHelpFile) SysFreeString(ex.bstrHelpFile);
        return hr;
    }
    VARIANT Get(const wchar_t* name) {
        VARIANT v; VariantInit(&v);
        DISPID id = IdOf(name);
        if (id != DISPID_UNKNOWN) Invoke(id, DISPATCH_PROPERTYGET, nullptr, 0, &v);
        return v;
    }
    VARIANT Call(const wchar_t* name, VARIANT* args = nullptr, int argc = 0) {
        VARIANT v; VariantInit(&v);
        DISPID id = IdOf(name);
        if (id != DISPID_UNKNOWN) Invoke(id, DISPATCH_METHOD | DISPATCH_PROPERTYGET, args, argc, &v);
        return v;
    }
    void Put(const wchar_t* name, VARIANT& val) {
        DISPID id = IdOf(name);
        if (id == DISPID_UNKNOWN) return;
        DISPPARAMS dp = { &val, nullptr, 1, 1 };
        DISPID pid = DISPID_PROPERTYPUT;
        dp.cNamedArgs = 1;
        dp.rgdispidNamedArgs = &pid;
        EXCEPINFO ex = { 0 };
        d->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dp, nullptr, &ex, nullptr);
    }
    static VARIANT BStr(const wchar_t* s) {
        VARIANT v; VariantInit(&v);
        v.vt = VT_BSTR;
        v.bstrVal = SysAllocString(s);
        return v;
    }
    static VARIANT I4(long l) {
        VARIANT v; VariantInit(&v);
        v.vt = VT_I4; v.lVal = l;
        return v;
    }
    static std::wstring Str(const VARIANT& v) {
        if (v.vt == VT_BSTR && v.bstrVal) return v.bstrVal;
        if (v.vt == VT_DISPATCH && v.pdispVal) {
            Disp dd(v.pdispVal);
            VARIANT s = dd.Get(L"FullName");
            if (s.vt == VT_BSTR && s.bstrVal) return s.bstrVal;
        }
        return L"";
    }
};

// ---------------------------------------------------------------------------
// Bağlantı durumu
// ---------------------------------------------------------------------------
static VSInstance g_conn;

bool IsConnected() { return !g_conn.projectPath.empty() || !g_conn.solutionName.empty(); }
VSInstance& Current() { return g_conn; }

void Disconnect() {
    if (g_conn.dte) { g_conn.dte->Release(); g_conn.dte = nullptr; }
    g_conn = VSInstance();
    Status(L"Visual Studio bağlantısı kesildi");
}

// ---------------------------------------------------------------------------
// ROT: çalışan DTE örnekleri
// ---------------------------------------------------------------------------
static bool CollectDteFromROT(std::vector<VSInstance>& out) {
    IRunningObjectTable* rot = nullptr;
    IEnumMoniker* em = nullptr;
    IBindCtx* bc = nullptr;
    if (FAILED(CreateBindCtx(0, &bc))) return false;
    if (FAILED(GetRunningObjectTable(0, &rot))) { bc->Release(); return false; }
    rot->EnumRunning(&em);
    IMoniker* mk = nullptr;
    while (em && em->Next(1, &mk, nullptr) == S_OK) {
        LPOLESTR disp = nullptr;
        if (SUCCEEDED(mk->GetDisplayName(bc, nullptr, &disp)) && disp) {
            std::wstring name = disp;
            if (name.find(L"VisualStudio.DTE.") != std::wstring::npos) {
                // versiyonu ayıkla
                size_t p = name.find(L"VisualStudio.DTE.");
                std::wstring ver = name.substr(p + wcslen(L"VisualStudio.DTE."));
                size_t sp = ver.find(L'!');
                if (sp != std::wstring::npos) ver = ver.substr(0, sp);
                IUnknown* unk = nullptr;
                if (SUCCEEDED(rot->GetObject(mk, &unk)) && unk) {
                    IDispatch* dte = nullptr;
                    if (SUCCEEDED(unk->QueryInterface(IID_IDispatch, (void**)&dte)) && dte) {
                        VSInstance inst;
                        inst.hasDte = true;
                        inst.dte = dte;   // sahiplik instance'ta (QI ref)
                        inst.progId = L"VisualStudio.DTE." + ver;
                        inst.version = ver;
                        Disp d(dte);
                        VARIANT sol = d.Get(L"Solution");
                        if (sol.vt == VT_DISPATCH && sol.pdispVal) {
                            Disp ds(sol.pdispVal);
                            VARIANT fn = ds.Get(L"FullName");
                            inst.solutionPath = Disp::Str(fn);
                            if (fn.vt == VT_BSTR) VariantClear(&fn);
                        }
                        if (sol.vt == VT_BSTR) inst.solutionPath = sol.bstrVal;
                        VariantClear(&sol);
                        if (inst.solutionPath.empty()) {
                            inst.solutionName = L"(çözüm yok)";
                        } else {
                            // "Foo.sln" → Foo
                            std::wstring fn = inst.solutionPath;
                            size_t q = fn.find_last_of(L"\\/");
                            inst.solutionName = GetFileName(fn);
                            size_t dot = inst.solutionName.rfind(L'.');
                            if (dot != std::wstring::npos) inst.solutionName = inst.solutionName.substr(0, dot);
                        }
                        VARIANT mw = d.Get(L"MainWindow");
                        if (mw.vt == VT_DISPATCH && mw.pdispVal) {
                            Disp dmw(mw.pdispVal);
                            VARIANT hw = dmw.Get(L"HWnd");
                            if (hw.vt == VT_I4) inst.hwnd = (HWND)(INT_PTR)hw.lVal;
                            VariantClear(&hw);
                        }
                        VariantClear(&mw);
                        // PID: ProcessID property
                        VARIANT pidv = d.Get(L"ProcessID");
                        if (pidv.vt == VT_I4) inst.pid = (DWORD)pidv.lVal;
                        VariantClear(&pidv);
                        out.push_back(inst);
                        // dte referansı inst'te tutuldu
                    }
                    unk->Release();
                }
            }
            CoTaskMemFree(disp);
        }
        mk->Release();
    }
    em->Release();
    rot->Release();
    bc->Release();
    return true;
}

// DTE bulunamayan (veya COM hata veren) örnekler için pencere taraması
static void CollectFromWindows(std::vector<VSInstance>& out) {
    struct Ctx { std::vector<VSInstance>* v; };
    Ctx ctx{ &out };
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        Ctx* c = (Ctx*)lp;
        wchar_t cls[64] = L"", title[512] = L"";
        GetClassNameW(h, cls, 64);
        if (wcscmp(cls, L" devenv") != 0) return TRUE;   // devenv ana sınıfı öndeki boşlukla gelir
        GetWindowTextW(h, title, 512);
        std::wstring t = title;
        size_t p = t.find(L" - Microsoft Visual Studio");
        if (p == std::wstring::npos) return TRUE;
        DWORD pid = 0;
        GetWindowThreadProcessId(h, &pid);
        std::wstring solName = t.substr(0, p);
        // " (Running)" eki temizle
        size_t rp = solName.find(L" (");
        if (rp != std::wstring::npos) solName = solName.substr(0, rp);
        // zaten var mı?
        for (auto& i : *c->v) if (i.pid == pid) return TRUE;
        VSInstance inst;
        inst.pid = pid; inst.hwnd = h; inst.solutionName = solName;
        inst.hasDte = false;
        c->v->push_back(inst);
        return TRUE;
    }, (LPARAM)&ctx);
}

std::vector<VSInstance> Enumerate() {
    std::vector<VSInstance> out;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    CollectDteFromROT(out);
    CollectFromWindows(out);
    // pid 0 olanları eşle
    for (auto& i : out) {
        if (!i.pid && i.hwnd) { DWORD p = 0; GetWindowThreadProcessId(i.hwnd, &p); i.pid = p; }
    }
    return out;
}

// --- proje yolu çözümleme ------------------------------------------------------
// Solution path → içindeki .vcxproj'ları bul (basit .sln ayrıştırma)
std::vector<std::wstring> EnumerateProjectsOfSolution(const std::wstring& slnPath) {
    std::vector<std::wstring> out;
    std::string data;
    if (!ReadFileText(slnPath, data)) return out;
    std::wstring text = Utf8ToWide(data);
    size_t pos = 0;
    while ((pos = text.find(L"Project(", pos)) != std::wstring::npos) {
        size_t q1 = text.find(L'"', pos);
        if (q1 == std::wstring::npos) break;
        size_t q2 = text.find(L'"', q1 + 1);           // proje adı
        size_t q3 = text.find(L'"', q2 + 1);
        size_t q4 = text.find(L'"', q3 + 1);           // yol
        if (q4 == std::wstring::npos) break;
        std::wstring rel = text.substr(q3 + 1, q4 - q3 - 1);
        pos = q4;
        if (rel.size() > 8 && _wcsicmp(rel.substr(rel.size() - 8).c_str(), L".vcxproj") == 0) {
            std::wstring dir = GetFileDir(slnPath);
            std::wstring full = dir + L"\\" + rel;
            // "..\\" normalize et
            std::vector<std::wstring> parts = Split(full, L'\\');
            std::vector<std::wstring> norm;
            for (auto& p : parts) {
                if (p == L"..") { if (!norm.empty()) norm.pop_back(); }
                else if (p == L".") {}
                else norm.push_back(p);
            }
            out.push_back(Join(norm, L"\\"));
        }
    }
    return out;
}

bool Connect(VSInstance& inst, const std::wstring& projectPath) {
    g_conn = inst;
    g_conn.projectPath = projectPath;
    g_conn.projectName = GetFileName(projectPath);
    if (!inst.solutionPath.empty()) {
        auto projs = EnumerateProjectsOfSolution(inst.solutionPath);
        (void)projs;
    }
    return true;
}

// ===PART2===

// ---------------------------------------------------------------------------
// Dosya + proje entegrasyonu
// ---------------------------------------------------------------------------
// .vcxproj'a Form1.h/.cpp girdilerini ekle (yoksa)
static bool InjectIntoVcxproj(const std::wstring& vcxprojPath) {
    std::string data;
    if (!ReadFileText(vcxprojPath, data)) return false;
    std::wstring text = Utf8ToWide(data);
    bool changed = false;
    if (text.find(L"Form1.h") == std::wstring::npos) {
        size_t p = text.find(L"</ItemGroup>");
        if (p != std::wstring::npos) {
            text.insert(p, L"  <ClInclude Include=\"Form1.h\" />\n");
            changed = true;
        } else {
            size_t e = text.rfind(L"</Project>");
            if (e != std::wstring::npos) {
                text.insert(e, L"  <ItemGroup>\n    <ClInclude Include=\"Form1.h\" />\n  </ItemGroup>\n");
                changed = true;
            }
        }
    }
    if (text.find(L"Form1.cpp") == std::wstring::npos) {
        size_t p = text.find(L"</ItemGroup>");
        if (p != std::wstring::npos) {
            text.insert(p, L"  <ClCompile Include=\"Form1.cpp\" />\n");
            changed = true;
        } else {
            size_t e = text.rfind(L"</Project>");
            if (e != std::wstring::npos) {
                text.insert(e, L"  <ItemGroup>\n    <ClCompile Include=\"Form1.cpp\" />\n  </ItemGroup>\n");
                changed = true;
            }
        }
    }
    if (changed) WriteFileText(vcxprojPath, WideToUtf8(text));
    return changed;
}

// Projeye Form1 dosyalarını ekle (DTE varsa AddFromFile, yoksa vcxproj yamala)
static bool AddFormFilesToProject(const VSInstance& inst) {
    std::wstring dir = inst.projectPath;
    std::wstring h = dir + L"\\Form1.h";
    std::wstring cpp = dir + L"\\Form1.cpp";
    bool created = false;
    if (!FileExists(h)) { WriteFileText(h, WideToUtf8(codegen::GenerateForm1H())); created = true; }
    if (!FileExists(cpp)) { WriteFileText(cpp, WideToUtf8(codegen::GenerateForm1Cpp())); created = true; }
    if (inst.dte) {
        Disp d(inst.dte);
        VARIANT solv = d.Get(L"Solution");
        if (solv.vt == VT_DISPATCH && solv.pdispVal) {
            Disp ds(solv.pdispVal);
            VARIANT args = Disp::BStr(h.c_str());
            VARIANT it = ds.Call(L"FindProjectItem", &args, 1);
            VariantClear(&args);
            if (it.vt == VT_DISPATCH && it.pdispVal) {
                Disp di(it.pdispVal);
                di.Call(L"Expand");   // Solution Explorer'da göster
            }
            VariantClear(&it);
        }
        VariantClear(&solv);
    }
    // her durumda vcxproj güncel olsun (VS harici değişikliği algılayıp yeniden yükler)
    InjectIntoVcxproj(inst.projectPath);
    return created;
}

bool SyncToProject(Ctrl* targetCtrl, const std::wstring& eventName, int* handlerLine) {
    if (!IsConnected()) return false;
    // VS'de açık belgeler varsa önce kaydet (çakışmayı önle)
    if (g_conn.dte) {
        Disp d(g_conn.dte);
        VARIANT a1 = Disp::BStr(L"File.SaveAll");
        d.Call(L"ExecuteCommand", &a1, 1);
        VariantClear(&a1);
    }
    AddFormFilesToProject(g_conn);
    std::wstring dir = g_conn.projectPath;
    bool ok = codegen::SyncIntoFile(dir + L"\\Form1.cpp", dir + L"\\Form1.h", targetCtrl, eventName, handlerLine);
    if (ok) Status(L"Form1.h/.cpp senkronize edildi → " + g_conn.projectName);
    return ok == 1;
}

void BringToFront(const VSInstance& inst) {
    HWND h = inst.hwnd;
    if (!h) {
        // pencereyi bul
        struct F { DWORD pid; HWND h; } f = { inst.pid, nullptr };
        EnumWindows([](HWND hh, LPARAM lp) -> BOOL {
            F* p = (F*)lp;
            DWORD pid = 0; GetWindowThreadProcessId(hh, &pid);
            if (pid == p->pid) {
                wchar_t cls[64]; GetClassNameW(hh, cls, 64);
                if (wcscmp(cls, L" devenv") == 0) { p->h = hh; return FALSE; }
            }
            return TRUE;
        }, (LPARAM)&f);
        h = f.h;
    }
    if (!h) return;
    // güvenilir öne getirme: AttachThreadInput hilesi
    DWORD fgPid = 0;
    HWND fg = GetForegroundWindow();
    GetWindowThreadProcessId(fg, &fgPid);
    DWORD curT = GetCurrentThreadId();
    DWORD fgT = GetWindowThreadProcessId(fg, nullptr);
    DWORD myT = GetWindowThreadProcessId(h, nullptr);
    if (fg && fgPid != GetCurrentProcessId()) AttachThreadInput(curT, fgT, TRUE);
    AttachThreadInput(myT, curT, TRUE);
    ShowWindow(h, SW_RESTORE);
    SetForegroundWindow(h);
    BringWindowToTop(h);
    AttachThreadInput(myT, curT, FALSE);
    if (fg && fgPid != GetCurrentProcessId()) AttachThreadInput(curT, fgT, FALSE);
}

bool EnsureHandlerAndGoto(Ctrl* targetCtrl, const std::wstring& eventName) {
    if (!targetCtrl && !g_model.formSelected) return false;
    if (!IsConnected()) {
        Status(L"Visual Studio bağlı değil — handler tasarımda kaydedildi");
        return false;
    }
    std::wstring hname;
    if (targetCtrl) hname = g_model.HandlerNameOf(targetCtrl, eventName);
    else hname = L"Form1_" + eventName;
    int line = 0;
    if (!SyncToProject(targetCtrl, eventName, &line)) return false;
    // VS'yi öne getir + satıra git
    BringToFront(g_conn);
    if (g_conn.dte && line > 0) {
        Sleep(150);
        Disp d(g_conn.dte);
        std::wstring dir = g_conn.projectPath;
        // ItemOperations.OpenFile(Form1.cpp)
        VARIANT a1 = Disp::BStr((dir + L"\\Form1.cpp").c_str());
        VARIANT iov = d.Get(L"ItemOperations");
        if (iov.vt == VT_DISPATCH && iov.pdispVal) {
            Disp dIo(iov.pdispVal);
            dIo.Call(L"OpenFile", &a1, 1);
        }
        VariantClear(&a1); VariantClear(&iov);
        // ExecuteCommand("Edit.GoTo", satır)  →  Invoke ters sıralar → rgvarg[0]=satır olmalı
        wchar_t ln[16]; swprintf(ln, 16, L"%d", line);
        VARIANT c1 = Disp::BStr(L"Edit.GoTo");
        VARIANT c2 = Disp::BStr(ln);
        VARIANT args[2] = { c1, c2 };
        d.Call(L"ExecuteCommand", args, 2);
        VariantClear(&c1); VariantClear(&c2);
        Status(L"VS: Form1.cpp satır " + std::to_wstring(line) + L" → " + hname);
    } else if (line > 0 && g_conn.hwnd) {
        // DTE yok: Ctrl+G tuşla
        Sleep(200);
        INPUT in[40] = { 0 };
        int idx = 0;
        auto key = [&](WORD vk, BOOL up) {
            if (idx >= 38) return;
            in[idx].type = INPUT_KEYBOARD;
            in[idx].ki.wVk = vk;
            in[idx].ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
            idx++;
        };
        // Ctrl+G
        key(VK_CONTROL, FALSE); key('G', FALSE); key('G', TRUE); key(VK_CONTROL, TRUE);
        // satır rakamları + Enter
        wchar_t buf[16]; swprintf(buf, 16, L"%d", line);
        for (wchar_t* p = buf; *p; p++) {
            WORD vk = 0x30 + (WORD)(*p - L'0');
            key(vk, FALSE); key(vk, TRUE);
        }
        key(VK_RETURN, FALSE); key(VK_RETURN, TRUE);
        SendInput((UINT)idx, in, sizeof(INPUT));
        Status(L"VS (pencere modu): Ctrl+G ile satır " + std::to_wstring(line));
    }
    return true;
}

} // namespace vsint
