# M1 Arc

**Native RED4ext extension and settings interface for Cyberpunk 2077.**

M1 Arc is a standalone RED4ext DLL with a built-in Dear ImGui settings
window and native functions that can be called from REDscript.

- No Lua runtime or custom script formats.
- Does not replace, alter, or depend on Cyber Engine Tweaks.
- Uses its own DLL, its own shortcut and its own ImGui context.
- Mod settings are registered through native REDscript RTTI functions,
  persisted to `settings.json`, and updated as soon as a control changes.
- Author: **민숙 MISO @m1nsook**.

## Scope of this first branch

The rendering/input integration is an **extraction of the known-working
`m1core/core` approach**, not the earlier `m1core/native` prototype:
game-thread ImGui frame preparation, game-present DX12 rendering, the same
custom Win32 and DX12 ImGui backend sources, physical keyboard RAWINPUT,
and a native `ForceCursor` reason.

In particular, the old m1core scripting VM, script-file formats, console,
TweakDB editor and CET branding are **not** included. The original
`m1core` backup branch remains unchanged.

## Install

Windows Actions produces **one game-root artifact**, containing
`red4ext/plugins/m1arc/m1arc.dll` and the repository LICENSE.

On first launch the plugin prompts for an independent keyboard shortcut.
It does not bind or edit CET's existing shortcut.

This is **not a public release**. Windows CI validates compilation and
settings persistence, but only an actual Cyberpunk 2077 session with CET
installed can confirm that both overlays and their input coexist correctly.

## Native API

M1 Arc exports these native REDscript RTTI global functions:

- `M1Arc_Version() -> String`
- `M1Arc_RegisterJson(modId, displayName, schemaJson) -> Bool`
- `M1Arc_GetJson(modId, settingId) -> String`
- `M1Arc_SetJson(modId, settingId, payloadJson) -> Bool`
- `M1Arc_ResetSetting(modId, settingId) -> Bool`
- `M1Arc_ResetMod(modId) -> Bool`
- `M1Arc_SaveProfile(name) -> Bool`
- `M1Arc_LoadProfile(name) -> Bool`
- `M1Arc_DeleteProfile(name) -> Bool`

The JSON setting definitions support booleans, integers, floats and choices,
including groups and descriptions. See `src/registry.hpp`.

There is no remote update service or additional scripting engine.

## Development rules

This branch is strictly focused on working RED4ext + ImGui initialization,
correct input release and native REDscript settings. Do not add unrelated
features or repeat previously verified m1core testing. The `m1core`
backup branch is read-only.
