// ============================================================================
//  DesignerInternal.h — shared declarations between Designer.cpp / Designer2.cpp
//  (internal to DesignerNS; not part of the public UI.h contract)
// ============================================================================
#pragma once
#include "UI.h"
#include "Theme.h"
#include "CodeGen.h"

using namespace Gdiplus;

namespace DesignerNS
{
    #define WM_USER_REFRESH  (WM_USER + 7)

    struct Host;

    // -------------------------------------------------------------------------
    // Canvas
    // -------------------------------------------------------------------------
    enum Handle { H_NONE, H_MOVE, H_NW, H_N, H_NE, H_E, H_SE, H_S, H_SW, H_W };

    struct Canvas
    {
        HWND hwnd = nullptr;
        Host* host = nullptr;
        FormInfo form;
        std::vector<ControlInstance> ctrls;
        int selected = -1;             // -1 = form
        Handle drag = H_NONE;
        POINT dragStart{};
        ControlInstance dragOrig;
        HoverTrack hover;
        RECT formRc{};

        Canvas(HWND h, Host* hp) : hwnd(h), host(hp) {}

        float S() const { return ScaleF(hwnd); }
        RECT  FormRect() const;
        RECT  CtrlRect(const ControlInstance& c);
        int   HitCtrl(POINT pt);
        void  Select(int idx);

        LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l);
        void    Paint(Graphics& g, const RECT& rc, float scale);
    };

    // -------------------------------------------------------------------------
    // Props — right panel: object header, properties, events
    // -------------------------------------------------------------------------
    struct Props
    {
        HWND hwnd = nullptr;
        Host* host = nullptr;
        int scroll = 0;
        struct Row { RECT rc; bool isEvent; std::wstring key, val; int evIndex; bool header; };
        std::vector<Row> rows;
        int hotRow = -1;
        HoverTrack hover;
        HWND hEdit = nullptr;          // inline edit child
        int  editRow = -1;             // row being edited
        bool editIsEvent = false;
        int  editEvIndex = -1;

        Props(HWND h, Host* hp) : hwnd(h), host(hp) {}

        LRESULT Proc(HWND h, UINT m, WPARAM w, LPARAM l);
        void    Paint(Graphics& g, const RECT& rc, float scale);
        void    RebuildRows();
        void    BeginEdit(int row);
        void    CommitEdit();
        void    CancelEdit();
    };

    // -------------------------------------------------------------------------
    // Host — contains canvas + props, mediates
    // -------------------------------------------------------------------------
    struct Host
    {
        HWND hwnd = nullptr;
        HWND hCanvas = nullptr, hProps = nullptr;
        Canvas* canvas = nullptr;
        Props*  props = nullptr;
        std::wstring project;
        bool propsVisible = true;

        void OnSelectionChanged();     // canvas selection changed -> rebuild props
        void OnCanvasEdited();         // layout changed -> autosave
        void ApplyAndSave();           // props edits -> canvas + autosave
        void LoadFromStore();          // store -> bundled -> blank
        void LayoutChildren();
    };

    std::map<HWND, Host*>& Hosts();    // HWND -> Host registry

    // -------------------------------------------------------------------------
    // painting helpers shared between files
    // -------------------------------------------------------------------------
    void FillRoundTop(Graphics& g, float x, float y, float w, float h, float r, Color c);
    void PaintControlGfx(Graphics& g, const ControlInstance& c, const RECT& r,
                         float scale, bool isSelected);
}
