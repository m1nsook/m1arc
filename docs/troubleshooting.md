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
Review the RED4ext or M1 Arc logs for rejection reasons.

## I see the M1 ARC UI Test mod

This is a **development-only fixture**. It appears only if
`red4ext/plugins/m1arc/dev-preview.enabled` exists. Delete that
marker and restart the game. GitHub Release ZIPs omit it.

## My settings disappeared after an update

The ordinary release ZIP does not contain or overwrite
`red4ext/plugins/m1arc/settings.json`. Confirm you installed to the
same Cyberpunk game directory and haven't deleted your stored file.

## How should I report an issue?

Open [GitHub Issues](https://github.com/m1nsook/m1arc/issues) with the
Cyberpunk version, M1 Arc release tag, RED4ext version, exact symptom
and relevant log messages. Avoid posting private game data.
