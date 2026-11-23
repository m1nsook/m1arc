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

The language directory is a **reserved extension point**, not a functional
translator yet. \`en_US.json\` files will be used when native localization is
implemented. Never include Lua, CET files or \`.m1s\`/\`.m1c\` entrypoints.
