# M1 Arc documentation

Built with [NextGuide](https://github.com/m1nsook/NextGuide) in Developer Mode. This branch contains the documentation website only, not the native mod.

## Local development

Requires Node.js 22, Bun and a local checkout of the private NextGuide repository.

In GitHub Desktop, clone NextGuide beside m1arc (both folders under the same parent), and select its main branch. From the m1arc folder:

```sh
bun run setup
bun run dev
```

The setup command copies the NextGuide package source into a temporary build folder, installs its dependencies, compiles it, packs it locally and installs the documentation dependencies. It never publishes a package or changes the NextGuide checkout. It uses the locally checked out source; CI uses the pinned upstream revision. To use a checkout elsewhere, set NEXTGUIDE_SOURCE to its absolute path before running setup. For PowerShell: `$env:NEXTGUIDE_SOURCE = "C:\path\to\NextGuide"`.

Open http://localhost:3000/m1arc/. For subsequent content-only edits, run `bun run dev`. Run `bun run setup` again to rebuild after NextGuide changes, and `bun run check` for static export validation.

## CI

NextGuide is private and 0.0.1-alpha.2 is not published. For GitHub Actions, configure a repository secret NEXTGUIDE_READ_TOKEN containing a fine-grained read-only token restricted to the NextGuide repository (Contents: read). The workflow checks out the exact upstream commit into a temporary folder with credentials persistence disabled. No token belongs in source code. Without this secret the CI build reports the missing prerequisite. PR builds do not publish.

Edit `content/docs/*.mdx` and `_meta.json`. Site identity is in `nextguide.config.json`. Production Pages deployment uses the existing /m1arc/ URL. The 1.1.0 page describes unreleased features; do not publish it until approved.
