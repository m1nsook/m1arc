---
layout: home

hero:
  name: M1 ARC
  text: Native RED4ext Settings
  tagline: A standalone in-game control panel for Cyberpunk 2077 mods.
  actions:
    - theme: brand
      text: Install M1 Arc
      link: /quickstart
    - theme: alt
      text: Create a mod manifest
      link: /mod-manifest

features:
  - title: Native RED4ext integration
    details: A Windows C++ DLL and REDscript declarations; no new Lua scripting engine or CET requirement.
  - title: Mod-specific settings
    details: Mod authors define data-only settings, scopes and short descriptions.
  - title: Persistent configuration
    details: User-selected settings, profiles, theme and the M1 Arc menu shortcut survive restarts.
---

## Interactive control panel preview

This is a **browser-only demonstration** of the panel's general layout and controls.
It does not connect to Cyberpunk, read installed mods or change game settings.
The native interface continues to develop independently on the
[`native` branch](https://github.com/m1nsook/m1arc/tree/native).

<M1Preview />

See the [installation guide](/quickstart), [mod manifest reference](/mod-manifest)
and [REDscript API](/native-api) for the real framework.
