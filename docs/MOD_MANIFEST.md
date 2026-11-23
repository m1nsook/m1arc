# M1 Arc mod manifests (native data-only)

M1 Arc provides a **native ImGui settings registry**, not a new script
engine. Each mod supplies its own normal REDscript under \`r6/scripts/\`.
Mod registration is automatic on game startup, from this directory:

\`\`\`text
Cyberpunk 2077/
├── red4ext/plugins/m1arc/m1arc.dll        # installed separately
├── red4ext/plugins/m1arc/mods/
│   └── m1nsook.weight/
│       ├── mod.json                         # required
│       ├── settings.json                    # required; UI schema, not saved values
│       └── language/                        # reserved for future translations
│           └── en_US.json
└── r6/scripts/M1ArcWeight/Weight.reds      # the mod's real gameplay code
\`\`\`

**\`mod.json\`**:

\`\`\`json
{
  "format": 1,
  "id": "m1nsook.weight",
  "name": "Weight Limit",
  "version": "0.1.0",
  "scope": "Gameplay",
  "shortDesc": "Customize V's carrying capacity",
  "requires": { "m1arc": "0.1.0" }
}
\`\`\`

The folder name **must match** \`id\` (ASCII letters, digits, \`.\`, \`-\`,
\`_\`; maximum 96 characters). Version strings use numeric \`major.minor.patch\`.
\`requires.m1arc\` is a minimum version (M1 Arc currently 0.1.0).
A missing \`requires\` field is permitted; including it is recommended.


## Mod scope and sidebar description

The **mod author** may add `scope` and `shortDesc` to `mod.json`.
Scope chooses the Phosphor icon displayed **before the mod name**;
`shortDesc` appears **under the name** in smaller Karla text.
These are visual metadata, not gameplay settings.

| Scope | Description | Phosphor icon |
| --- | --- | --- |
| `Gameplay` | Mechanics, combat AI, driving, economy, progression | `game-controller` |
| `Crafting` | Creation, upgrading, dismantling, recipes, components | `wrench` |
| `Cheats` | Debug menus, god mode, money/items, noclip, console | `terminal` |
| `Weapons` | Firearms, melee weapons, attachments, weapon balance | `sword` |
| `Locations` | Interiors, apartments, map and environment edits | `map-pin` |
| `Appearance` | Clothing, hairstyles, cosmetics, meshes, character creator | `t-shirt` |
| `Characters` | NPCs, companions, romances, interactions | `users` |
| `Miscellaneous` | Utilities, audio replacements, bug fixes, other mods | `toolbox` |
| `UI/UX` | User interface and HUD tweaks | `sliders` |

Exact spelling and capitalization matters. `scope` defaults to
`Miscellaneous` (toolbox icon) when omitted. `shortDesc` is optional;
when absent the sidebar retains its original one-line height.

`shortDesc` must be a JSON string of no more than **180 UTF-8 bytes**
without line breaks or control characters. Mod name remains the main
title; both title and subtitle are clipped to fit the sidebar, and the
full text is shown on hover. An unknown scope, incorrect field type,
or oversized description rejects only the corresponding mod at load.
The RED4ext log contains the rejected mod's ID and reason.

Old manifests and the three-argument `M1Arc_RegisterJson` native
function remain compatible. Only data-only manifests declare these
two optional display fields; no additional native API is required.
**\`settings.json\`** is an array of up to 250 native setting definitions:

\`\`\`json
[
  {
    "id": "enabled",
    "kind": "boolean",
    "label": "Custom weight limit",
    "description": "Restore vanilla capacity when disabled.",
    "default": true
  },
  {
    "id": "limit",
    "kind": "integer",
    "label": "Carry capacity (kg)",
    "min": 50,
    "max": 5000,
    "step": 10,
    "default": 500
  }
]
\`\`\`

Setting kinds: \`boolean\`, \`integer\`, \`float\`, \`choice\`. The existing
native registry validates IDs, ranges, defaults and choice members.
A broken manifest is skipped without preventing other mods from loading.
Files are read once per game process. Restart to add/remove mod manifests.

The mod's normal REDscript consumes live values from \`M1Arc_GetBool\`,
\`M1Arc_GetInt\`, \`M1Arc_Revision\`, etc. These are supplied by
\`r6/scripts/m1arc/M1Arc.reds\` installed with M1 Arc.

**Saved values are NOT written into the mod's schema.**
All user choices are persisted to the central
\`red4ext/plugins/m1arc/settings.json\` under \`values[modId]\` and restored
on the next game launch. Uninstalling a mod leaves its values in storage
so reinstalling can restore them.

## Native translations

M1 Arc ships framework language catalogs under
\`red4ext/plugins/m1arc/locales/en_US.json\`,
\`pl_PL.json\`, and \`ko_KR.json\`. The in-game language selector saves
\`overlay.language\` in M1 Arc's central settings.json. Additional
\`<locale>.json\` framework catalogs appear in the selector automatically.

Mods may override their own **display text only** using
\`red4ext/plugins/m1arc/mods/<mod-id>/language/<locale>.json\`.
For example, \`m1nsook.weight/language/ko_KR.json\`:

\`\`\`json
{
  "name": "무게 제한",
  "shortDesc": "소지 용량을 조절합니다",
  "settings": {
    "enabled": {
      "label": "활성화",
      "description": "무게 제한 기능을 켜거나 끕니다.",
      "group": "일반"
    },
    "mode": {
      "label": "모드",
      "choices": {
        "High": "높음",
        "Low": "낮음"
      }
    }
  }
}
\`\`\`

\`choices\` maps the **original choice value** to localized text. It
must never rename choice IDs in \`settings.json\` or REDscript; saved values
and native getters always use original IDs and options. Missing or invalid
translation entries fall back to the mod's English/default text.
No Lua, CET dependency, new scripting language, or execution is involved.


## Buttons, checkboxes and sections

M1 Arc uses existing native settings definitions. Set \`group\` to display
section headings; use \`presentation: "checkbox"\` for a Boolean checkbox
(the existing ON/OFF switch stays the default).

For example, add these rows to a mod's **settings.json** array:

\`\`\`json
[
  {
    "id": "ammo_count",
    "kind": "integer",
    "label": "Ammunition",
    "min": 1,
    "max": 999,
    "default": 50,
    "group": "Weapons",
    "button": { "id": "spawn_ammo", "label": "Spawn" }
  },
  {
    "id": "light_ammo",
    "kind": "boolean",
    "label": "Light Ammo",
    "default": true,
    "presentation": "checkbox",
    "group": "Weapons"
  },
  {
    "id": "heavy_ammo",
    "kind": "boolean",
    "label": "Heavy Ammo",
    "default": true,
    "presentation": "checkbox",
    "group": "Weapons"
  },
  {
    "id": "spawn_selected",
    "kind": "action",
    "label": "Spawn selected ammunition",
    "buttonLabel": "Spawn all",
    "group": "Weapons"
  }
]
\`\`\`

**An action is only a button press.** JSON cannot contain arbitrary
console commands, script callbacks or executable expressions.
The mod author implements its own game behavior in ordinary REDscript
and polls \`M1Arc_ConsumeAction("mod.id", "spawn_ammo")\`.
When it returns \`true\`, the mod may read \`M1Arc_GetInt\` and
\`M1Arc_GetBool\` and safely perform the requested gameplay action using
its own code. Each click is consumed once; up to 16 pending clicks are
stored per action until the owning mod consumes them. Presses are never
written into saved settings or profiles and do not increase the settings
revision counter.

A standalone \`kind: "action"\` row has **no** \`default\`; numeric and
Boolean controls require their ordinary defaults. Button IDs must be
unique and cannot collide with setting IDs. Localized button captions
use the per-setting \`buttonLabel\` in each mod's language JSON.
Old schemas work unchanged, and setting values remain the original IDs.

## Native mod hotkey controls

Mod authors may add \`kind: "hotkey"\` to a normal settings.json row.
This uses the same input router as the standalone M1 Arc menu and never
requires Lua, CET's \`registerHotkey\`, or a separate input hook.

\`\`\`json
{
  "id": "spawn_hotkey",
  "kind": "hotkey",
  "label": "Spawn ammunition hotkey",
  "description": "Press this key in-game to request ammunition.",
  "group": "Weapons",
  "default": 0
}
\`\`\`

\`default: 0\` means unassigned. Press the keybinding control in the
M1 Arc menu to capture a physical key, or **Clear** to unbind. Escape
cancels capture. The framework menu shortcut, Windows keys and modifier
keys cannot be used as mod shortcuts. For gameplay hotkeys, use plain
single keys rather than Ctrl/Alt/Win chords.

The key code is saved with other mod settings, and can be reset or saved
in profiles. Key presses are queued **only when the game window has focus
and the M1 Arc menu is closed**, once per key-down edge. A disabled mod
(\`enabled: false\` or \`active: false\`) does not receive hotkey presses.
Actions are not executed from JSON. Your REDscript mod polls the same
native one-shot action function used for buttons:

\`\`\`reds
if M1Arc_ConsumeAction("example.my-mod", "spawn_hotkey") {
  // Perform the gameplay action in this mod's REDscript code.
}
\`\`\`

\`M1Arc_GetInt("example.my-mod", "spawn_hotkey")\` returns the stored
Win32 virtual-key code, or 0 if unbound. The key press itself is
**not** saved as a setting and does not change the revision number.
The existing game input continues to receive the key normally.
