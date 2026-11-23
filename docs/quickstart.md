# Install M1 Arc

M1 Arc is a standalone native **RED4ext** plugin for **Cyberpunk 2077 2.50**.
You need RED4ext and REDscript installed. Cyber Engine Tweaks can stay
installed, but M1 Arc does not depend on it.

## Download and install

1. Open [GitHub Releases](https://github.com/m1nsook/m1arc/releases).
   Draft builds are initially visible only to the repository maintainer.
2. Download the versioned `m1arc-v...-game-root.zip` and its SHA-256 text file
   when the chosen release is available.
3. Close Cyberpunk 2077 before changing files.
4. Extract the **contents** of the ZIP into the **Cyberpunk 2077 game directory**,
   where `red4ext/` and `r6/` already exist. Do not create a nested `package/` folder.
5. Launch Cyberpunk and set a separate M1 Arc panel shortcut when prompted.

The ZIP must have these paths:

~~~text
Cyberpunk 2077/
  red4ext/plugins/m1arc/m1arc.dll
  red4ext/plugins/m1arc/LICENSE
  red4ext/plugins/m1arc/fonts/
    Karla-SemiBold.ttf
    ChakraPetch-SemiBold.ttf
    Phosphor-Regular.ttf
    ... font notices
  r6/scripts/m1arc/M1Arc.reds
~~~

The release ZIP deliberately does **not** contain the temporary
`dev-preview.enabled` marker, any gameplay mod, proprietary Blender Pro
fonts or a separate AutoBreach DLL.

Existing `red4ext/plugins/m1arc/settings.json` stores personal settings
and is not included in the release archive. Do not delete it while updating.

## Open the panel

M1 Arc uses the key you chose at first launch and stores it independently
from CET. Use the panel's binding control if you wish to change the key.
The panel includes light/dark themes, mod-specific controls and profile saving.

## Install mods that support M1 Arc

The framework itself is not a gameplay mod. Mod authors provide their own
`mod.json` and `settings.json` under
`red4ext/plugins/m1arc/mods/<mod-id>/`, plus any ordinary REDscript under
`r6/scripts/`. See the [manifest guide](/mod-manifest).

For errors or compatibility issues, read [troubleshooting](/troubleshooting).
