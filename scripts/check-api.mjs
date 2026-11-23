import { readFileSync, existsSync } from 'node:fs'
import assert from 'node:assert/strict'

const here = (name) => new URL('../' + name, import.meta.url)
const read = (name) => readFileSync(here(name), 'utf8')
const manifest = JSON.parse(read('api/contract.json'))
const luaTypes = read('types/m1script.lua')
assert.equal(manifest.schemaVersion, 1)
assert.deepEqual(manifest.entrypoints, ['init.m1s', 'init.lua', 'init.m1c'])

function uniqueNameList(items, group) {
  assert.ok(Array.isArray(items) && items.length, 'Empty API group: ' + group)
  const names = items.map((item) => item.name)
  assert.equal(new Set(names).size, names.length, 'Duplicate API names: ' + group)
  for (const item of items) {
    assert.match(item.name, /^[A-Za-z_][A-Za-z0-9_]*$/, 'Invalid API name')
    assert.ok(Array.isArray(item.args ?? item.params), 'Missing parameters: ' + item.name)
  }
  return names
}

for (const name of uniqueNameList(manifest.events, 'events'))
  assert.ok(luaTypes.includes('"' + name + '"'), 'Missing LuaLS event ' + name)
for (const name of uniqueNameList(manifest.globals, 'globals'))
  assert.ok(luaTypes.includes('function ' + name + '('), 'Missing LuaLS global ' + name)
for (const name of uniqueNameList(manifest.luaRegistry, 'registry'))
  assert.ok(luaTypes.includes(' ' + name + ' fun('), 'Missing LuaLS registry API ' + name)
const nativeDocs = read('docs/native-api.md')
for (const name of uniqueNameList(manifest.nativeRTTI, 'nativeRTTI'))
  assert.ok(nativeDocs.includes(name), 'Missing native API doc ' + name)

for (const page of ['index','quickstart','script-formats','architecture','api-reference',
  'lifecycle','settings','keybinds','native-api','api-examples','editor-support'])
  assert.ok(existsSync(here('docs/' + page + '.md')), 'Missing documentation page ' + page)
for (const example of ['examples/hello/init.m1s','examples/settings/init.m1s']) {
  assert.ok(existsSync(here(example)), 'Missing mod example ' + example)
  assert.ok(read(example).includes('registerForEvent('), 'Invalid mod example ' + example)
}
console.log('m1script: API contract, LuaLS declarations, native docs and examples verified')
