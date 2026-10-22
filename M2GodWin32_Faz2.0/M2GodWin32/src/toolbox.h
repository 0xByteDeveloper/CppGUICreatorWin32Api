// toolbox.h — araç kutusu (arama + kategori ağacı + sürükle-bırak + tıkla-yerleştir)
#pragma once
#include "app.h"
namespace toolbox {
HWND Create(HWND parent);
void Refresh(HWND tb);                     // katalogdan (filtreyle) yeniden doldur

// tıkla-yerleştir modu (VS2019 davranışı):
// nesneye tıkla → formda imleç artı olur → tıklanan yere nesne eklenir → imleç normale döner
std::wstring ArmedType();                  // boşsa mod kapalı
void Arm(const std::wstring& type);
void Disarm();                             // ESC veya yerleştirme sonrası
}
