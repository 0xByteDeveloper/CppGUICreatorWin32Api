// vsint.h — Visual Studio entegrasyonu (DTE COM otomasyonu + pencere fallback)
#pragma once
#include "model.h"
struct IDispatch;

namespace vsint {

struct VSInstance {
    DWORD pid = 0;
    std::wstring version;       // "16.0" ...
    std::wstring progId;        // "VisualStudio.DTE.16.0"
    std::wstring solutionName;  // "MySolution"
    std::wstring solutionPath;  // tam yol (DTE varsa)
    std::wstring projectPath;   // seçilen proje yolu (connect sonrası)
    std::wstring projectName;
    HWND hwnd = nullptr;
    bool hasDte = false;
    IDispatch* dte = nullptr;   // DTE late-binding (AddRef sahibi: bu struct)
};

// Çalışan VS örneklerini listele (ROT + EnumWindows)
std::vector<VSInstance> Enumerate();

// Çözüm dosyasındaki .vcxproj yolları
std::vector<std::wstring> EnumerateProjectsOfSolution(const std::wstring& slnPath);

// Bağlan: seçilen örnek + proje yolu kaydet
bool Connect(VSInstance& inst, const std::wstring& projectPath);

// Bağlantıyı kes
void Disconnect();

bool IsConnected();
VSInstance& Current();

// Form1.h/.cpp'yi bağlı projeye senkronize et (marker koruyarak)
bool SyncToProject(Ctrl* targetCtrl, const std::wstring& eventName, int* handlerLine);

// Handler'ı garanti et, VS'yi öne getir ve satıra git
bool EnsureHandlerAndGoto(Ctrl* targetCtrl, const std::wstring& eventName);

// VS ana penceresini öne getir (topmost gibi)
void BringToFront(const VSInstance& inst);

} // namespace vsint
