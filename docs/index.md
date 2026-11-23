---
layout: home

hero:
  name: "m1script"
  text: "Scripting APIs for Cyberpunk 2077"
  tagline: LuaJIT, native mod settings and lifecycle events, hosted by the standalone m1core RED4ext DLL.
  actions:
    - theme: brand
      text: Start scripting
      link: /quickstart
    - theme: alt
      text: API reference
      link: /api-reference

features:
  - title: Standalone runtime
    details: Scripts run inside m1core.dll; no separate CET installation.
  - title: LuaJIT
    details: Write .m1s or compatible .lua source, or distribute genuine .m1c bytecode.
  - title: Native settings
    details: Checkboxes, numbers, choices, persisted values and saved profiles.
  - title: Lifecycle and keybinds
    details: Use game events and mod-scoped keyboard bindings.
---

::: warning Developer preview
m1script documents the actual m1core development APIs. It is not an additional DLL or a newly invented syntax; some edge cases still require in-game verification.
:::

