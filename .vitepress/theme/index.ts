import DefaultTheme from 'vitepress/theme'
import type { Theme } from 'vitepress'
import M1Preview from './components/M1Preview.vue'
import './style.css'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('M1Preview', M1Preview)
  }
} satisfies Theme
