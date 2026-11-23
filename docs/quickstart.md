# Quick start

m1script files are LuaJIT scripts running inside the standalone **m1core** RED4ext DLL. Install the successful [m1core Windows CI game-root package](https://github.com/m1nsook/CP2077/actions) into Cyberpunk 2077.

Create `red4ext/plugins/m1core/mods/my-first-mod/init.m1s`:

~~~lua
registerForEvent("onInit", function()
    print("[my-first-mod] initialized")
end)

registerForEvent("onShutdown", function()
    print("[my-first-mod] shutting down")
end)
~~~

Launch Cyberpunk. m1core loads each mod's entrypoint automatically. No separate CET or m1script binary is installed. Use `onShutdown` to clean up game modifiers or listeners **owned by your mod**.

Read [Lifecycle](./lifecycle), [Settings](./settings), [Keybinds](./keybinds), [Formats](./script-formats), and the [complete reference](./api-reference).

