# Mod manifest and settings schema

M1 Arc loads data-only manifests automatically at startup.
It does **not** run Lua, custom `.m1s` files, or a new scripting language.

## File structure

~~~text
Cyberpunk 2077/
  red4ext/plugins/m1arc/mods/example.my-mod/
    mod.json
    settings.json
  r6/scripts/ExampleMyMod/Example.reds
~~~

The folder name must match the manifest `id`, which may contain letters,
numbers, periods, hyphens and underscores (maximum 96 characters).

## mod.json

~~~json
{
  "format": 1,
  "id": "example.my-mod",
  "name": "Example Mod",
  "version": "0.1.0",
  "scope": "Gameplay",
  "shortDesc": "Configure the mod's gameplay behavior",
  "requires": { "m1arc": "0.1.0" }
}
~~~

`scope` and `shortDesc` are **optional** sidebar metadata.
The scope determines which Phosphor icon appears before the mod name.
The short description appears underneath in a smaller font.

| Scope | Description | Phosphor icon |
| --- | --- | --- |
| Gameplay | Mechanics, combat, driving, economy, progression | `game-controller` |
| Crafting | Recipes, upgrades, dismantling, components | `wrench` |
| Cheats | Debug tools, money, noclip and console utilities | `terminal` |
| Weapons | Guns, blades, attachments and balance | `sword` |
| Locations | Interiors, apartments, maps and environments | `map-pin` |
| Appearance | Clothing, hair, body meshes and cosmetics | `t-shirt` |
| Characters | NPCs, companions, romances and interactions | `users` |
| Miscellaneous | Utilities, audio replacements and bug fixes | `toolbox` |
| UI/UX | HUD and interface improvements | `sliders` |

The nine names are case-sensitive and must match the table.
Without `scope`, M1 Arc uses `Miscellaneous`.
Without `shortDesc`, the original single-line sidebar row remains.
Descriptions must be plain strings of **180 UTF-8 bytes or fewer**, without
control characters or line breaks.

## settings.json

This is the **schema defining controls**, not the player's saved values:

~~~json
[
  {
    "id": "enabled",
    "kind": "boolean",
    "label": "Enabled",
    "description": "Enable the mod",
    "default": true
  },
  {
    "id": "limit",
    "kind": "integer",
    "label": "Carry capacity",
    "min": 50,
    "max": 5000,
    "step": 10,
    "default": 500
  },
  {
    "id": "mode",
    "kind": "choice",
    "label": "Mode",
    "choices": ["vanilla", "balanced", "custom"],
    "default": "balanced"
  }
]
~~~

Supported setting kinds: `boolean`, `integer`, `float` and `choice`.
Each mod may define up to 250 setting controls. Invalid manifests are
skipped individually and logged; they do not prevent other mods from loading.

Player values are saved centrally in
`red4ext/plugins/m1arc/settings.json`, never into the mod's schema.
Manifests are scanned at startup, so restart the game after adding a mod.

## Read the setting from REDscript

After installing M1 Arc's matching `M1Arc.reds` native declarations,
normal REDscript can read values:

~~~reds
let enabled: Bool = M1Arc_GetBool("example.my-mod", "enabled");
let limit: Int32 = M1Arc_GetInt("example.my-mod", "limit");
~~~

See [native API](/native-api) for typed getters, setters and profiles.
