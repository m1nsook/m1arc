# M1 Arc documentation

Documentation built with NextGuide. The only website application is in `apps/web`.

## Development

The existing local NextGuide 0.0.1-alpha.2 archive is resolved from the sibling NextGuide repository, not a machine-specific absolute path.

```powershell
cd apps/web
bun install
bun run dev
```

Edit `apps/web/content/docs` and `apps/web/nextguide.config.json`.

GitHub Pages deploys from the `web` branch using a static export at /m1arc/. CI requires the exact NextGuide version to be published to npm. The native M1 Arc framework is maintained separately.
