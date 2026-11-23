# Lifecycle and events

At the top level of `init.m1s`, `init.lua` or `init.m1c`, call `registerForEvent(name, callback)`.

| Event | Callback | Use |
| --- | --- | --- |
| `onHook` | `function()` | Initial scripting hook stage |
| `onTweak` | `function()` | TweakDB stage |
| `onInit` | `function()` | Mod and game initialization |
| `onUpdate` | `function(deltaTime)` | Per-frame update; elapsed seconds |
| `onDraw` | `function()` | ImGui draw callbacks |
| `onOverlayOpen` | `function()` | m1core overlay opened |
| `onOverlayClose` | `function()` | m1core overlay closed |
| `onShutdown` | `function()` | Mod unload/reload; release owned resources |

~~~lua
local elapsed = 0

registerForEvent("onUpdate", function(dt)
    elapsed = elapsed + (dt or 0)
    if elapsed < 0.5 then return end
    elapsed = 0
    -- Do a periodic, game-safe update.
end)

registerForEvent("onShutdown", function()
    -- Remove your own temporary modifiers or registered listeners.
end)
~~~

A mod's unhandled Lua exception is reported by the per-mod logger. **Event-name labels are not guaranteed in the current runtime.** A failing callback is isolated so other mods can continue receiving updates. The framework restores ImGui style and availability after a mod's `onDraw` or tooltip callback.

Only call ImGui from a permitted draw or tooltip context, not arbitrary callbacks. A mod must undo game-side effects it owns; m1core cannot automatically reverse game modifications made by scripts.

## Reloading

The **Reload scripts** action in m1core Logs schedules a reload on the native update loop. Initialized mod contexts receive `onShutdown` before their script generation is discarded and fresh entrypoints load. A failed mod is logged and other mod directories can still be processed. Native CI runs real LuaJIT script-format tests and simulated Weight Limit reload tests. A dedicated callback-failure regression was attempted but reverted after CI failure; not all error cases have been validated inside the game.

