// =============================================================
// 用户状态（Pinia）：token 与当前用户信息
// 持久化到 localStorage，刷新后仍保持登录态
// =============================================================
import { defineStore } from 'pinia'

export const useUserStore = defineStore('user', {
  state: () => ({
    token: localStorage.getItem('blog_token') || '',
    user: JSON.parse(localStorage.getItem('blog_user') || 'null')
  }),
  getters: {
    isLoggedIn: (s) => !!s.token,
    isAdmin: (s) => s.user?.role === 'admin'
  },
  actions: {
    // 登录成功后写入状态与本地存储
    setAuth(token, user) {
      this.token = token
      this.user = user
      localStorage.setItem('blog_token', token)
      localStorage.setItem('blog_user', JSON.stringify(user))
    },
    logout() {
      this.token = ''
      this.user = null
      localStorage.removeItem('blog_token')
      localStorage.removeItem('blog_user')
    }
  }
})
