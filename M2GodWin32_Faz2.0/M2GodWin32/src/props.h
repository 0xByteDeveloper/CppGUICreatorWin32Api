// props.h — Özellikler/Olaylar paneli
#pragma once
#include "app.h"
namespace props {
HWND Create(HWND parent);
void Refresh(HWND panel);          // tam yenileme (seçim/model değişimi)
void UpdateLiveGeometry();         // sürükleme sırasında hafif güncelleme (flicker yok)
void ApplyPending(HWND panel);     // açık satır içi düzenleyiciyi işle
bool IsEditing();                  // satır içi düzenleyici açık mı (ESC hızlandırıcısını yutmak için)
}
