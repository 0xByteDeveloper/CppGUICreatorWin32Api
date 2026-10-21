// ============================================================================
//  DTEBridge.cpp — EnvDTE late binding implementation (IDispatch only)
// ============================================================================
#include "DTEBridge.h"

// ---------------------------------------------------------------------------
// NOTE: <wingdi.h>'s "GetObject" macro is active while <objidl.h> is parsed,
// so IRunningObjectTable's GetObject member is declared pre-renamed as
// GetObjectW (UNICODE) / GetObjectA. This holds for MSVC and MinGW alike.
#ifdef UNICODE
#define ROT_GETOBJECT(rot, mk, out) (rot)->GetObjectW((mk), (out))
#else
#define ROT_GETOBJECT(rot, mk, out) (rot)->GetObjectA((mk), (out))
#endif

namespace DTE
{
    // -------------------------------------------------------------------------
    // tiny IDispatch helper
    // -------------------------------------------------------------------------
    static HRESULT GetDispId(IDispatch* d, const wchar_t* name, DISPID* id)
    {
        LPOLESTR names[1] = { (LPOLESTR)name };
        return d->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, id);
    }

    static HRESULT InvokeN(IDispatch* d, DISPID id, WORD flags,
                           VARIANT* result, int argc, VARIANT* argv)
    {
        DISPPARAMS dp;
        memset(&dp, 0, sizeof(dp));
        dp.cArgs = argc;
        dp.rgvarg = argc ? argv : nullptr;
        // named args not needed
        EXCEPINFO ex;
        memset(&ex, 0, sizeof(ex));
        UINT argErr = 0;
        HRESULT hr = d->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                               &dp, result, &ex, &argErr);
        return hr;
    }

    static HRESULT InvokeName(IDispatch* d, const wchar_t* name, WORD flags,
                              VARIANT* result, int argc, VARIANT* argv)
    {
        DISPID id = DISPID_UNKNOWN;
        HRESULT hr = GetDispId(d, name, &id);
        if (FAILED(hr)) return hr;
        return InvokeN(d, id, flags, result, argc, argv);
    }

    static VARIANT PropGet(IDispatch* d, const wchar_t* name)
    {
        VARIANT r;
        VariantInit(&r);
        InvokeName(d, name, DISPATCH_PROPERTYGET, &r, 0, nullptr);
        return r;
    }

    static VARIANT PropGet1(IDispatch* d, const wchar_t* name, VARIANT a1)
    {
        VARIANT r;
        VariantInit(&r);
        InvokeName(d, name, DISPATCH_PROPERTYGET | DISPATCH_METHOD, &r, 1, &a1);
        return r;
    }

    static BSTR BstrArg(const std::wstring& s)
    {
        return SysAllocStringLen(s.c_str(), (UINT)s.size());
    }

    static void FreeArgs(std::vector<VARIANT>& v)
    {
        for (auto& x : v) VariantClear(&x);
    }

    // -------------------------------------------------------------------------
    // ROT: find DTE for a given devenv pid (or first DTE when pid==0)
    // -------------------------------------------------------------------------
    static IDispatch* FindDTE(DWORD pid)
    {
        IDispatch* found = nullptr;
        IRunningObjectTable* rot = nullptr;
        IEnumMoniker* em = nullptr;
        if (FAILED(GetRunningObjectTable(0, &rot)) || !rot) return nullptr;
        if (FAILED(rot->EnumRunning(&em)) || !em) { rot->Release(); return nullptr; }

        IMoniker* mk = nullptr;
        while (em->Next(1, &mk, nullptr) == S_OK && !found) {
            IBindCtx* bc = nullptr;
            if (SUCCEEDED(CreateBindCtx(0, &bc)) && bc) {
                LPOLESTR disp = nullptr;
                if (SUCCEEDED(mk->GetDisplayName(bc, nullptr, &disp)) && disp) {
                    std::wstring name = disp;
                    // display name: "!VisualStudio.DTE.16.0:1234"
                    if (name.find(L"VisualStudio.DTE") != std::wstring::npos) {
                        bool pidOk = (pid == 0);
                        if (!pidOk) {
                            std::wstring suffix = L":" + std::to_wstring((unsigned long)pid);
                            if (name.size() >= suffix.size() &&
                                name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
                                pidOk = true;
                        }
                        if (pidOk) {
                            IUnknown* unk = nullptr;
                            if (SUCCEEDED(ROT_GETOBJECT(rot, mk, &unk)) && unk) {
                                unk->QueryInterface(IID_IDispatch, (void**)&found);
                                unk->Release();
                            }
                        }
                    }
                    CoTaskMemFree(disp);
                }
                bc->Release();
            }
            mk->Release();
        }
        em->Release();
        rot->Release();
        return found;
    }

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------
    void ActivateVS(HWND vsMainWnd)
    {
        if (!vsMainWnd || !IsWindow(vsMainWnd)) return;
        HWND prev = GetForegroundWindow();
        DWORD pid = 0;
        GetWindowThreadProcessId(vsMainWnd, &pid);
        DWORD cur = GetCurrentThreadId();
        DWORD fore = GetWindowThreadProcessId(prev, nullptr);
        if (prev != vsMainWnd) {
            AttachThreadInput(cur, fore, TRUE);
            SetForegroundWindow(vsMainWnd);
            BringWindowToTop(vsMainWnd);
            AttachThreadInput(cur, fore, FALSE);
        }
    }

    bool OpenFileAtLine(HWND vsMainWnd, DWORD pid,
                        const std::wstring& file, int line,
                        std::wstring* err)
    {
        bool ok = false;
        IDispatch* dte = FindDTE(pid);
        if (!dte) {
            // fall back: any running DTE
            dte = FindDTE(0);
        }
        if (!dte) {
            if (err) *err = L"DTE bulunamadi (Running Object Table bos)";
            return false;
        }

        do {
            // ItemOperations.OpenFile(file, "") -> Window
            IDispatch* itemOps = nullptr;
            {
                VARIANT v = PropGet(dte, L"ItemOperations");
                if (v.vt == VT_DISPATCH && v.pdispVal) itemOps = v.pdispVal;
                else VariantClear(&v);
            }
            if (!itemOps) { if (err) *err = L"ItemOperations alinamadi"; break; }

            HWND hw = vsMainWnd;
            {
                VARIANT v = PropGet(dte, L"MainWindow");
                if (v.vt == VT_DISPATCH && v.pdispVal) {
                    VARIANT hwv = PropGet(v.pdispVal, L"HWnd");
                    if (hwv.vt == VT_I4) hw = (HWND)(intptr_t)hwv.lVal;
                    else if (hwv.vt == VT_I8) hw = (HWND)(intptr_t)hwv.llVal;
                    VariantClear(&hwv);
                    v.pdispVal->Release();
                }
                VariantClear(&v);
            }

            std::vector<VARIANT> args;
            VARIANT a0, a1;
            VariantInit(&a0); VariantInit(&a1);
            a0.vt = VT_BSTR; a0.bstrVal = BstrArg(file);
            a1.vt = VT_BSTR; a1.bstrVal = BstrArg(L"");   // default view
            args.push_back(a1);
            args.push_back(a0); // reversed order for DISPPARAMS
            VARIANT winVar;
            VariantInit(&winVar);
            HRESULT hr = InvokeName(itemOps, L"OpenFile",
                                    DISPATCH_METHOD, &winVar,
                                    (int)args.size(), args.data());
            FreeArgs(args);
            if (FAILED(hr) || !(winVar.vt == VT_DISPATCH && winVar.pdispVal)) {
                if (err) *err = L"OpenFile basarisiz";
                itemOps->Release();
                break;
            }
            itemOps->Release();

            // Window.Document -> Document
            IDispatch* doc = nullptr;
            {
                VARIANT v = PropGet(winVar.pdispVal, L"Document");
                if (v.vt == VT_DISPATCH && v.pdispVal) doc = v.pdispVal;
                else VariantClear(&v);
            }
            winVar.pdispVal->Release();
            if (doc) {
                // Document.Object("TextDocument") -> TextDocument
                IDispatch* textDoc = nullptr;
                {
                    VARIANT key;
                    VariantInit(&key);
                    key.vt = VT_BSTR; key.bstrVal = BstrArg(L"TextDocument");
                    VARIANT v = PropGet1(doc, L"Object", key);
                    VariantClear(&key);
                    if (v.vt == VT_DISPATCH && v.pdispVal) textDoc = v.pdispVal;
                    else VariantClear(&v);
                }
                doc->Release();
                if (textDoc) {
                    IDispatch* sel = nullptr;
                    {
                        VARIANT v = PropGet(textDoc, L"Selection");
                        if (v.vt == VT_DISPATCH && v.pdispVal) sel = v.pdispVal;
                        else VariantClear(&v);
                    }
                    textDoc->Release();
                    if (sel) {
                        VARIANT aL; VariantInit(&aL);
                        VARIANT aO; VariantInit(&aO);
                        VARIANT aE; VariantInit(&aE);
                        aL.vt = VT_I4; aL.lVal = line > 0 ? line : 1;
                        aO.vt = VT_I4; aO.lVal = 1;
                        aE.vt = VT_BOOL; aE.boolVal = VARIANT_FALSE; // extend=false
                        VARIANT argv[3] = { aE, aO, aL };            // reversed
                        VARIANT res; VariantInit(&res);
                        HRESULT hr2 = InvokeName(sel, L"MoveToLineAndOffset",
                                                 DISPATCH_METHOD, &res, 3, argv);
                        VariantClear(&aL); VariantClear(&aO); VariantClear(&aE);
                        VariantClear(&res);
                        sel->Release();
                        ok = SUCCEEDED(hr2);
                        if (!ok && err) *err = L"MoveToLineAndOffset basarisiz";
                    } else if (err) *err = L"Selection alinamadi";
                } else if (err) *err = L"TextDocument alinamadi";
            }
            // Activate VS window last (even if scroll failed we opened the file)
            ActivateVS(hw ? hw : vsMainWnd);
            ok = true; // file opened counts as success for our purposes
        } while (false);

        dte->Release();
        return ok;
    }
}
