import { defineConfig } from 'vitepress'

export default defineConfig({
  srcDir: 'docs',
  base: '/m1arc/',
  title: 'M1 Arc',
  description: 'Native RED4ext settings framework for Cyberpunk 2077',
  cleanUrls: true,
  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Install', link: '/quickstart' },
      { text: 'Mod manifests', link: '/mod-manifest' },
      { text: 'Native API', link: '/native-api' },
      { text: 'Nexus Mods', link: 'https://www.nexusmods.com/cyberpunk2077/mods/39311' },
      { text: 'GitHub', link: 'https://github.com/m1nsook/m1arc' }
    ],
    sidebar: [
      {
        text: 'For players',
        items: [
          { text: 'Install M1 Arc', link: '/quickstart' },
          { text: 'Settings and profiles', link: '/settings' },
          { text: 'Shortcut and keybinds', link: '/keybinds' },
          { text: 'Troubleshooting', link: '/troubleshooting' }
        ]
      },
      {
        text: 'For mod authors',
        items: [
          { text: 'mod.json and settings.json', link: '/mod-manifest' },
          { text: 'REDscript native API', link: '/native-api' },
          { text: 'How M1 Arc works', link: '/architecture' }
        ]
      }
    ],
    socialLinks: [
      { icon: 'github', link: 'https://github.com/m1nsook/m1arc' }
    ],
    footer: {
      message: 'M1 Arc by 민숙 MISO @m1nsook. Community documentation; not affiliated with CD PROJEKT RED.'
    }
  }
})
