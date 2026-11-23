import { defineConfig } from 'vitepress'

export default defineConfig({
  srcDir: 'docs',
  base: '/m1script/',
  title: 'm1script',
  description: 'Cyberpunk 2077 scripting API reference for the standalone m1core RED4ext framework',
  cleanUrls: true,
  themeConfig: {
    nav: [
      { text: 'Guide', link: '/quickstart' },
      { text: 'API', link: '/api-reference' },
      { text: 'GitHub', link: 'https://github.com/m1nsook/m1script' }
    ],
    sidebar: [
      { text: 'Getting started', items: [
        { text: 'Introduction', link: '/' },
        { text: 'Quick start', link: '/quickstart' },
        { text: 'Formats', link: '/script-formats' },
        { text: 'Architecture', link: '/architecture' },
        { text: 'Roadmap', link: '/roadmap' }
      ] },
      { text: 'Developer APIs', items: [
        { text: 'Complete reference', link: '/api-reference' },
        { text: 'Lifecycle', link: '/lifecycle' },
        { text: 'Settings', link: '/settings' },
        { text: 'Keybinds', link: '/keybinds' },
        { text: 'REDscript API', link: '/native-api' },
        { text: 'Examples', link: '/api-examples' },
        { text: 'Editor support', link: '/editor-support' }
      ] }
    ],
    socialLinks: [{ icon: 'github', link: 'https://github.com/m1nsook/m1script' }]
  }
})
