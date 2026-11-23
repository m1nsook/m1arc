#pragma once
#include <imgui.h>

namespace m1arc::fonts {
// Font objects live in the existing M1 Arc ImGui atlas.
inline ImFont* body = nullptr;
inline ImFont* titleMedium = nullptr;
inline ImFont* titleBold = nullptr;
inline ImFont* titleHeavy = nullptr;
inline bool iconsAvailable = false;
void Load() noexcept;
void Clear() noexcept;
inline bool HasIcons() noexcept { return iconsAvailable; }
} // namespace m1arc::fonts
