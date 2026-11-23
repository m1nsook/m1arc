# Languages and translations

M1 Arc uses **native JSON** files for its interface and for individual
mods. No Lua, CET dependency, or custom scripting language is needed.

## Select a language

Open M1 Arc and use the language drop-down in the control panel footer.
Choose **English**, **Polski**, or **한국어**. Your selection is saved in
\`red4ext/plugins/m1arc/settings.json\` as \`overlay.language\`, alongside
the theme and keyboard shortcut.

The setting is preserved across game restarts. Only visible text changes:
your mod values, profiles and choice IDs remain identical.

## Framework languages

The framework's own translations are located at:

~~~text
red4ext/plugins/m1arc/
  locales/
    en_US.json
    pl_PL.json
    ko_KR.json
~~~

Each file defines a native name and a string table:

~~~json
{
  "name": "Polski",
  "strings": {
    "ui.theme": "ZMIEŃ MOTYW",
    "ui.close": "ZAMKNIJ"
  }
}
~~~

Additional locale JSON files in this folder become selectable when the
game next launches. A missing or invalid entry falls back to the
built-in English text. Fonts must contain glyphs for the new language;
the release packages include Hangul coverage for Korean.

## Translate your mod

A mod author can put a language file beside the existing manifest and
settings schema:

~~~text
red4ext/plugins/m1arc/mods/example.my-mod/
  mod.json
  settings.json
  language/
    pl_PL.json
    ko_KR.json
~~~

Example \`ko_KR.json\`:

~~~json
{
  "name": "예제 모드",
  "shortDesc": "모드 설정을 변경합니다",
  "settings": {
    "enabled": {
      "label": "활성화",
      "description": "모드 기능을 켭니다",
      "group": "일반"
    },
    "mode": {
      "label": "모드",
      "choices": {
        "balanced": "균형",
        "custom": "사용자 지정"
      }
    }
  }
}
~~~

The keys in \`settings\` are your original setting IDs from
\`settings.json\`. In \`choices\`, the keys are the original option
values; only the values in the translation file are displayed.
**Never translate IDs or stored values.** REDscript continues to read
the exact original values.

Partial translations are supported: missing fields display the
mod's original name, label, description, group, or choice value.
Malformed or missing translation files do not prevent a mod from
loading. The translation file is data only and cannot execute code.

After adding or changing language files, restart Cyberpunk 2077 to
reload them. See [mod manifests](/mod-manifest) for the base schema.
