// ============================================================================
//  stdafx.h — common precompiled header for CppGUICreatorWin32Api
//  Pure Win32 API + C++17. No .NET / CLR anywhere.
// ============================================================================
#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _WIN32
#include <windows.h>
#include <objidl.h>      // must precede gdiplus.h
#include <gdiplus.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <oleauto.h>
#else
// ---- non-Windows shims: keep the portable core (Model/CodeGen/Util string
// ---- + file helpers) compilable for native unit tests on Linux/macOS.
#include <cstddef>
typedef unsigned long DWORD;
typedef unsigned int  UINT;
typedef int           BOOL;
typedef void*         HWND;
typedef void*         HANDLE;
typedef void*         HINSTANCE;
typedef void*         HDC;
typedef void*         HFONT;
typedef void*         HBITMAP;
typedef void*         HBRUSH;
typedef void*         HICON;
typedef void*         HCURSOR;
typedef long          LRESULT;
typedef unsigned int  UINT_PTR;
typedef long long     LONG_PTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t*       LPWSTR;
typedef const char*    LPCSTR;
#define MAX_PATH 260
#define WINAPI
#define APIENTRY
#define CALLBACK
struct RECT { long left, top, right, bottom; };
struct POINT { long x, y; };
typedef unsigned long long WPARAM;
typedef long long          LPARAM;
typedef void*              LPVOID;
#ifndef LOWORD
#define LOWORD(l)  ((unsigned short)(((uintptr_t)(l)) & 0xFFFF))
#endif
#ifndef HIWORD
#define HIWORD(l)  ((unsigned short)((((uintptr_t)(l)) >> 16) & 0xFFFF))
#endif
#endif

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <memory>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cwchar>
#include <cctype>
#include <ctime>
#include <cmath>
#include <cstdint>

// ---------------------------------------------------------------------------
// App-wide constants
// ---------------------------------------------------------------------------
#define APP_NAME        L"CppGUI Creator"
#define APP_COMPANY     L"0xByteDeveloper"
#define APP_VERSION_W   L"1.0.0"
#define WELCOME_TAB     L"Welcome Page"

// app message base for widget -> MainWindow notifications
#define AM_BASE         (WM_APP + 200)

// ---------------------------------------------------------------------------
// Small wide-string helpers (declared here, implemented in Util.cpp)
// ---------------------------------------------------------------------------
std::wstring Utf8ToW(const std::string& s);
std::string  WToUtf8(const std::wstring& w);

std::wstring TrimW(const std::wstring& s);
std::wstring LowerW(const std::wstring& s);
std::wstring UpperW(const std::wstring& s);
bool StartsWithW(const std::wstring& s, const std::wstring& p);
bool EndsWithW(const std::wstring& s, const std::wstring& p);
bool ContainsI(const std::wstring& s, const std::wstring& sub);
std::vector<std::wstring> SplitW(const std::wstring& s, wchar_t sep);
std::wstring JoinW(const std::vector<std::wstring>& v, const std::wstring& sep);
std::wstring ReplaceW(std::wstring s, const std::wstring& from, const std::wstring& to);
std::wstring Fmt(const wchar_t* fmt, ...);
std::wstring NormalizeIdentW(const std::wstring& s);   // Turkish -> ASCII identifier

// ASCII-only check (generated code must stay ASCII-safe)
bool IsAsciiW(const std::wstring& s);

// ---------------------------------------------------------------------------
// File helpers
// ---------------------------------------------------------------------------
bool  ReadBinFile(const std::wstring& path, std::string& out);
bool  WriteBinFile(const std::wstring& path, const std::string& data, bool append);
bool  FileExistsW(const std::wstring& path);
bool  DirExistsW(const std::wstring& path);
bool  CreateDirDeepW(const std::wstring& path);
std::wstring ExeDirW();
std::wstring NameFromPathW(const std::wstring& p);
std::wstring DirFromPathW(const std::wstring& p);
long long FileMtimeUnix(const std::wstring& path);

// ---------------------------------------------------------------------------
// Time helpers
// ---------------------------------------------------------------------------
long long    NowUnix();
std::wstring TimeStr(long long unixSec);      // yyyy-mm-dd hh:mm
std::wstring RelTime(long long unixSec);      // "3 dk önce" etc.

// ---------------------------------------------------------------------------
// Mouse coordinate extractors
// ---------------------------------------------------------------------------
inline int GetXL(LPARAM l) { return (int)(short)LOWORD(l); }
inline int GetYL(LPARAM l) { return (int)(short)HIWORD(l); }

// ---------------------------------------------------------------------------
// DPI helper
// ---------------------------------------------------------------------------
UINT DpiForWnd(HWND h);
inline int  ScaleI(HWND h, int dip) { return (int)lround(dip * DpiForWnd(h) / 96.0); }
inline float ScaleF(HWND h)         { return DpiForWnd(h) / 96.0f; }
