// ============================================================================
//  M2GodWin32 Faz 2.0 — God Mode MVP
//  Visual Studio 2019 C++ CLR Form Designer benzeri, tam kapsamlı Win32 API
//  görsel UI tasarım ortamı. Config_info.json (.NET Framework kataloğu) ile
//  çalışır. Detay için README.md
// ============================================================================
#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>

using std::min;
using std::max;

// ---------------------------------------------------------------------------
// Uygulama çapında genel durum
// ---------------------------------------------------------------------------
extern HINSTANCE g_hInst;
extern HWND      g_hMain;          // ana pencere
extern HFONT     g_fontUI;         // Segoe UI 9pt
extern HFONT     g_fontUIBold;
extern HFONT     g_fontMono;       // Consolas 9pt
extern std::wstring g_exeDir;      // exe klasörü (trailing \)
extern bool      g_modelDirty;

// Ana pencereye gönderilen özel mesajlar
#define MWM_SEL_CHANGED     (WM_APP+1)   // wParam: Ctrl* (0 = form seçili), lParam: 0
#define MWM_MODEL_CHANGED   (WM_APP+2)
#define MWM_EVENT_DBLCLICK  (WM_APP+3)   // wParam: Ctrl*, lParam: wchar_t* eventName (heap)
#define MWM_PROPS_APPLIED   (WM_APP+4)   // özellik paneli uyguladı → surface yenile
#define MWM_STATUS          (WM_APP+5)   // lParam: wchar_t* text (heap, static havuz)

// ---------------------------------------------------------------------------
// Dize / dosya yardımcıları
// ---------------------------------------------------------------------------
std::wstring Utf8ToWide(const std::string& s);
std::string  WideToUtf8(const std::wstring& w);
std::wstring Lower(std::wstring s);
std::wstring Trim(const std::wstring& s);
std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep);
std::wstring Join(const std::vector<std::wstring>& v, const std::wstring& sep);
std::wstring ReplaceAll(std::wstring s, const std::wstring& from, const std::wstring& to);

bool  ReadFileText(const std::wstring& path, std::string& out);      // utf-8 (BOM tolere)
bool  WriteFileText(const std::wstring& path, const std::string& data);
bool  FileExists(const std::wstring& path);
std::wstring GetFileDir(const std::wstring& path);                   // trailing \ yok
std::wstring GetFileName(const std::wstring& path);
std::wstring ExePath();

// ---------------------------------------------------------------------------
// Renk ve font
// ---------------------------------------------------------------------------
// "#RRGGBB" veya bilinen isim ("Red", "Control", "Window"...) → COLORREF
bool ParseColor(const std::wstring& s, COLORREF& out);
std::wstring ColorToHex(COLORREF c);
HFONT  MakeFont(const std::wstring& name, int pt, bool bold, bool italic, bool underline);
// "Segoe UI, 9pt, style=Bold" gibi font değerini çöz
bool  ParseFontValue(const std::wstring& v, std::wstring& name, int& pt, bool& bold, bool& italic, bool& underline);

// ---------------------------------------------------------------------------
// Status bar mesajı
// ---------------------------------------------------------------------------
void Status(const std::wstring& text);
void SetStatusBarHwnd(HWND h);

// UI ölçüleri (mainwnd ile surface paylaşır)
constexpr int TOOLBOX_W  = 225;
constexpr int PROPS_W    = 300;
constexpr int TOOLBAR_H  = 34;
constexpr int STATUS_H   = 24;
constexpr int GRID_SIZE  = 8;      // snap ızgarası
constexpr int SEL_MARGIN = 4;      // seçim tutamaçları dış boşluğu

// WM_COMMAND id aralıkları (araç panelleri)
#define IDC_TOOLBOX_TREE   1001
#define IDC_PROPS_TAB      1100
#define IDC_PROPS_LIST     1101
#define IDC_EVENTS_LIST    1102
#define IDC_SURFACE        1200
#define IDC_TOOLBAR        1300
#define IDC_STATUSBAR      1400
