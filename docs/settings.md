# Settings and profiles

The global `m1core` table bridges Lua to the standalone native registry.

~~~lua
local MOD_ID = "example.my_mod"
local SCHEMA = [[
[
  {"id":"enabled","kind":"boolean","label":"Enabled","default":true},
  {"id":"level","kind":"integer","label":"Level","default":3,"min":0,"max":10,"step":1}
]
]]
registerForEvent("onInit", function()
    assert(m1core.registerJson(MOD_ID, "My Mod", SCHEMA))
end)
~~~

| Function | Returns |
| --- | --- |
| `m1core.registerJson(modId, title, schemaJson)` | boolean |
| `m1core.getJson(modId, settingId)` | JSON string or `"null"` |
| `m1core.setJson(modId, settingId, valueJson)` | boolean |
| `m1core.resetSetting(modId, settingId)` | boolean |
| `m1core.resetMod(modId)` | boolean |
| `m1core.saveProfile(name)` | boolean |
| `m1core.loadProfile(name)` | boolean |
| `m1core.deleteProfile(name)` | boolean |
| `m1core.profiles()` | array-like Lua table |
| `m1core.lastError()` | string, potentially empty |

Valid setting kinds are `boolean`, `integer`, `float` and `choice`. IDs are 1–96 characters from letters, digits, `_`, `-` and `.`. A schema contains 1–250 unique settings. Numbers are clamped to the declared range; choices must match an option. Wrong JSON types are rejected.

Values and global profiles are saved in `red4ext/plugins/m1core/settings.json`, separate from runtime configuration and input bindings. A saved profile can affect **every registered mod**, including settings for unloaded mods; only change a profile in response to deliberate user action.

~~~lua
local enabled = m1core.getJson(MOD_ID, "enabled") == "true"
m1core.setJson(MOD_ID, "enabled", "false")
~~~

See [complete API reference](./api-reference) for schema details.

