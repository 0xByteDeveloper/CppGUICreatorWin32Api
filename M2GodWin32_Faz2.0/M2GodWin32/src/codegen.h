// codegen.h — Form1.h / Form1.cpp kod üretimi (marker tabanlı artımlı senkron)
#pragma once
#include "model.h"

namespace codegen {

// Yeni tasarım için Form1.cpp gövdesi üret
std::wstring GenerateForm1Cpp();
// Form1.h üret
std::wstring GenerateForm1H();

// Tam derlenebilir VS2019 projesi klasörüne yaz (Form1.h, Form1.cpp)
// klasör boş olabilir; dönen değer: başarı
bool EmitProject(const std::wstring& folder);

// Mevcut dosyaya artımlı senkron (marker'ları güncelle, kullanıcı kodunu koru)
// handlerName boş değilse: eksik handler stub'ını ekle; handlerLine döner (1 tabanlı, 0=eklenemedi)
int SyncIntoFile(const std::wstring& cppPath, const std::wstring& hPath,
                 Ctrl* targetCtrl, const std::wstring& eventName, int* handlerLine);

// Sadece handler'ı garanti et ve satırını döndür (VS entegrasyonu için)
int EnsureHandler(const std::wstring& cppPath, const std::wstring& hPath,
                  const std::wstring& handlerName, const std::wstring& declLine);

// Faz1.4 "GeneratedForm.cpp" içe aktarma (CreateWindow satırlarından tasarım çıkarır)
bool ImportLegacyGenerated(const std::wstring& cppPath);

// ID sabiti adı: button1 → IDC_BUTTON1
std::wstring IdMacroOf(const std::wstring& ctrlName);

} // namespace codegen
