#pragma once

#include <string>

namespace m1arc {
void DrawArcUI();
// Floating feedback notification rendered below the control panel.
// Thread-safe enqueue; rendering stays on the existing UI/game thread.
enum class ToastKind { Info, Warning, Success, Error };
void ShowToast(std::string message, float seconds = 3.2f,
               ToastKind kind = ToastKind::Info);
}
