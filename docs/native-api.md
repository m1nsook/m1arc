# Native REDscript API

M1 Arc registers these functions through the RED4ext plugin.
`r6/scripts/m1arc/M1Arc.reds` contains native declarations that must
match the installed `red4ext/plugins/m1arc/m1arc.dll`. The declarations
are **not** a new scripting language or a separate runtime.

The canonical API source is [M1Arc.reds on the native branch](https://github.com/m1nsook/m1arc/blob/native/r6/scripts/m1arc/M1Arc.reds).

| Function | Return | Purpose |
| --- | --- | --- |
| `M1Arc_Version()` | String | Framework version |
| `M1Arc_RegisterJson(modId, displayName, schemaJson)` | Bool | Register settings from a JSON schema |
| `M1Arc_GetJson(modId, settingId)` | String | Get JSON-encoded value |
| `M1Arc_ConsumeAction(modId, actionId)` | Bool | Consume one queued button press, if any |
| `M1Arc_GetBool(modId, settingId)` | Bool | Typed Boolean getter |
| `M1Arc_GetInt(modId, settingId)` | Int32 | Typed integer getter |
| `M1Arc_GetFloat(modId, settingId)` | Float | Typed float getter |
| `M1Arc_GetString(modId, settingId)` | String | Typed string/choice getter |
| `M1Arc_Revision()` | Int32 | Changes when settings are persisted |
| `M1Arc_SetJson(modId, settingId, payload)` | Bool | Set JSON value |
| `M1Arc_SetBool(modId, settingId, value)` | Bool | Set Boolean |
| `M1Arc_SetInt(modId, settingId, value)` | Bool | Set integer |
| `M1Arc_SetFloat(modId, settingId, value)` | Bool | Set float |
| `M1Arc_SetString(modId, settingId, value)` | Bool | Set string/choice |
| `M1Arc_ResetSetting(modId, settingId)` | Bool | Reset one value |
| `M1Arc_ResetMod(modId)` | Bool | Reset an entire mod |
| `M1Arc_SaveProfile(name)` | Bool | Save user settings |
| `M1Arc_LoadProfile(name)` | Bool | Restore user settings |
| `M1Arc_DeleteProfile(name)` | Bool | Delete a profile |

Button actions are events, not saved values. Define a `kind: "action"`
row or an inline numeric `button` in the mod schema and call
`M1Arc_ConsumeAction` from your REDscript gameplay logic.
The framework does not execute arbitrary console commands from JSON.

## Using values

~~~reds
let enabled: Bool = M1Arc_GetBool("example.my-mod", "enabled");
let limit: Int32 = M1Arc_GetInt("example.my-mod", "limit");
let revision: Int32 = M1Arc_Revision();
~~~

Typed getters return a zero/empty default when the ID is missing or
incorrectly typed. Use `M1Arc_GetJson` if your mod needs strict
missing-setting checks.

M1 Arc stores settings persistently and updates its native in-memory
registry immediately when users change controls. No CET Lua API is required.

For ordinary mod registration without script code, prefer the data-only
[manifest format](/mod-manifest). The optional scope/subtitle fields
are *manifest metadata*, not extra native RTTI parameters.
