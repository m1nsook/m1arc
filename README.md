# M1 Arc documentation

Built with [NextGuide](https://github.com/m1nsook/NextGuide) in Developer Mode. This branch contains the documentation website only, not the native mod.

## Local development

Requires Node.js 22, Bun 1.4.4 and tar (included in current Windows 11 and Linux).

```sh
bun run setup
bun run dev
````

Open http://localhost:3000/m1arc/ (or the port printed by Next.js). The setup command builds NextGuide from the pinned source commit and installs dependencies; it does not publish anything. The first setup requires network access. Run `bun run check` for API and static-export link validation. Subsequent edits to content only need `bun run dev` or `bun run build`.

Edit `content/docs/*.mdx` and `_meta.json`. Site identity is in `nextguide.config.json`. Production Pages deployment uses the existing /m1arc/ URL. The 1.1.0 page describes unreleased features; do not publish it until approved.
