# Native REDscript functions

m1core exposes these native RTTI functions through its standalone RED4ext DLL. These are **REDscript calls**, not functions on the Lua `m1core` table.

| Function | Parameters (all String) | Returns |
| --- | --- | --- |
| `M1Core_Version` | none | String |
| `M1Core_RegisterJson` | modId, displayName, schemaJson | Bool |
| `M1Core_GetJson` | modId, settingId | String |
| `M1Core_SetJson` | modId, settingId, payload | Bool |
| `M1Core_ResetSetting` | modId, settingId | Bool |
| `M1Core_ResetMod` | modId | Bool |
| `M1Core_SaveProfile` | name | Bool |
| `M1Core_LoadProfile` | name | Bool |
| `M1Core_DeleteProfile` | name | Bool |

Native registration is implemented in [m1core/core/src/m1core_registry_api.cpp](https://github.com/m1nsook/CP2077/blob/dev/m1core-cet-foundation/m1core/core/src/m1core_registry_api.cpp). Register schemas after m1core's RTTI initialization.

Lua and REDscript settings share the same persistence and registry backend; a value changed from either API becomes visible through the other. See [Settings](./settings) and the [complete reference](./api-reference).

