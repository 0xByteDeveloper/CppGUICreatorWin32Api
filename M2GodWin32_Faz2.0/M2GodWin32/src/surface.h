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

// Araç kutusundan yeni kontrol yerleştir (bırakma VE tıkla-yerleştir ortak yolu).
// sx,sy: yüzey client koordinatı; hedef konteyner varsa otomatik içine alınır.
void PlaceNew(HWND surf, const std::wstring& type, int sx, int sy);

// Modelden bir kontrol silinirken çağrılır: aktif sürükleme bu kontrolün
// (veya atasının) üzerindeyse sürükleme durumunu güvenle kapatır.
void OnModelDelete(Ctrl* c);

} // namespace surface
