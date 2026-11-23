# Mod manifest and settings schema

M1 Arc loads data-only manifests automatically at startup.
It does **not** run Lua, custom `.m1s` files, or a new scripting language.

## File structure

~~~text
Cyberpunk 2077/
  red4ext/plugins/m1arc/mods/example.my-mod/
    mod.json
    settings.json
    language/
      ko_KR.json      # optional display translations
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

Supported setting kinds: `boolean`, `integer`, `float`, `choice` and `action`.
Each mod may define up to 250 setting controls. Invalid manifests are
skipped individually and logged; they do not prevent other mods from loading.

Player values are saved centrally in
`red4ext/plugins/m1arc/settings.json`, never into the mod's schema.
Manifests are scanned at startup, so restart the game after adding a mod.

## Buttons, checkboxes and sections

The existing \`group\` setting now displays a section heading such as
**Weapons**. To present a Boolean setting as a checkbox instead of the
default ON/OFF switch, use \`"presentation": "checkbox"\`.

A numeric control can also have a one-shot action button beside it:

~~~json
[
  {
    "id": "ammo_count",
    "kind": "integer",
    "label": "Ammunition",
    "default": 50,
    "min": 1,
    "max": 999,
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
~~~

The \`action\` kind is a standalone button and **must not** have a
\`default\` value. A numeric \`button\` contains a distinct action ID and
display label. All IDs must be unique. Group headings are rendered only
when a mod uses groups other than the default General section.

Button clicks are queued **in memory** and consumed once by a
mod's REDscript code:

~~~reds
if M1Arc_ConsumeAction("example.my-mod", "spawn_ammo") {
  let amount: Int32 = M1Arc_GetInt("example.my-mod", "ammo_count");
  let light: Bool = M1Arc_GetBool("example.my-mod", "light_ammo");
  let heavy: Bool = M1Arc_GetBool("example.my-mod", "heavy_ammo");
  // Handle the requested action here using your mod's gameplay code.
}
~~~

Call this in your mod's normal update/gameplay logic. M1 Arc **does not**
execute arbitrary commands specified in JSON and never bundles spawn
scripts. Actions are limited to 16 pending clicks per action, not
persisted in profiles or settings, and do not alter the settings revision.
Translate captions using \`buttonLabel\` in the per-mod
[language JSON](/localization).

## Read the setting from REDscript

After installing M1 Arc's matching `M1Arc.reds` native declarations,
normal REDscript can read values:

~~~reds
let enabled: Bool = M1Arc_GetBool("example.my-mod", "enabled");
let limit: Int32 = M1Arc_GetInt("example.my-mod", "limit");
~~~

See [native API](/native-api) for typed getters, setters and profiles.
For translated mod names, setting labels, descriptions and choice display
text, see [JSON localization](/localization).
