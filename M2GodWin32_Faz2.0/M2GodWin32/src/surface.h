// surface.h — tasarım yüzeyi (grid, seçim, sürükle/boyutlandır, bileşen tepsisi)
#pragma once
#include "model.h"

namespace surface {

// Yüzey penceresini oluştur (ana pencere child'ı)
HWND Create(HWND parent);

// Modeldeki tüm kontrolleri yeniden oluştur (yükleme/klasör değişimi sonrası)
void RecreateAll(HWND surf);

// Geometri/scroll'a göre child pencereleri konumlandır
void LayoutAll(HWND surf);
// Tek kontrolü doğru konuma taşı (tek ekleme sonrası)
void PositionOne(HWND surf, Ctrl* c);

// Seçim değişikliğini ana pencereye bildir
void NotifySelection();

// Form alanının (client) yüzey içindeki dikdörtgeni (scroll dahil)
RECT FormRect(HWND surf);

// Fare ekran koordinatı → form koordinatına çevir
void ScreenToForm(HWND surf, int sx, int sy, int* fx, int* fy);

// Tasarım modu subclass'ı (fareyi yüzeye aktarır)
void SubclassForDesign(HWND h);

// Seçimi yenile (özellik paneli değişiklikleri sonrası)
void RefreshSelection(HWND surf);

} // namespace surface
