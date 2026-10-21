// ============================================================================
//  tests/test_core.cpp — native (Linux/Windows) unit tests for the portable
//  core: Model serialization + CodeGen transforms. Run: ./test_core
//  Build: see tests/run.sh
// ============================================================================
#include "../src/stdafx.h"
#include "../src/Model.h"
#include "../src/CodeGen.h"
#include <cassert>
#include <cstdio>

static int g_failed = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s (line %d)\n", msg, __LINE__); g_failed++; } \
    else printf("ok  : %s\n", msg); } while (0)

int main()
{
    // ------------------------------------------------------------------
    printf("== MakeUniqueName / NextControlId ==\n");
    {
        std::vector<ControlInstance> v;
        CHECK(MakeUniqueName(L"Button", v) == L"button1", "first name is button1");
        ControlInstance c1; c1.name = L"button1"; v.push_back(c1);
        CHECK(MakeUniqueName(L"Button", v) == L"button2", "second is button2");
        ControlInstance c2; c2.name = L"pictureBox1"; v.push_back(c2);
        CHECK(MakeUniqueName(L"PictureBox", v) == L"pictureBox2", "pictureBox2");
        CHECK(NextControlId(v) == 1001, "first id 1001");
        v.back().idc = 1017;
        CHECK(NextControlId(v) == 1018, "next id after max");
        // Turkish normalization
        std::vector<ControlInstance> v2;
        CHECK(MakeUniqueName(L"GroupBox", v2) == L"groupBox1", "ascii name");
    }

    // ------------------------------------------------------------------
    printf("== Layout serialization roundtrip ==\n");
    {
        FormInfo f;
        f.name = L"MyForm"; f.text = L"Başlık Türkçe ĞÜŞİ"; f.w = 800; f.h = 600;
        f.backColor = L"FFFFFF";
        std::vector<ControlInstance> cs;
        ControlInstance b; b.type = L"Button"; b.name = L"button1"; b.text = L"KAYDET";
        b.x = 24; b.y = 40; b.w = 124; b.h = 36; b.idc = 1001;
        b.events[L"Click"] = L"button1_Click";
        cs.push_back(b);
        ControlInstance pb; pb.type = L"PictureBox"; pb.name = L"pictureBox1";
        pb.x = 200; pb.y = 40; pb.w = 200; pb.h = 120; pb.idc = 1002;
        pb.events[L"Paint"] = L"";
        pb.props[L"BackColor"] = L"F0F0F2";
        cs.push_back(pb);

        std::wstring text = SerializeLayout(f, cs);
        CHECK(text.find(L"FORM|MyForm|") == 0, "layout starts with FORM row");
        CHECK(text.find(L"EVT|button1|Click|button1_Click") != std::wstring::npos,
              "event row serialized");

        FormInfo f2; std::vector<ControlInstance> cs2;
        CHECK(ParseLayout(text, f2, cs2), "parse ok");
        CHECK(f2.name == L"MyForm" && f2.w == 800 && f2.h == 600, "form fields");
        CHECK(cs2.size() == 2, "two controls");
        CHECK(cs2[0].events[L"Click"] == L"button1_Click", "event roundtrip");
        CHECK(cs2[1].props[L"BackColor"] == L"F0F0F2", "prop roundtrip");
        CHECK(cs2[0].idc == 1001, "id roundtrip");
        CHECK(cs2[0].text == L"KAYDET", "text roundtrip (utf8-safe)");
        CHECK(cs2[1].type == L"PictureBox", "type roundtrip");
    }

    // ------------------------------------------------------------------
    printf("== Projects serialization roundtrip ==\n");
    {
        std::vector<ProjectRecord> v;
        ProjectRecord p; p.name = L"MyForm"; p.created = 1000; p.lastOpened = 2000;
        p.sourceDir = L"C:\\src\\repos\\MyForm"; p.cppPath = L"C:\\src\\repos\\MyForm\\MyForm.cpp";
        p.slnPath = L"C:\\src\\repos\\MyForm\\MyForm.sln"; p.fromVS = true; p.pid = 4242;
        v.push_back(p);
        ProjectRecord q; q.name = L"Adsız1"; q.created = 3000; q.lastOpened = 1500;
        v.push_back(q);

        std::wstring text = SerializeProjects(v);
        CHECK(StartsWithW(text, L"PROJECTS|v1"), "projects header");
        std::vector<ProjectRecord> v2;
        ParseProjects(text, v2);
        CHECK(v2.size() == 2, "two records");
        CHECK(v2[0].name == L"MyForm" && v2[0].pid == 4242 && v2[0].fromVS, "record fields");
        CHECK(v2[0].cppPath == L"C:\\src\\repos\\MyForm\\MyForm.cpp", "path with backslashes");
        CHECK(v2[1].name == L"Adsız1", "unicode name");
    }

    // ------------------------------------------------------------------
    printf("== CodeGen::HandlerName ==\n");
    {
        CHECK(CodeGen::HandlerName(L"button1", L"Click") == L"button1_Click", "button click");
        CHECK(CodeGen::HandlerName(L"MyForm", L"Load") == L"MyForm_Load", "form load");
        CHECK(CodeGen::HandlerName(L"pictureBox1", L"Paint") == L"pictureBox1_Paint", "paint");
        // Turkish normalization inside handler names
        CHECK(CodeGen::HandlerName(L"düğme", L"Click") == L"dugme_Click", "turkish->ascii");
    }

    // ------------------------------------------------------------------
    printf("== CodeGen::EnsureHandlerInContent (new handler) ==\n");
    {
        std::wstring content = L"#include <windows.h>\r\n\r\nint main(){return 0;}\r\n";
        auto r = CodeGen::EnsureHandlerInContent(content, L"pictureBox1", L"Paint", false);
        CHECK(!r.existed, "not existed");
        CHECK(r.fileChanged, "changed");
        CHECK(r.line > 0, "line computed");
        CHECK(content.size() > 0, "content untouched (pure fn)");
        // simulate what EnsureHandler does: rebuild output and verify
        std::wstring block = CodeGen::BuildHandlerBlock(L"pictureBox1", L"Paint", false);
        CHECK(block.find(L"void pictureBox1_Paint(W32Object* sender, W32EventArgs* e)")
              != std::wstring::npos, "signature uses Win32 sender/e");
        CHECK(IsAsciiW(block), "generated block is ASCII-only");
        CHECK(block.find(L"System::Object") == std::wstring::npos,
              "no .NET types in generated code");
    }

    // ------------------------------------------------------------------
    printf("== CodeGen::EnsureHandlerInContent (existing handler) ==\n");
    {
        std::wstring content =
            L"#include \"W32GuiRuntime.h\"\n"
            L"void button1_Click(W32Object* sender, W32EventArgs* e)\r\n{\r\n}\r\n";
        auto r = CodeGen::EnsureHandlerInContent(content, L"button1", L"Click", false);
        CHECK(r.existed, "existed");
        CHECK(r.line == 2, "found at line 2");
        CHECK(!r.fileChanged, "no change needed");
        // handler present but runtime include missing -> file must change (include only)
        std::wstring content2 =
            L"void button1_Click(W32Object* sender, W32EventArgs* e)\r\n{\r\n}\r\n";
        auto r2 = CodeGen::EnsureHandlerInContent(content2, L"button1", L"Click", false);
        CHECK(r2.existed, "existed (no include)");
        CHECK(r2.fileChanged && r2.incAdded, "only include added");
        CHECK(r2.line == 2, "line accounts for inserted include");
    }

    // ------------------------------------------------------------------
    printf("== EnsureRuntimeInclude ==\n");
    {
        std::wstring content = L"#include <windows.h>\n#include \"x.h\"\n\nvoid f(){}\n";
        std::wstring out = CodeGen::EnsureRuntimeInclude(content);
        CHECK(out.find(L"W32GuiRuntime.h") != std::wstring::npos, "include inserted");
        size_t p1 = out.find(L"#include \"x.h\"");
        size_t p2 = out.find(L"#include \"W32GuiRuntime.h\"");
        CHECK(p2 > p1, "inserted after last include");
        // already present -> unchanged
        std::wstring out2 = CodeGen::EnsureRuntimeInclude(out);
        CHECK(out2 == out, "idempotent");
        // no includes at all
        std::wstring bare = L"// comment\n\nvoid f(){}\n";
        std::wstring out3 = CodeGen::EnsureRuntimeInclude(bare);
        CHECK(out3.find(L"W32GuiRuntime.h") != std::wstring::npos, "inserted at top");
    }

    // ------------------------------------------------------------------
    printf("== ExportMainCpp self-heals control ids ==\n");
    {
        FormInfo f; f.name = L"MyForm"; f.text = L"Test";
        std::vector<ControlInstance> cs;
        ControlInstance a; a.type = L"Button";  a.name = L"button1"; cs.push_back(a);
        ControlInstance b; b.type = L"PictureBox"; b.name = L"pictureBox1"; cs.push_back(b);
        std::wstring code = CodeGen::ExportMainCpp(f, cs);
        CHECK(code.find(L"#define IDC_BUTTON1 1001") != std::wstring::npos, "button id 1001");
        CHECK(code.find(L"#define IDC_PICTUREBOX1 1002") != std::wstring::npos, "picturebox id 1002");
        CHECK(code.find(L"W32Object* sender, W32EventArgs* e") != std::wstring::npos,
              "export uses Win32 sender/e");
        CHECK(IsAsciiW(code), "export source is ASCII-only");
    }

    // ------------------------------------------------------------------
    printf("== Form load handler naming (MyForm_Load spec) ==\n");
    {
        auto block = CodeGen::BuildHandlerBlock(L"MyForm", L"Load", true);
        CHECK(block.find(L"void MyForm_Load(W32Object* sender, W32EventArgs* e)")
              != std::wstring::npos, "MyForm_Load Win32 signature");
    }

    // ------------------------------------------------------------------
    printf("== Store roundtrip via string API (Serialize/ParseProjects) ==\n");
    {
        std::vector<ProjectRecord> v;
        for (int i = 0; i < 5; i++) {
            ProjectRecord p;
            p.name = Fmt(L"P%d", i);
            p.lastOpened = 1000 + i;
            v.push_back(p);
        }
        SortProjectsNewestFirst(v);
        CHECK(v[0].name == L"P4" && v[4].name == L"P0", "sorted newest first");
    }

    printf("\n%s (%d failures)\n", g_failed ? "TESTS FAILED" : "ALL TESTS PASSED", g_failed);
    return g_failed ? 1 : 0;
}
