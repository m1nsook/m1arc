import { readFileSync, existsSync } from 'node:fs'
import assert from 'node:assert/strict'

const here = (name) => new URL('../' + name, import.meta.url)
const read = (name) => readFileSync(here(name), 'utf8')
const pages = ['index', 'quickstart', 'mod-manifest', 'native-api',
  'settings', 'keybinds', 'troubleshooting', 'architecture']
for (const name of pages) {
  assert.ok(existsSync(here('docs/' + name + '.md')),
    'Missing M1 Arc documentation: ' + name)
}
const api = read('docs/native-api.md')
const names = [
  'M1Arc_Version', 'M1Arc_RegisterJson', 'M1Arc_GetJson',
  'M1Arc_GetBool', 'M1Arc_GetInt', 'M1Arc_GetFloat', 'M1Arc_GetString',
  'M1Arc_Revision', 'M1Arc_SetJson', 'M1Arc_SetBool', 'M1Arc_SetInt',
  'M1Arc_SetFloat', 'M1Arc_SetString', 'M1Arc_ResetSetting',
  'M1Arc_ResetMod', 'M1Arc_SaveProfile', 'M1Arc_LoadProfile',
  'M1Arc_DeleteProfile'
]
for (const name of names) {
  assert.ok(api.includes(name), 'Missing documented native API: ' + name)
}
const manifest = read('docs/mod-manifest.md')
const scopes = ['Gameplay','Crafting','Cheats','Weapons','Locations',
  'Appearance','Characters','Miscellaneous','UI/UX']
for (const scope of scopes) {
  assert.ok(manifest.includes(scope), 'Missing documented scope: ' + scope)
}
for (const key of ['shortDesc', 'mod.json', 'settings.json', '180 UTF-8']) {
  assert.ok(manifest.includes(key), 'Missing manifest key: ' + key)
}
const config = read('.vitepress/config.mts')
assert.ok(config.includes("base: '/m1arc/'"), 'Website base must be /m1arc/')
assert.ok(config.includes("link: '/quickstart'"), 'Missing install link')
const theme = read('.vitepress/theme/index.ts')
assert.ok(theme.includes("app.component('M1Preview', M1Preview)"),
  'M1Preview must be registered with the VitePress theme')
const landing = read('docs/index.md')
assert.ok(landing.includes('browser-only demonstration'),
  'Preview must disclose that it is not connected to the game')
assert.ok(!landing.includes('/markdown-examples'),
  'Remove default VitePress placeholder links')
console.log('M1 Arc: manifest categories, 18 native APIs and website pages validated')
