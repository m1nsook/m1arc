# Troubleshooting

## The game rejects `M1Arc.reds` native declarations

Check that the installed `M1Arc.reds` and `m1arc.dll` come from the
**same build**. A newer REDscript declaration with an older
DLL can cause RED4ext to reject native definitions on startup.

Install the complete game-root ZIP rather than mixing files from
different builds.

## My mod doesn't appear in the sidebar

Place `mod.json` and `settings.json` in
`red4ext/plugins/m1arc/mods/<matching-mod-id>/`. Ensure the folder name
matches the `id` and the scope is one of the nine documented names.
Restart Cyberpunk; M1 Arc reads manifests when the process starts.
In 1.0.1+, open the Logs view and use **Copy diagnostics** even if no mod is registered. The report includes startup manifest rejection reasons. If M1 Arc cannot open, review the RED4ext or M1 Arc disk logs.

## I see the M1 ARC UI Test mod

This is a **development-only fixture**. Older test builds can show it when
`red4ext/plugins/m1arc/dev-preview.enabled` exists. Normal 1.0.1 builds
do not compile the fixture and ignore the old marker. Install the ordinary
game-root ZIP, not the optional UI Test ZIP.

## My settings disappeared after an update

The ordinary release ZIP does not contain or overwrite
`red4ext/plugins/m1arc/settings.json`. Confirm you installed to the
same Cyberpunk game directory and haven't deleted your stored file.

## The update asks me to choose a new shortcut

This is intentional when upgrading from 1.0.0 to 1.0.1. Click Capture,
choose a key or Ctrl/Shift combination, then Save. The migration preserves
mod settings, profiles, theme and language. Do not delete settings.json.
After reopening M1 Arc, acknowledge the one-time update message.

## How should I report an issue?

Use the header Logs switch, select the affected mod and click **Copy diagnostics**
(1.0.1+). Review the copied report and include it with reproduction steps in
[Nexus Mods discussions](https://www.nexusmods.com/cyberpunk2077/mods/39311?tab=posts)
or [GitHub Issues](https://github.com/m1nsook/m1arc/issues).

If the game crashes before the panel opens, include the relevant original
RED4ext/REDscript logs and installed versions instead. See the full
[Logs and diagnostics](/diagnostics) guide.
