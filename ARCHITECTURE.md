# M1 Arc — first source extraction

Source/reference: `m1core` backup branch in this repository,
specifically its **working `core/` implementation**. Never derive
input or rendering from the older `native/` prototype.

Extracted into the M1 Arc code:
- `core/src/imgui_impl/win32.*`: donor's working Win32 backend,
  with path-dependent INI initialization removed.
- `core/src/imgui_impl/dx12.*`: exact compatible custom DX12 backend.
- `core/src/d3d12/D3D12_Functions.cpp`: game-thread ImGui
  NewFrame/Render -> cloned DrawData -> Present thread DX12 draw.
- `core/src/window/window.cpp` + `core/src/VKBindings.cpp`:
  real game viewport WndProc, RAWINPUT physical key handling and
  cursor gating. M1 Arc isolates these concepts from the old Lua VM.
- `native/{registry,state}.*`: existing standalone REDscript settings
  and persistence, renamed to the M1 Arc namespace only.

The direct backend sources retain their original third-party attribution
in their file headers and use the main repository GPL-3.0 license.
No CET dependency, new scripting language or archived m1core source is
shipped with the DLL.

**Verification boundary**: CI compilation is not evidence of in-game
compatibility with CET or of input/cursor behavior. One targeted
coexistence test will still be needed before merging or releasing.
