# Architecture

M1 Arc is a Windows **C++ RED4ext DLL** for Cyberpunk 2077, with a
Dear ImGui interface rendered using the game's DX12 frame integration.

| Part | Responsibility |
| --- | --- |
| `m1arc.dll` | Native registry, state storage, UI, input ownership and game-safe RED4ext bridge |
| `M1Arc.reds` | Declarations for native REDscript functions |
| `mod.json` | Author-selected mod identity, scope and sidebar description |
| `settings.json` in each mod | Declarative control definitions |
| `red4ext/plugins/m1arc/settings.json` | Player-selected values, profiles, theme and panel shortcut |

The framework is independent of Cyber Engine Tweaks. It does not embed
Lua, run a new script VM, or support the retired `.m1s` and `.m1c`
file types.

M1 Arc has an optional game-native input bridge for the separately
installed Skip Breach mod. The bridge is dormant until that mod enables
it; M1 Arc alone does not run a Breach Protocol solver.

See [native development](https://github.com/m1nsook/m1arc/tree/native)
and [GitHub Releases](https://github.com/m1nsook/m1arc/releases).
