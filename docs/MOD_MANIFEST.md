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
  "requires": { "m1arc": "0.1.0" }
}
\`\`\`

The folder name **must match** \`id\` (ASCII letters, digits, \`.\`, \`-\`,
\`_\`; maximum 96 characters). Version strings use numeric \`major.minor.patch\`.
\`requires.m1arc\` is a minimum version (M1 Arc currently 0.1.0).
A missing \`requires\` field is permitted; including it is recommended.

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
