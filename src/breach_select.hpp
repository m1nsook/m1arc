#pragma once

#include <RED4ext/RED4ext.hpp>

namespace m1arc::breach {

// A narrow game-specific native helper. Never available unless the executable
// version AND a unique verified function entry match the known CP2077 2.5
// selector interface. All other builds fail closed.
void Initialize() noexcept;
bool Available() noexcept;
void RegisterNative() noexcept;

} // namespace m1arc::breach
