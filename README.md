# M1 Arc documentation

Built with [NextGuide](https://github.com/m1nsook/NextGuide) in Developer Mode. This branch contains the documentation website only, not the native mod.

## Local development

Requires Node.js 22, Bun 1.4.4.

```sh
bun install
bun run dev
````

Open http://localhost:3000/m1arc/ (or the port printed by Next.js). NextGuide is pinned to the npm alpha version matching the upstream source manifest (0.0.1-alpha.2). Installation does not publish anything. The first install requires network access. Run `bun run check` for API and static-export link validation. Subsequent edits to content only need `bun run dev` or `bun run build`.

Edit `content/docs/*.mdx` and `_meta.json`. Site identity is in `nextguide.config.json`. Production Pages deployment uses the existing /m1arc/ URL. The 1.1.0 page describes unreleased features; do not publish it until approved. Commit the Bun lockfile after a successful dependency resolution.
