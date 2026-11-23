#pragma once

#include <string>

namespace m1arc {
void DrawArcUI();
// Floating feedback notification rendered below the control panel.
// Use only from M1 Arc's UI/game thread; never from REDscript or render hooks.
enum class ToastKind { Info, Warning, Success, Error };
void ShowToast(std::string message, float seconds = 3.2f,
               ToastKind kind = ToastKind::Info);
}
