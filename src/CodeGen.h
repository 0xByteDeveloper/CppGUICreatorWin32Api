// ============================================================================
//  CodeGen.h — event handler code generation (Win32 sender/e equivalents)
//
//  CLR prototype :  private: System::Void MyForm_Load(System::Object^ sender,
//                                            System::EventArgs^ e)
//  Win32 output  :  void MyForm_Load(W32Object* sender, W32EventArgs* e)
//
//  Generated code is ASCII-only and marker-safe: user code outside our
//  regions is never touched.
// ============================================================================
#pragma once
#include "stdafx.h"
#include "Model.h"

namespace CodeGen
{
    // "pictureBox1" + "Paint" -> "pictureBox1_Paint"; form: "MyForm" + "Load" -> "MyForm_Load"
    std::wstring HandlerName(const std::wstring& owner, const std::wstring& event);

    // the handler stub text block (ASCII only), ends with two newlines
    std::wstring BuildHandlerBlock(const std::wstring& owner, const std::wstring& event,
                                   bool isForm);

    struct EnsureResult
    {
        bool existed = false;      // handler already present
        bool fileChanged = false;  // we wrote the file
        bool incAdded = false;     // runtime include line was inserted
        bool runtimeCopied = false;// W32GuiRuntime.h copied next to cpp
        int  line = 0;             // 1-based line of the handler definition
        std::wstring handler;
        std::wstring error;
    };

    // ---- pure, unit-testable content transforms ----
    // find handler; if missing, append stub. Returns line of definition (1-based).
    EnsureResult EnsureHandlerInContent(const std::wstring& content,
                                        const std::wstring& owner,
                                        const std::wstring& event,
                                        bool isForm);

    // ensure the runtime include exists in the content (after last #include)
    std::wstring EnsureRuntimeInclude(const std::wstring& content);

    // ---- file-level wrapper ----
    EnsureResult EnsureHandler(const std::wstring& cppPath,
                               const std::wstring& owner,
                               const std::wstring& event,
                               bool isForm,
                               const std::wstring& runtimeHeaderSourceDir);

    // ---- standalone app export ----
    // generates a complete main_w32.cpp for W32GuiRuntime.h
    std::wstring ExportMainCpp(const FormInfo& f, const std::vector<ControlInstance>& ctrls);
}
