import { readFileSync, existsSync } from 'node:fs'
import assert from 'node:assert/strict'

const here = (name) => new URL('../' + name, import.meta.url)
const read = (name) => readFileSync(here(name), 'utf8')
const pages = ['index', 'quickstart', 'mod-manifest', 'native-api',
  'settings', 'keybinds', 'troubleshooting', 'architecture']
for (const name of pages) {
  assert.ok(existsSync(here('content/docs/' + name + '.mdx')),
    'Missing M1 Arc documentation: ' + name)
}
const api = read('content/docs/native-api.mdx')
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
const manifest = read('content/docs/mod-manifest.mdx')
const scopes = ['Gameplay','Crafting','Cheats','Weapons','Locations',
  'Appearance','Characters','Miscellaneous','UI/UX']
for (const scope of scopes) {
  assert.ok(manifest.includes(scope), 'Missing documented scope: ' + scope)
}
for (const key of ['shortDesc', 'mod.json', 'settings.json', '180 UTF-8']) {
  assert.ok(manifest.includes(key), 'Missing manifest key: ' + key)
}
const config = read('next.config.mjs')
assert.ok(config.includes('basePath: "/m1arc"'), 'Website base must be /m1arc')
const landing = read('content/docs/index.mdx')
assert.ok(landing.includes('browser-only demonstration'))
assert.ok(landing.includes('<M1Preview />'))
assert.ok(read('content/docs/development.mdx').includes('Development only'))
assert.ok(read('content/docs/development.mdx').includes('controlsGroup'))
console.log('M1 Arc: manifest categories, native APIs and documentation pages validated')
