# m1script roadmap

m1script is the **scripting contract and developer tooling** for the standalone m1core RED4ext framework. Work on the public API starts here; native implementation remains in m1core.

## Delivered

- Public VitePress reference and guides for real m1core LuaJIT APIs.
- Source `.m1s`, compatibility `.lua`, and compiled LuaJIT `.m1c` documentation.
- Mod lifecycle events and hotkey/input API reference.
- Native settings and profile APIs for Lua and REDscript.
- Editor-only LuaLS annotations, JSON API manifest and small runnable source examples.
- Automated contract/declaration consistency checking and GitHub Pages deployment.

## Next milestones

### API contract reliability

Verify exported API lists against source or generated metadata from native m1core, rather than relying only on a manually maintained name list. Introduce clearly marked **experimental**, **implemented**, and **deprecated** states when needed.

### Authoring developer experience

Improve Lua Language Server declarations (including callback-specific types), add tested patterns for file/module loading and error handling, and establish versioned examples. Revisit named callback failure diagnostics only with a passing real LuaJIT regression and native Windows build. Annotations must remain editor-only and may not be installed as a mod.

### Stability and compatibility

Define a versioned public API policy and document how updates, breaking changes and compiled LuaJIT bytecode compatibility are handled. In-game behavior requires actual Cyberpunk testing; tests and docs alone are not game validation.

## Runtime boundaries

Do not turn m1script into a mandatory extra runtime. m1core remains the standalone native DLL with LuaJIT, RED4ext hooks, game bridge, settings/persistence, input, and accepted ImGui interface.

See the [architecture guide](./architecture) and [project handoff](https://github.com/m1nsook/m1script/blob/main/PROJECT_STATUS.md).
