# M1 Arc documentation

Documentation built with NextGuide. The website lives in `apps/web`.

```sh
cd apps/web
bun install
bun run dev
```

The pinned NextGuide alpha package is resolved from `apps/web/vendor/nextguide-0.0.1-alpha.2.tgz`. The npm placeholder is not used. The archive must be present; CI verifies its SHA-512 against the tested lockfile before building. No sibling repository, private token or local absolute path is needed.

Edit `apps/web/content/docs` and `apps/web/nextguide.config.json`.

GitHub Pages deploys from the `web` branch using a static export at /m1arc/. The native M1 Arc framework is maintained separately.
