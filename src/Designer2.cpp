// ============================================================================
//  Designer2.cpp — Props panel, Host orchestration, DesignerNS public API
// ============================================================================
#include "DesignerInternal.h"

namespace DesignerNS
{
    // =========================================================================
    // Props
    // =========================================================================
    void Props::RebuildRows()
    {
        rows.clear();
        Canvas& cv = *host->canvas;
        bool isForm = (cv.selected < 0);
        RECT rc; GetClientRect(hwnd, &rc);
        float scale = ScaleF(hwnd);
        float pad = 12 * scale;
        float rowH = 24 * scale;
        float y = 8 * scale;

        Row head;
        head.header = true; head.isEvent = false; head.evIndex = -1;
        head.key = isForm ? (L"Form: " + host->project)
                          : (L"Kontrol: " + cv.ctrls[cv.selected].name);
        head.rc = { 0, (int)y, rc.right, (int)(y + 30 * scale) };
        rows.push_back(head);
        y += 34 * scale;

        // ---- properties ----
        Row sec; sec.header = true; sec.isEvent = false; sec.evIndex = -1;
        sec.key = L"Özellikler";
        sec.rc = { 0, (int)y, rc.right, (int)(y + 22 * scale) };
        rows.push_back(sec);
        y += 24 * scale;

        // structural props (always)
        auto addProp = [&](const std::wstring& key, const std::wstring& val) {
            Row r; r.isEvent = false; r.evIndex = -1;
            r.key = key; r.val = val;
            r.rc = { 0, (int)y, rc.right, (int)(y + rowH) };
            rows.push_back(r);
            y += rowH;
        };
        if (isForm) {
            addProp(L"Name", host->project);
            addProp(L"Text", cv.form.text);
            addProp(L"Width", std::to_wstring(cv.form.w));
            addProp(L"Height", std::to_wstring(cv.form.h));
            addProp(L"BackColor", cv.form.backColor);
            for (const auto& p : Catalog::PropListFor(L"Form"))
                addProp(p, L"");
        } else {
            const ControlInstance& c = cv.ctrls[cv.selected];
            addProp(L"Name", c.name);
            addProp(L"Text", c.text);
            addProp(L"X", std::to_wstring(c.x));
            addProp(L"Y", std::to_wstring(c.y));
            addProp(L"Width", std::to_wstring(c.w));
            addProp(L"Height", std::to_wstring(c.h));
            for (const auto& p : Catalog::PropListFor(c.type)) {
                auto it = c.props.find(p);
                addProp(p, it == c.props.end() ? std::wstring() : it->second);
            }
            for (const auto& kv : c.props) {
                bool listed = false;
                for (const auto& p2 : Catalog::PropListFor(c.type))
                    if (p2 == kv.first) { listed = true; break; }
                if (!listed) addProp(kv.first, kv.second);
            }
        }

        // ---- events ----
        y += 6 * scale;
        Row sec2; sec2.header = true; sec2.isEvent = false; sec2.evIndex = -1;
        sec2.key = L"Olaylar (çift tık -> VS'de kod)";
        sec2.rc = { 0, (int)y, rc.right, (int)(y + 22 * scale) };
        rows.push_back(sec2);
        y += 24 * scale;

        if (isForm) {
            std::vector<std::wstring> fev = { L"Load", L"FormClosing", L"Resize", L"Paint" };
            for (size_t i = 0; i < fev.size(); i++) {
                Row r; r.isEvent = true; r.evIndex = (int)i;
                r.key = fev[i];
                r.val = CodeGen::HandlerName(host->project, fev[i]);
                r.rc = { 0, (int)y, rc.right, (int)(y + rowH) };
                rows.push_back(r);
                y += rowH;
            }
        } else {
            const ControlInstance& c = cv.ctrls[cv.selected];
            std::vector<std::wstring> evs = Catalog::EventListFor(c.type);
            for (size_t i = 0; i < evs.size(); i++) {
                Row r; r.isEvent = true; r.evIndex = (int)i;
                r.key = evs[i];
                auto it = c.events.find(evs[i]);
                r.val = (it != c.events.end() && !it->second.empty())
                        ? it->second : L"—";
                r.rc = { 0, (int)y, rc.right, (int)(y + rowH) };
                rows.push_back(r);
                y += rowH;
            }
        }
        // NOTE: no InvalidateRect here — RebuildRows runs inside WM_PAINT too;
        // invalidating from paint caused an infinite repaint loop (100% CPU).
    }

    void Props::BeginEdit(int row)
    {
        if (row < 0 || row >= (int)rows.size()) return;
        Row& r = rows[row];
        if (r.header) return;
        if (r.isEvent) return;          // events: double-click generates code
        CancelEdit();
        editRow = row;
        editIsEvent = false;
        editEvIndex = -1;
        RECT rc = r.rc;
        float scale = ScaleF(hwnd);
        int half = (int)(rc.left + (rc.right - rc.left) * 0.42);
        RECT val = { half + 4, rc.top + 1, rc.right - 4, rc.bottom - 1 };
        hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", r.val.c_str(),
                                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                val.left, val.top, val.right - val.left, val.bottom - val.top,
                                hwnd, nullptr, (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), nullptr);
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)Theme_Font(F_BODY), TRUE);
        SetFocus(hEdit);
    }

    void Props::CommitEdit()
    {
        if (!hEdit || editRow < 0) return;
        wchar_t buf[512] = { 0 };
        GetWindowTextW(hEdit, buf, 512);
        std::wstring val = TrimW(buf);
        Canvas& cv = *host->canvas;
        const std::wstring& key = rows[editRow].key;
        bool isForm = (cv.selected < 0);

        if (isForm) {
            if (key == L"Text") cv.form.text = val;
            else if (key == L"Width") cv.form.w = std::max(160, _wtoi(val.c_str()));
            else if (key == L"Height") cv.form.h = std::max(120, _wtoi(val.c_str()));
            else if (key == L"BackColor" && val.size() == 6) cv.form.backColor = val;
            else if (key != L"Name" && !val.empty())
                cv.form.props[key] = val;   // extra catalog props live here too
        } else {
            ControlInstance& c = cv.ctrls[cv.selected];
            if (key == L"Text") c.text = val;
            else if (key == L"X") c.x = std::max(0, _wtoi(val.c_str()));
            else if (key == L"Y") c.y = std::max(0, _wtoi(val.c_str()));
            else if (key == L"Width") c.w = std::max(4, _wtoi(val.c_str()));
            else if (key == L"Height") c.h = std::max(4, _wtoi(val.c_str()));
            else if (key == L"Name") {
                std::wstring nn = NormalizeIdentW(val);
                if (!nn.empty() && nn != c.name) {
                    // unique check
                    bool used = false;
                    for (auto& o : cv.ctrls)
                        if (&o != &c && LowerW(o.name) == LowerW(nn)) { used = true; break; }
                    if (!used) c.name = nn;
                }
            }
            else if (key == L"Enabled") c.enabled = (val != L"0" && LowerW(val) != L"false");
            else if (key == L"Visible") c.visible = (val != L"0" && LowerW(val) != L"false");
            else if (!val.empty() || c.props.count(key)) c.props[key] = val;
        }
        DestroyWindow(hEdit);
        hEdit = nullptr;
        editRow = -1;
        RebuildRows();
        InvalidateRect(hwnd, nullptr, FALSE);
        host->canvas->formRc = host->canvas->FormRect();
        InvalidateRect(host->canvas->hwnd, nullptr, FALSE);
        host->OnCanvasEdited();
    }

    void Props::CancelEdit()
    {
        if (hEdit) { DestroyWindow(hEdit); hEdit = nullptr; }
        editRow = -1;
    }

    LRESULT Props::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        switch (m) {
            case WM_SIZE:
                RebuildRows();
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_MOUSEMOVE: {
                host->props->hover.On(h, m, l);
                int y = GetYL(l);
                int nh = -1;
                for (size_t i = 0; i < rows.size(); i++)
                    if (PtInRect(&rows[i].rc, POINT{ 0, y }) || (y >= rows[i].rc.top && y < rows[i].rc.bottom))
                        nh = (int)i;
                if (nh != hotRow) {
                    hotRow = nh;
                    InvalidateRect(h, nullptr, FALSE);
                }
                return 0;
            }
            case WM_MOUSELEAVE:
                host->props->hover.On(h, m, l);
                hotRow = -1;
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_LBUTTONDOWN: {
                SetFocus(h);
                int y = GetYL(l);
                for (size_t i = 0; i < rows.size(); i++) {
                    if (y >= rows[i].rc.top && y < rows[i].rc.bottom) {
                        if (!rows[i].header && !rows[i].isEvent)
                            BeginEdit((int)i);
                        return 0;
                    }
                }
                CancelEdit();
                return 0;
            }
            case WM_LBUTTONDBLCLK: {
                int y = GetYL(l);
                for (size_t i = 0; i < rows.size(); i++) {
                    if (y >= rows[i].rc.top && y < rows[i].rc.bottom && rows[i].isEvent) {
                        Canvas& cv = *host->canvas;
                        if (cv.selected < 0) {
                            g_mw->OnFormDblClick(host->project);
                        } else {
                            const ControlInstance& c = cv.ctrls[cv.selected];
                            std::vector<std::wstring> evs = Catalog::EventListFor(c.type);
                            int ei = rows[i].evIndex;
                            std::wstring ev = (ei >= 0 && ei < (int)evs.size()) ? evs[ei] : rows[i].key;
                            g_mw->OnControlDblClick(host->project, c.name, c.type, ev);
                        }
                        return 0;
                    }
                }
                return 0;
            }
            case WM_COMMAND:
                if (HIWORD(w) == EN_KILLFOCUS) {
                    CommitEdit();
                    return 0;
                }
                break;
            case WM_VSCROLL:
            case WM_MOUSEWHEEL:
                scroll = std::max(0, scroll - (GET_WHEEL_DELTA_WPARAM(w) > 0 ? 24 : -24));
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_USER_REFRESH:
                RebuildRows();
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            case WM_PAINT: {
                BufferedPaint bp(h, TC(C_CARD));
                RebuildRows();
                Paint(*bp.g, bp.rc, ScaleF(h));
                return 0;
            }
        }
        return DefWindowProcW(h, m, w, l);
    }

    void Props::Paint(Graphics& g, const RECT& rc, float scale)
    {
        (void)rc;
        float pad = 12 * scale;
        float rowH = 24 * scale;
        FillRectC(g, 0, 0, (float)rc.right, 1, TC(C_BORDER));
        int half = (int)(pad + (rc.right - pad * 2) * 0.42);
        for (size_t i = 0; i < rows.size(); i++) {
            Row& r = rows[i];
            float y = (float)r.rc.top;
            float rh = (float)(r.rc.bottom - r.rc.top);
            if (r.header) {
                if (i == 0) {
                    // object header
                    FillRectC(g, 0, y, (float)rc.right, rh, TC(C_GRAY_SOFT));
                    ThemeText(g, r.key, Theme_Font(F_BODY_SB), TC(C_TEXT),
                              RectF(pad, y, (float)rc.right - pad * 2, rh),
                              StringAlignmentNear, StringAlignmentCenter);
                } else {
                    ThemeText(g, r.key, Theme_Font(F_SMALL_SB), TC(C_TEXT2),
                              RectF(pad, y, (float)rc.right - pad * 2, rh),
                              StringAlignmentNear, StringAlignmentCenter);
                }
                FillRectC(g, 0, y + rh - 1, (float)rc.right, 1, TC(C_BORDER));
                continue;
            }
            bool hot = ((int)i == hotRow);
            if (hot) FillRectC(g, 0, y, (float)rc.right, rh, TC(C_HOVER_SOFT));
            // key half
            ThemeText(g, r.key, Theme_Font(F_SMALL), TC(C_TEXT2),
                      RectF(pad, y, half - pad * 2, rh),
                      StringAlignmentNear, StringAlignmentCenter);
            // value half (light field)
            FillRectC(g, (float)half + 2, y + 2, (float)rc.right - half - 6, rh - 4,
                      (int)i == editRow ? TC(C_WHITE) : TC(C_GRAY_SOFT));
            ThemeText(g, r.isEvent ? r.val : r.val,
                      Theme_Font(r.isEvent ? F_SMALL : F_SMALL),
                      r.isEvent ? (r.val == L"—" ? TC(C_TEXT3) : TC(C_ACCENT)) : TC(C_TEXT),
                      RectF((float)half + 10, y, (float)rc.right - half - 18, rh),
                      StringAlignmentNear, StringAlignmentCenter);
        }
    }

    // =========================================================================
    // Host
    // =========================================================================
    void Host::OnSelectionChanged()
    {
        if (props) SendMessageW(hProps, WM_USER_REFRESH, 0, 0);
    }

    void Host::OnCanvasEdited()
    {
        Store::SaveLayout(project, canvas->form, canvas->ctrls);
        // keep MainWindow recents fresh (lastOpened)
        if (g_mw) {
            ProjectRecord* p = Store::FindProject(g_mw->recents, project);
            if (p) { p->lastOpened = NowUnix(); Store::SortRecents(g_mw->recents);
                     Store::SaveProjects(g_mw->recents); }
        }
    }

    void Host::LoadFromStore()
    {
        // order: user layout -> bundled demo -> blank
        if (!Store::LoadLayout(project, canvas->form, canvas->ctrls)) {
            if (!Store::LoadBundledLayout(project, canvas->form, canvas->ctrls)) {
                canvas->form = FormInfo();
                canvas->form.name = project;
                canvas->form.text = project;
                canvas->ctrls.clear();
            }
        }
        // guarantee ids
        for (auto& c : canvas->ctrls)
            if (!c.idc) c.idc = NextControlId(canvas->ctrls);
        canvas->formRc = canvas->FormRect();
        OnSelectionChanged();
        InvalidateRect(hCanvas, nullptr, FALSE);
    }

    void Host::LayoutChildren()
    {
        RECT rc; GetClientRect(hwnd, &rc);
        int propW = g_mw->propsVisible ? ScaleI(hwnd, 285) : 0;
        if (hCanvas) MoveWindow(hCanvas, 0, 0, rc.right - propW, rc.bottom, TRUE);
        if (hProps)  MoveWindow(hProps, rc.right - propW, 0, propW, rc.bottom, propW > 0);
    }

    // =========================================================================
    // window procs
    // =========================================================================
    static LRESULT HostProc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        if (m == WM_NCCREATE) {
            auto* cs = (CREATESTRUCTW*)l;
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return DefWindowProcW(h, m, w, l);
        }
        auto* self = (Host*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (!self) return DefWindowProcW(h, m, w, l);
        switch (m) {
            case WM_SIZE:
                self->LayoutChildren();
                return 0;
            case WM_ERASEBKGND:
                return 1;
        }
        return DefWindowProcW(h, m, w, l);
    }

    static LRESULT CanvasProc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        if (m == WM_NCCREATE) {
            auto* cs = (CREATESTRUCTW*)l;
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return DefWindowProcW(h, m, w, l);
        }
        auto* self = (Canvas*)GetWindowLongPtrW(h, GWLP_USERDATA);
        return self ? self->Proc(h, m, w, l) : DefWindowProcW(h, m, w, l);
    }

    static LRESULT PropsProc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        if (m == WM_NCCREATE) {
            auto* cs = (CREATESTRUCTW*)l;
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return DefWindowProcW(h, m, w, l);
        }
        auto* self = (Props*)GetWindowLongPtrW(h, GWLP_USERDATA);
        return self ? self->Proc(h, m, w, l) : DefWindowProcW(h, m, w, l);
    }

    static ATOM Reg(const wchar_t* cls, WNDPROC p, HINSTANCE inst)
    {
        WNDCLASSW wc;
        if (GetClassInfoW(inst, cls, &wc)) return 0;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = p;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
        wc.lpszClassName = cls;
        wc.hbrBackground = nullptr;
        return RegisterClassW(&wc);
    }

    ATOM RegisterClass1(HINSTANCE inst)
    {
        Reg(L"CGC_Host", HostProc, inst);
        Reg(L"CGC_Canvas", CanvasProc, inst);
        Reg(L"CGC_Props", PropsProc, inst);
        return 0;
    }

    HWND Create(HWND parent, const std::wstring& project)
    {
        HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
        Host* host = new Host();
        host->project = project;
        host->hwnd = CreateWindowExW(0, L"CGC_Host", nullptr, WS_CHILD | WS_VISIBLE,
                                     0, 0, 100, 100, parent, nullptr, inst, host);
        host->canvas = new Canvas(nullptr, host);
        host->props = new Props(nullptr, host);
        host->hCanvas = CreateWindowExW(0, L"CGC_Canvas", nullptr,
                                        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                        0, 0, 100, 100, host->hwnd, nullptr, inst, host->canvas);
        host->hProps = CreateWindowExW(0, L"CGC_Props", nullptr,
                                       WS_CHILD | WS_VISIBLE,
                                       0, 0, 100, 100, host->hwnd, nullptr, inst, host->props);
        host->canvas->hwnd = host->hCanvas;
        host->props->hwnd = host->hProps;
        Hosts()[host->hwnd] = host;
        host->LoadFromStore();
        host->LayoutChildren();
        return host->hwnd;
    }

    // =========================================================================
    // Public API
    // =========================================================================
    static Host* GetHost(HWND host) { return Hosts().count(host) ? Hosts()[host] : nullptr; }

    std::wstring ProjectName(HWND hostH)
    {
        Host* h = GetHost(hostH);
        return h ? h->project : std::wstring();
    }

    void AddControl(HWND hostH, const std::wstring& type)
    {
        Host* h = GetHost(hostH);
        if (!h) return;
        Canvas& cv = *h->canvas;
        ControlInstance c;
        c.type = type;
        c.name = MakeUniqueName(type, cv.ctrls);
        c.text = type == L"Button" ? L"Button1"
               : type == L"Label" ? L"Label1"
               : type == L"TextBox" ? L"" : L"";
        // smart defaults per type
        if (type == L"Button") { c.w = 124; c.h = 36; }
        else if (type == L"Label") { c.w = 160; c.h = 24; }
        else if (type == L"TextBox") { c.w = 240; c.h = 30; }
        else if (type == L"CheckBox" || type == L"RadioButton") { c.w = 150; c.h = 26; }
        else if (type == L"GroupBox") { c.w = 340; c.h = 160; }
        else if (type == L"PictureBox") { c.w = 200; c.h = 140; }
        else if (type == L"ListBox") { c.w = 220; c.h = 120; }
        else if (type == L"ComboBox") { c.w = 200; c.h = 30; }
        else if (type == L"ProgressBar") { c.w = 220; c.h = 18; }
        else if (type == L"TrackBar") { c.w = 220; c.h = 30; }
        else if (type == L"MenuStrip") { c.w = cv.form.w - 16; c.h = 26; }
        else { c.w = 140; c.h = 80; }
        // cascade placement
        int n = (int)cv.ctrls.size();
        c.x = 24 + (n % 6) * 24;
        c.y = 44 + (n % 6) * 20;
        if (c.type == L"MenuStrip") { c.x = 0; c.y = 0; }
        c.idc = NextControlId(cv.ctrls);
        if (type == L"CheckBox" || type == L"RadioButton") c.props[L"Checked"] = L"0";
        cv.ctrls.push_back(c);
        cv.selected = (int)cv.ctrls.size() - 1;
        cv.formRc = cv.FormRect();
        InvalidateRect(h->hCanvas, nullptr, FALSE);
        h->OnSelectionChanged();
        h->OnCanvasEdited();
    }

    void RefreshProps(HWND hostH)
    {
        Host* h = GetHost(hostH);
        if (h) SendMessageW(h->hProps, WM_USER_REFRESH, 0, 0);
    }

    void SetPropsVisible(HWND hostH, bool v)
    {
        Host* h = GetHost(hostH);
        if (h) { h->propsVisible = v; h->LayoutChildren(); }
    }

    bool SaveNow(HWND hostH)
    {
        Host* h = GetHost(hostH);
        if (!h) return false;
        Store::SaveLayout(h->project, h->canvas->form, h->canvas->ctrls);
        return true;
    }

    void FocusCanvas(HWND hostH)
    {
        Host* h = GetHost(hostH);
        if (h) SetFocus(h->hCanvas);
    }

    bool Export(HWND hostH, std::wstring* err)
    {
        Host* h = GetHost(hostH);
        if (!h) { if (err) *err = L"host yok"; return false; }
        // destination: project dir (or Desktop\CppGUIExport\<project>)
        std::wstring dir;
        ProjectRecord* p = g_mw ? Store::FindProject(g_mw->recents, h->project) : nullptr;
        if (p && !p->sourceDir.empty()) dir = p->sourceDir;
        else {
            wchar_t up[MAX_PATH + 2];
            DWORD n = GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH + 2);
            std::wstring home = (n > 0 && n < MAX_PATH + 2) ? up : L"C:\\Users\\Public";
            dir = home + L"\\Desktop\\CppGUIExport\\" + h->project;
        }
        if (!CreateDirDeepW(dir)) { if (err) *err = L"klasör oluşturulamadı: " + dir; return false; }

        std::wstring mainCpp = dir + L"\\" + NormalizeIdentW(h->project) + L"_w32.cpp";
        std::wstring content = CodeGen::ExportMainCpp(h->canvas->form, h->canvas->ctrls);
        if (!WriteBinFile(mainCpp, "\xEF\xBB\xBF" + WToUtf8(content), false)) {
            if (err) *err = L"yazılamadı: " + mainCpp;
            return false;
        }
        // runtime header
        std::wstring hdrDst = dir + L"\\W32GuiRuntime.h";
        std::string hdr;
        std::wstring hdrSrc = ExeDirW() + L"\\..\\runtime\\W32GuiRuntime.h";
        if (!ReadBinFile(hdrSrc, hdr)) hdrSrc = ExeDirW() + L"\\runtime\\W32GuiRuntime.h";
        if (!ReadBinFile(hdrSrc, hdr)) hdrSrc = L"runtime\\W32GuiRuntime.h";
        if (!ReadBinFile(hdrSrc, hdr)) { if (err) *err = L"W32GuiRuntime.h bulunamadı"; return false; }
        if (!WriteBinFile(hdrDst, hdr, false)) { if (err) *err = L"yazılamadı: " + hdrDst; return false; }
        // build script
        std::wstring bat = dir + L"\\build_mingw.bat";
        std::wstring batTxt =
            L"@echo off\r\n"
            L"rem Build " + h->project + L" (pure Win32)\r\n"
            L"where x86_64-w64-mingw32-g++ >nul 2>nul\r\n"
            L"if %errorlevel%==0 (\r\n"
            L"  x86_64-w64-mingw32-g++ -std=c++17 -municode -mwindows \"" +
            NameFromPathW(mainCpp) + L"\" -o \"" + NormalizeIdentW(h->project) + L".exe\"\r\n"
            L") else (\r\n"
            L"  echo MinGW bulunamadi. VS2019 ile: bos C++ projesi acip dosyalari ekleyin.\r\n"
            L")\r\n";
        WriteBinFile(bat, WToUtf8(batTxt), false);
        if (err) *err = dir;
        return true;
    }
}
