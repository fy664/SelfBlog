// =============================================================
// 主题状态（Pinia）：六套 UI 一键切换
// 晨光/纸感/暗夜 + 终端/杂志/星空（参考 GitHub 高星开源作品）
// 持久化到 localStorage；应用方式：
//   document.documentElement.dataset.theme = 主题名
//   深色主题额外给 <html> 加 dark class（驱动 Element Plus 深色组件）
// =============================================================
import { defineStore } from 'pinia'

// 六套主题（key / 名称 / 色块 / 是否深色）
export const THEMES = [
  { key: 'dawn',     label: '晨光', dot: '#0f766e', dark: false },
  { key: 'paper',    label: '纸感', dot: '#78716c', dark: false },
  { key: 'night',    label: '暗夜', dot: '#2dd4bf', dark: true },
  { key: 'terminal', label: '终端', dot: '#7cfc00', dark: true },
  { key: 'magazine', label: '杂志', dot: '#b3261e', dark: false },
  { key: 'starread', label: '星空', dot: '#a78bfa', dark: true }
]

export const useThemeStore = defineStore('theme', {
  state: () => ({
    theme: localStorage.getItem('blog_theme') || 'dawn'
  }),
  actions: {
    // 切换主题并应用到 <html>
    setTheme(key) {
      const def = THEMES.find((t) => t.key === key)
      if (!def) key = 'dawn'
      this.theme = key
      localStorage.setItem('blog_theme', key)
      const el = document.documentElement
      el.dataset.theme = key
      // Element Plus 暗色模式：仅深色主题启用 html.dark
      if (def && def.dark) el.classList.add('dark')
      else el.classList.remove('dark')
    },
    // 启动时恢复持久化主题
    init() {
      this.setTheme(this.theme)
    }
  }
})
