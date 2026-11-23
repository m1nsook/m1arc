# M1 Arc

**Native RED4ext extension and settings interface for Cyberpunk 2077.**

M1 Arc is a standalone RED4ext DLL with a built-in Dear ImGui settings
window and native functions that can be called from REDscript.

- No Lua runtime or custom script formats.
- Does not replace, alter, or depend on Cyber Engine Tweaks.
- Uses its own DLL, its own shortcut and its own ImGui context.
- Mod settings are registered through native REDscript RTTI functions,
  persisted to `settings.json`, and updated as soon as a control changes.
- Author: **민숙 MISO @m1nsook**.

## Scope of this first branch

The rendering/input integration is an **extraction of the known-working
`m1core/core` approach**, not the earlier `m1core/native` prototype:
game-thread ImGui frame preparation, game-present DX12 rendering, the same
custom Win32 and DX12 ImGui backend sources, physical keyboard RAWINPUT,
and a native `ForceCursor` reason.

In particular, the old m1core scripting VM, script-file formats, console,
TweakDB editor and CET branding are **not** included. The original
`m1core` backup branch remains unchanged.

## Install

Windows Actions produces **one game-root artifact**, containing
`red4ext/plugins/m1arc/m1arc.dll`, `r6/scripts/m1arc/M1Arc.reds`, and the repository LICENSE.

The native UI uses **Karla SemiBold (600), 18 px** for readable main
text and **Phosphor Regular icons** at matching size. GitHub Actions
generates fixed-weight TTFs from the pinned Karla variable-font source;
the runtime never depends on the variable font's default axis value.
Karla Medium (500) is a fallback, and Bold (700) is available for
headings. Light and dark muted text has stronger contrast and disabled
controls retain sufficient opacity. Font sizing is intentionally designed
for in-game readability rather than extra-light/thin styling. Their OFL/MIT license notices ship in the same package. The
supplied `fonts.zip` contains Blender Pro Medium, Bold and Heavy, which are
proprietary and deliberately **not published** by this repository: if you
have the right to use them, extract those three files directly to
`red4ext/plugins/m1arc/fonts/` to enable Blender Pro headings. **Chakra Petch SemiBold/Bold** headings are bundled and used whenever Blender
Pro is not installed locally; Karla SemiBold is the third fallback.
Blender Pro is your preferred heading font, not an ignored design resource.
The supplied ZIP contains fonts but no accompanying distribution terms,
so it has not been copied into publicly downloadable releases.

**Light is the first-run default**. The light/dark toggle stores a choice in
`red4ext/plugins/m1arc/settings.json`, and existing saved preferences always win.
If an older installation previously saved `"theme": "dark"`, it will
keep starting dark until switched to light once.

The native panel header reflows to two or three rows on narrower viewports.
Sidebar mod titles are ellipsized (with full-name hover tooltips); setting
labels wrap and controls stack if needed. Small windows are kept inside the
visible game viewport instead of extending beyond the screen. Theme colors
avoid black text on dark backgrounds while preserving the approved palette.

On the **first launch only**, M1 Arc prompts for its independent keyboard
shortcut. The binding is persisted immediately to
`red4ext/plugins/m1arc/settings.json` as `overlay.hotkey` (a Windows
virtual-key code) and restored **before** first-run setup checks whether
a binding is missing. The prompt should not reappear after restarting
Cyberpunk 2077, unless the saved key is missing or invalid. Rebinding
through the header button saves the replacement too. M1 Arc never
changes CET's own shortcut.

This is **not a public release**. Windows CI validates compilation and
settings persistence, but only an actual Cyberpunk 2077 session with CET
installed can confirm that both overlays and their input coexist correctly.

## Temporary UI Test mod (draft CI only)

The development build includes a marker at
`red4ext/plugins/m1arc/dev-preview.enabled`. While that marker exists,
M1 Arc registers the **M1 ARC UI Test** mod through its real native
settings registry. It is a UI fixture, **not a gameplay mod**. It has
all currently supported settings controls: ON/OFF buttons, integer and
float sliders, and choice arrows, plus long labels/choices, a Latin
Extended sample, hover descriptions, an enable/disable master switch,
and 24 extra rows for vertical scroll testing.

To remove the temporary mod, **delete `dev-preview.enabled` and
restart the game**. The rest of M1 Arc is unaffected. Existing preview
values can remain in settings.json but are not shown without the marker.
Public releases must omit the development marker. The separate language
selector and translated text files are planned as native localization,
not as an invented scripting format.

## Installing mods and declaring settings

Mods can register **without any runtime registration script**: put
`mod.json` and `settings.json` under
`red4ext/plugins/m1arc/mods/<mod-id>/`, and install that mod's normal
REDscript under `r6/scripts/`. M1 Arc scans and validates manifests
at startup. See [Mod manifest specification](docs/MOD_MANIFEST.md).

The sample carry-weight mod lives in the [CP2077 repository](https://github.com/m1nsook/CP2077)
and uses this folder convention.

## Native API

M1 Arc exports these native REDscript RTTI global functions:

- `M1Arc_Version() -> String`
- `M1Arc_RegisterJson(modId, displayName, schemaJson) -> Bool`
- `M1Arc_GetJson(modId, settingId) -> String`
- `M1Arc_GetBool/GetInt/GetFloat/GetString(modId, settingId) -> value` (typed live getters)
- `M1Arc_Revision() -> Int32` (increments whenever persisted settings change)
- `M1Arc_SetJson(modId, settingId, payloadJson) -> Bool`
- `M1Arc_SetBool/SetInt/SetFloat/SetString(modId, settingId, value) -> Bool`
- `M1Arc_ResetSetting(modId, settingId) -> Bool`
- `M1Arc_ResetMod(modId) -> Bool`
- `M1Arc_SaveProfile(name) -> Bool`
- `M1Arc_LoadProfile(name) -> Bool`
- `M1Arc_DeleteProfile(name) -> Bool`

The JSON setting definitions support booleans, integers, floats and choices,
including groups and descriptions. See `src/registry.hpp`.

There is no remote update service or additional scripting engine.


## REDscript mod integration

The packaged `M1Arc.reds` declares the native functions to the REDscript
compiler. It is **not** a new scripting language or a script VM. M1 Arc
requires RED4ext for the DLL; REDscript is needed for REDscript consumers.

Register a mod once during its normal initialization (for example in the
mod's existing game-attach lifecycle), then read values directly whenever
gameplay logic needs them:

```reds
// REDscript, inside your mod's existing initialization callback:
M1Arc_RegisterJson("my.mod", "My Mod",
  "[{\"id\":\"enabled\",\"kind\":\"boolean\",\"default\":true},{\"id\":\"weight\",\"kind\":\"integer\",\"min\":50,\"max\":1000,\"default\":200}]");

// REDscript, inside an existing gameplay callback:
let active: Bool = M1Arc_GetBool("my.mod", "enabled");
let weight: Int32 = M1Arc_GetInt("my.mod", "weight");
```

When a player changes a setting in the overlay, the getter immediately reads
the updated in-memory value; the change is also saved to disk. Mods can
optionally cache `M1Arc_Revision()` to skip reapplying unchanged values.
The revision starts at zero on launch and increases on each successful
persistent change.

Native typed getters return `false`, `0`, `0.0`, or an empty string for a
missing or incorrectly typed setting. For strict error handling use
`M1Arc_GetJson` (which returns `"null"` for missing IDs) or validate IDs
during registration. Typed setters return `false` for invalid values or
failed persistence. Choice settings use `GetString`/`SetString`.

**Integration check:** The REDscript native declarations and C++ signatures
are included in the CI build package, but actual REDscript compilation and
in-game live settings still require verification on Cyberpunk 2077.

## Optional, guarded Breach Protocol input

See [Breach Protocol native bridge](docs/BREACH_INPUT.md). On an exact
supported Cyberpunk 2077 2.5 build, M1 Arc exposes a version-gated,
signature-checked selection method to the game's hacking logic
controller. It never automatically clicks or solves anything, and
fails closed on unsupported builds. The upcoming Skip Breach consumer
still performs its own gameplay UI verification.

## Future input-assisted mods

M1 Arc includes a **guarded, internal native mouse-click utility**, ported
from the working `m1core` backup. It is deliberately **not** invoked by
M1 Arc settings, not automatically enabled, and not exposed as a REDscript
API yet. This is preparation for a future **Skip Breach** mod that
calculates a valid vanilla Breach Protocol puzzle path and submits
individual selections, without pre-fulfilling daemons or faking success.

See [Internal mouse-click contract](docs/INPUT_CLICK_HELPER.md).
Windows CI exercises a simulated click backend, focus rejection, invalid
coordinates, and emergency button-release behavior. Real game UI
compatibility remains unverified; no automatic input is sent by this build.

## Official REDscript CLI validation

Windows CI now downloads the official
[\`jac3km4/redscript\` CLI v1.0.1](https://github.com/jac3km4/redscript/releases/tag/v1.0.1),
verifies its pinned SHA-256, and runs its real **parser** against
\`r6/scripts/m1arc/\` on every native build. Source parse failures fail
the workflow; the format command is run *without* \`--save\` or
\`--check\`, so it does not rewrite the files or mistake formatting for
compiler errors.

**Full semantic \`lint\` and \`compile\` need a Cyberpunk 2077 2.5
\`final.redscripts\` game bundle.** This compiled CDPR asset is not stored
in the public repo. To enable the full checks, configure BOTH GitHub
Actions repository secrets:

- \`REDSCRIPT_GAME_BUNDLE_URL\`: HTTPS URL to a privately hosted,
  unmodified game 2.5 \`final.redscripts\` file. The URL must be
  accessible to the Windows CI runner.
- \`REDSCRIPT_GAME_BUNDLE_SHA256\`: SHA-256 of the exact file.

The workflow verifies the bundle hash and then performs actual
\`redscript-cli lint -s <sources> -b <bundle> --runtime-compatible\`
and \`compile ... -o <temp-file>\`. It deletes the temporary compiled
game bundle and does not upload or redistribute it.

**If the private game bundle isn't configured, CI explicitly marks
semantic lint and compile as SKIPPED.** A green native/parse check
must never be described as REDscript semantic compilation or in-game
functionality. For a matching local game bundle, the same
\`scripts/check_redscript_cli.ps1\` command can be run from PowerShell
with environment variables set.

## Development rules

This branch is strictly focused on working RED4ext + ImGui initialization,
correct input release and native REDscript settings. Do not add unrelated
features or repeat previously verified m1core testing. The `m1core`
backup branch is read-only.
