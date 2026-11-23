# M1 Arc mod manifests

M1 Arc reads mod settings from JSON. Game behavior remains in the mod's own
REDscript code.

## Files

```text
Cyberpunk 2077/
  red4ext/plugins/m1arc/mods/example.mod/
    mod.json
    settings.json
    language/                 # optional translated labels
      ko_KR.json
  r6/scripts/ExampleMod/Example.reds
```

## mod.json

```json
{
  "format": 1,
  "id": "example.mod",
  "name": "Example Mod",
  "version": "1.0.0",
  "scope": "Weapons",
  "shortDesc": "Configure ammunition",
  "requires": { "m1arc": "1.0.0" }
}
```

The folder name and `id` must match. `scope` selects a sidebar icon;
`shortDesc` appears below the mod name. Both are optional.

## settings.json

```json
[
  {
    "id": "ammo",
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
    "id": "spawn_key",
    "kind": "hotkey",
    "label": "Spawn hotkey",
    "default": 0,
    "group": "Weapons"
  }
]
```

Supported kinds: `boolean`, `integer`, `float`, `choice`, `action`, and
`hotkey`. The `group` field creates a section. Button and hotkey events are
consumed by the owning mod using `M1Arc_ConsumeAction(modId, actionId)`.
JSON does not contain executable commands.

M1 Arc saves user settings and profiles centrally in
`red4ext/plugins/m1arc/settings.json`; it does not overwrite a mod's schema.
Language files change visible labels, never stored setting IDs or choice values.

For complete schemas, localized settings, available scopes, and native API
examples, use the **[official M1 Arc documentation](https://m1nsook.github.io/m1arc/)**.
