// factory.h — .NET tipi → Win32 sınıfı eşleme ve HWND oluşturma
#pragma once
#include "model.h"

namespace factory {

// Kontrolü yüzeyde oluşturur (hwnd döner; surface prosedürünü kendisi subclass'lar)
HWND CreateOnSurface(HWND surface, Ctrl* c);

// Özellik değiştiğinde HWND'ye uygula (Text/Enabled/Visible/Font/renk/geometri)
void ApplyProp(Ctrl* c, const std::wstring& prop, const std::wstring& value);

// Tüm görsel özellikleri yeniden uygula
void ApplyAll(Ctrl* c);

// M2GOD_PANEL / M2GOD_PICTURE / M2GOD_PLACEHOLDER pencere sınıflarını kaydet
void RegisterCustomClasses(HINSTANCE hInst);

// tip → (winclass, ekstra style) — katalogdaki win32_mapping + bu tablo birleşir
struct Win32Class { std::wstring cls; DWORD style; DWORD exstyle; std::wstring styleText; int dw = 0, dh = 0; };
Win32Class MapClass(const std::wstring& type);

// --- FAZ 2.6: TabControl / TabPage sayfa yönetimi ---------------------------
// Sayfa HWND'lerinin dikdörtgenini TCM_ADJUSTRECT ile hesaplar; yalnız seçili
// sayfayı gösterir; sayfa Ctrl geometrilerini senkronlar.
void UpdateTabPages(HWND surf, Ctrl* tab);
// Seçili sayfayı öne çıkar (sekme şeridi tıklaması / panel seçimi)
void SelectTabPage(HWND surf, Ctrl* tab, Ctrl* page);
// Sekme şeridi hit-test: yüzey koordinatında nokta bir sekme başlığındaysa index, değilse -1
int  TabStripHitIndex(HWND surf, Ctrl* tab, int sx, int sy);
// Sekme başlığının (şerit) yüzey koordinatlarındaki rect'i (akıllı ok konumu için)
bool TabStripRect(HWND surf, Ctrl* tab, RECT& out);

} // namespace factory
