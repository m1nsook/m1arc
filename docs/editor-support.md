# Lua editor support

m1script includes editor-only **Lua Language Server** declarations at [`types/m1script.lua`](https://github.com/m1nsook/m1script/blob/main/types/m1script.lua). The file declares the actually exposed `m1core` Lua registry, `registerForEvent`, input helpers and utility functions.

This is **not a runtime module**. Do not place it inside `red4ext/plugins/m1core/mods/`; use it only as a workspace library for LuaLS or a compatible editor.

`api/contract.json` records the names, event callbacks, entrypoints and native RTTI APIs used to check that editor definitions match the developer reference.

The API is in **developer preview**. Verify new function names against the native implementation before documenting them as shipped.
