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

M1 Arc reads mod settings from JSON and exposes typed native functions to
REDscript. Game actions belong to the mod that handles them.

See the [native API](/native-api), [mod manifests](/mod-manifest), and
[GitHub repository](https://github.com/m1nsook/m1arc).
