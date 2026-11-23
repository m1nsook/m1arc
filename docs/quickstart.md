# Install M1 Arc

M1 Arc is a native **RED4ext** plugin for **Cyberpunk 2077 2.50**.
Install RED4ext and REDscript before installing M1 Arc.

## Download and install

1. Find an available version on [GitHub Releases](https://github.com/m1nsook/m1arc/releases) or [Nexus Mods](https://www.nexusmods.com/cyberpunk2077/mods/39311). Draft releases are not public.
2. Download its `game-root.zip` and SHA-256 checksum.
3. Close Cyberpunk 2077.
4. Extract the ZIP **contents** into the game directory containing `red4ext/` and `r6/` (not a new `package/` folder).
5. Launch the game and choose your M1 Arc menu key.

The ZIP installs `red4ext/plugins/m1arc/m1arc.dll`, its fonts and
language files, and `r6/scripts/m1arc/M1Arc.reds`. Regular releases do not
include the development-only UI Test fixture or gameplay mods.

Existing `red4ext/plugins/m1arc/settings.json` stores personal settings
and is not included in the release archive. Do not delete it while updating.

## Open the panel

The selected key opens and closes the panel. You can rebind it inside M1 Arc.
The panel includes light/dark themes, mod-specific controls and profile saving.

## Install mods that support M1 Arc

The framework itself is not a gameplay mod. Mod authors provide their own
`mod.json` and `settings.json` under
`red4ext/plugins/m1arc/mods/<mod-id>/`, plus any ordinary REDscript under
`r6/scripts/`. See the [manifest guide](/mod-manifest).

For errors or compatibility issues, read [troubleshooting](/troubleshooting).
