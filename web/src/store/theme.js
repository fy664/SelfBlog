// =============================================================
// 主题状态（Pinia）：晨光 / 纸感 / 暗夜 三套 UI 一键切换
// 持久化到 localStorage；应用方式：
//   document.documentElement.dataset.theme = 主题名
//   暗夜主题额外给 <html> 加 dark class（驱动 Element Plus 深色组件）
// =============================================================
import { defineStore } from 'pinia'

// 三套主题的定义（名称 / 标签 / 色块，色块用于导航栏切换器）
export const THEMES = [
  { key: 'dawn', label: '晨光', dot: '#0f766e' },
  { key: 'paper', label: '纸感', dot: '#78716c' },
  { key: 'night', label: '暗夜', dot: '#2dd4bf' }
]

export const useThemeStore = defineStore('theme', {
  state: () => ({
    theme: localStorage.getItem('blog_theme') || 'dawn'
  }),
  actions: {
    // 切换主题并应用到 <html>
    setTheme(key) {
      if (!THEMES.some((t) => t.key === key)) key = 'dawn'
      this.theme = key
      localStorage.setItem('blog_theme', key)
      const el = document.documentElement
      el.dataset.theme = key
      // Element Plus 暗色模式：仅暗夜主题启用 html.dark
      if (key === 'night') el.classList.add('dark')
      else el.classList.remove('dark')
    },
    // 启动时恢复持久化主题
    init() {
      this.setTheme(this.theme)
    }
  }
})
