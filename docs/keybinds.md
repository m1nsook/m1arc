# Mod keybinds

Register actions at **top level during entrypoint loading**, not from `onInit`.

~~~lua
registerHotkey("toggle_feature", "Toggle feature",
               "Press to toggle this feature", function()
    print("[my-mod] feature toggled")
end)

registerInput("hold_feature", "Hold feature",
              "Called on key down/up", function(isDown)
    print("Held:", isDown)
end)
~~~

| API | Behavior |
| --- | --- |
| `registerHotkey(id, name, description, callback)` | Runs when a configured key combination completes |
| `registerInput(id, name, description, callback)` | Invokes callback with `isDown: boolean` |
| `IsBound(id)` | Returns whether this mod action has a binding |
| `GetBind(id)` | Returns the displayed key string |

Both registration calls have a three-argument overload `(id, name, callback)`. IDs must use alphanumeric characters, underscore or dot, and be unique within a mod. Keys are scoped by mod directory name, and user assignments persist in `red4ext/plugins/m1core/bindings.json`.

Players configure registered actions in the **Keybinds** section of m1core's Dear ImGui overlay. Scripts must not replace m1core's Win32 input handling.

