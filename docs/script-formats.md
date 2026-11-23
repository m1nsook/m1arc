# Script formats and modules

m1script currently means **LuaJIT scripting** with custom file extensions; it is not a new source grammar.

| File | Meaning |
| --- | --- |
| `init.m1s` | Preferred Lua source |
| `init.lua` | Compatible Lua source |
| `init.m1c` | Genuine LuaJIT bytecode |

If multiple entrypoints are found, m1core chooses `init.m1s`, then `init.lua`, then `init.m1c`. Install just one entrypoint format in each mod package.

## Modules

Use `require("helper")` to load a local `helper.m1s` or compatible module:

~~~text
mods/my-mod/
  init.m1s
  helper.m1s
~~~

~~~lua
local helper, err = require("helper")
if helper then
    registerForEvent("onInit", helper.start)
else
    print("[my-mod] helper failed:", err)
end
~~~

Module resolution checks bare names, then `.m1s`, `.lua`, `.m1c`, and module-directory `init` files. Sandbox path containment is enforced; absolute paths and directory traversal are not supported.

`.m1c` files must contain LuaJIT bytecode compiled for a compatible LuaJIT VM. Renaming ordinary Lua source or standard Lua bytecode to `.m1c` does not work. Source format is easier to debug.

Reloading discards previous mod script contexts; mods should clean up their own game-side changes in `onShutdown`.

