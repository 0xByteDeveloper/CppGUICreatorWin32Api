// ============================================================================
//  gen_export_test.cpp — dev-time pipeline test tool (portable, runs on Linux)
//  Reads a .layout file and emits the standalone Win32 app source that
//  CodeGen::ExportMainCpp would generate. Used to verify the generated code
//  actually compiles with a mingw-w64 toolchain (see tests/export_test.sh).
// ============================================================================
#include "../src/stdafx.h"
#include "../src/Model.h"
#include "../src/CodeGen.h"
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: gen_export_test <layout-file>\n");
        return 2;
    }
    std::string bytes;
#ifdef _WIN32
    std::wstring path;
    int n = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, nullptr, 0);
    path.resize(n);
    MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, &path[0], n);
    path.pop_back();
#else
    std::string p = argv[1];
    std::wstring path = Utf8ToW(p);
#endif
    if (!ReadBinFile(path, bytes)) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    FormInfo f;
    std::vector<ControlInstance> cs;
    if (!ParseLayout(Utf8ToW(bytes), f, cs)) {
        fprintf(stderr, "parse failed\n");
        return 2;
    }
    std::wstring out = CodeGen::ExportMainCpp(f, cs);
    std::string u8 = WToUtf8(out);
    fwrite(u8.data(), 1, u8.size(), stdout);
    return 0;
}
