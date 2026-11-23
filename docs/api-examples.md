# Scripting examples

Unlike the original website starter, these examples are real Lua callbacks for m1core, not VitePress APIs.

## Hello world

~~~lua
registerForEvent("onInit", function()
    print("[hello] m1script ready")
end)
registerForEvent("onShutdown", function()
    print("[hello] goodbye")
end)
~~~

[Hello example source](https://github.com/m1nsook/m1script/blob/main/examples/hello/init.m1s)

## Persistent settings

~~~lua
local MOD_ID = "example.m1script_settings"
local SCHEMA = [[
[
  {"id":"enabled","kind":"boolean","default":true},
  {"id":"interval","kind":"float","default":0.5,"min":0.1,"max":5,"step":0.1}
]
]]

registerForEvent("onInit", function()
    assert(m1core.registerJson(MOD_ID, "m1script Example", SCHEMA))
end)

registerForEvent("onUpdate", function(dt)
    if m1core.getJson(MOD_ID, "enabled") == "true" then
        -- Use game-safe logic here, not a busy loop.
    end
end)
~~~

[Settings example source](https://github.com/m1nsook/m1script/blob/main/examples/settings/init.m1s)

## Real Weight Limit integration

For a mod that applies a temporary stat modifier and removes it in `onShutdown`, see the [m1core Weight Limit sample](https://github.com/m1nsook/CP2077/blob/dev/m1core-cet-foundation/m1core/examples/weight-limit/init.m1s). Its Lua source and compiled bytecode variants are tested by native Windows CI.

