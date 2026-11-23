#pragma once

#include <string>

namespace m1arc {
void DrawArcUI();
// NEW: Queue a transient toast at the top of the control panel.
// Must be called from the UI/game thread while the panel is open.
void ShowToast(std::string message, float seconds = 3.2f);
}
