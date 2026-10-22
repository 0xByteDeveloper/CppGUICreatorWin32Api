// vsint.h — Visual Studio entegrasyonu (DTE COM otomasyonu + süreç/pencere taraması)
#pragma once
#include "model.h"
struct IDispatch;

namespace vsint {

struct VSInstance {
    DWORD pid = 0;
    std::wstring version;       // "16.0" ...
    std::wstring progId;        // "VisualStudio.DTE.16.0"
    std::wstring solutionName;  // "MySolution"  (pencere başlığından)
    std::wstring solutionPath;  // tam .sln yolu (DTE veya disk taraması)
    std::wstring projectPath;   // seçilen proje yolu (connect sonrası)
    std::wstring projectName;   // "M2God.vcxproj"
    std::wstring exePath;       // devenv.exe yolu
    std::wstring title;         // ana pencere başlığı
    HWND hwnd = nullptr;
    bool hasDte = false;
    IDispatch* dte = nullptr;   // DTE late-binding (AddRef sahibi: bu struct)
    std::vector<std::wstring> projects;   // çözümdeki .vcxproj yolları (proje isimleriyle listelenir)
};

// Çalışan VS örneklerini listele.
// YÖNETİCİ UYUMLU: devenv.exe süreçleri Toolhelp32 ile taranır + ana pencere
// başlığından çözüm adı okunur (yüksek bütünlük seviyesinde de çalışır).
// DTE (ROT) varsa best-effort zenginleştirme yapılır; bulunamazsa pencere modu.
std::vector<VSInstance> Enumerate();

// Çözüm dosyasındaki .vcxproj yolları
std::vector<std::wstring> EnumerateProjectsOfSolution(const std::wstring& slnPath);

// Klasörde (derinlik sınırlı, bin/obj/.vs atlanarak) .vcxproj tarar
std::vector<std::wstring> FindVcxprojsUnder(const std::wstring& dir, int depth);

// Çözüm adından diskte .sln arar (source\repos vb. bilinen kökler)
std::wstring FindSlnOnDisk(const std::wstring& solutionName);

// Bağlan: seçilen örnek + proje yolu kaydet
bool Connect(VSInstance& inst, const std::wstring& projectPath);

// Bağlantı sonrası: Form1.h/Form1.cpp'i proje klasöründe oluştur (yoksa)
// ve .vcxproj'a ekle. (Giriş ekranındaki "VS'ye bağlan" akışı bunu çağırır)
bool PrepareProjectFiles();

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
