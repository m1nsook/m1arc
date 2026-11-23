# Settings, profiles and themes

Each mod registers native settings. M1 Arc displays them in its in-game
Dear ImGui control panel. The current schema supports Boolean toggles,
integer/float sliders, choice controls, checkboxes, action buttons,
per-mod keyboard hotkeys and sections. See [mod manifests](/mod-manifest) for author configuration.

Changes are stored in `red4ext/plugins/m1arc/settings.json` and applied
to registered native values immediately. The stored file is user data:
back it up before manual edits or uninstalling the framework.

## Profiles

M1 Arc can save, load or delete named profiles from its control panel.
A profile stores a snapshot of configurable mod settings. Loading a
profile restores compatible values without deleting unrelated settings.

REDscript authors can also call
`M1Arc_SaveProfile`, `M1Arc_LoadProfile` and `M1Arc_DeleteProfile`.
See [native API](/native-api) for the exact declarations.

## Light and dark modes

The theme toggle is part of the M1 Arc panel itself, not part of an
installed mod's settings. It is remembered across restarts.
The default theme for a first-time user is light.

## Language selection

The language menu in the panel footer remembers your chosen language
across restarts. English, Polish and Korean are included. Mod settings
values and profiles never change when you switch languages.
See [languages and translations](/localization).

## Mod scope and short description

Mod authors choose these display-only values in `mod.json`:
`scope` selects a Phosphor icon and `shortDesc` adds a small subtitle
below the mod name. The list is documented in [mod manifests](/mod-manifest).
