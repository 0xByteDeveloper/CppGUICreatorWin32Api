// toolbox.h — araç kutusu (kategori ağacı + sürükle-bırak)
#pragma once
#include "app.h"
namespace toolbox {
HWND Create(HWND parent);
void Refresh(HWND tb);   // katalogdan yeniden doldur
}
