#pragma once

#include <RED4ext/RED4ext.hpp>

namespace m1arc::breach {

// Optional native input capability built into M1 Arc itself.
// It remains inert without the separately installed + enabled Skip Breach mod.
// Real Win32 clicks are independent of the old internal selector signature;
// both paths still require a supported game build and a safe input context.
void Initialize() noexcept;
bool Available() noexcept;
// Human-readable idle status logged once at plugin startup; never initiates input.
const char* StatusMessage() noexcept;
void RegisterNative() noexcept;

} // namespace m1arc::breach
