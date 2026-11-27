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

## Per-mod logs (M1 Arc 1.0.1+)

The header clipboard button switches between settings and the selected mod's
session logs. Registered mods can write their own diagnostic messages:

```swift
M1Arc_Log("example.mod", "INFO", "Initialization completed");
M1Arc_Log("example.mod", "WARNING", "Optional feature unavailable");
```

Use DEBUG, INFO, WARNING or ERROR, a registered mod ID and a non-empty message
of at most 2048 UTF-8 bytes. The function returns whether the message was accepted.
Avoid logging every frame. M1 Arc retains up to 256 entries per mod and 4096 in
total, newest first. Clear operates on the selected mod only; Copy Diagnostics
also includes shared framework startup information. Logs are
in-memory for the current game session; clearing them never touches disk logs.
Existing mods' private diagnostics require opt-in calls to M1Arc_Log.

The Copy Diagnostics report contains the selected mod's manifest version, current
settings, pending action counts, recent scoped logs, detected executable/DLL
file versions and framework startup rejection reasons. Review before sharing;
mod-authored log messages may contain data chosen by the mod author. Use stable
error codes and include the operation, inputs relevant to the failure, and reason
where known. Do not log secrets or every polling call. A consumed action is not
proof that the requested gameplay effect succeeded.

Mods that call this API should declare `"requires": { "m1arc": "1.0.1" }`
in their manifest. See the [complete logging and support guide](https://m1nsook.github.io/m1arc/diagnostics).

## Optional author metadata

Add these fields to an existing format-1 `mod.json` alongside its existing fields:

```json
{
  "author": "m1nsook",
  "nexusProfile": "m1nsook",
  "nexusProject": "https://www.nexusmods.com/cyberpunk2077/mods/39454",
  "description": "A longer description shown in the configuration header."
}
```

- `author`: display name, up to 120 UTF-8 bytes.
- `nexusProfile`: Nexus Mods username only (ASCII letters, digits, underscore or hyphen, up to 96 bytes). M1 Arc generates `https://www.nexusmods.com/profile/<username>`. No full URL or arbitrary external destination is accepted.
- `description`: optional full description, up to 1024 UTF-8 bytes. The header falls back to `shortDesc` if absent. Localized mod dictionaries may supply a translated `description`.
- The existing `version` is displayed in the sidebar and configuration header. Old manifests work unchanged; missing or malformed new metadata is omitted.

- `nexusProject`: optional full HTTPS Cyberpunk 2077 Nexus Mods project URL, with a numeric mod ID and optional trailing slash. Invalid or missing values hide the project button without rejecting the mod. The author link remains independent.

## Live refresh (developer workflow)

Use **Refresh** in the control panel toolbar after editing a mod's `mod.json`, `settings.json`, or `language/*.json`. The framework rescans all mod directories and applies validated definitions in the next UI frame. Existing saved values are normalized against the new schema; new or incompatible values use their declared defaults, and numeric values may be clamped to new bounds. Saved values, profiles, and the framework's own global settings file are not rewritten or reloaded by Refresh. Successful refreshes advance `M1Arc_Revision()` so mods that poll the revision can re-read settings.

If a mod's files are missing or invalid, its last registered definition remains active. Script-only mods are not removed. New file-backed definitions can be discovered but installing, removing, or changing REDscript, native DLLs, archives, TweakDB data, or gameplay initialization still requires the relevant game reload/restart. Successful schema reloads clear pending action requests for that mod to avoid stale clicks. Copy Diagnostics includes the most recent rescan results. No executable code is reloaded or executed by this feature.

### Diagnostics viewer

The diagnostics view shows per-severity counts, a searchable compact log table, and a pause/resume control. Pause freezes the selected mod's visible list only; logging continues. Select any row to inspect and copy its full message. Copy Diagnostics always copies the live, unfiltered support report. Clear Logs requires confirmation and clears only the selected mod's current-session history. Switching mods resets the view filters and pause state. Search ignores ASCII case and matches other UTF-8 text exactly.
