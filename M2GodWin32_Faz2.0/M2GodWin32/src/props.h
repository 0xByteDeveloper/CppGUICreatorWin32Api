// props.h — Özellikler & Olaylar paneli
#pragma once
#include "app.h"
namespace props {
HWND Create(HWND parent);
void Refresh(HWND panel);          // seçime göre yeniden doldur
void ApplyPending(HWND panel);     // açık editör varsa uygula
}
