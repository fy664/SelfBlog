// =============================================================
// 路由表：前台（公开）+ 后台（登录守卫）
//
// 路由守卫说明：
//   1. /admin 下所有页面需要登录，未登录跳 /login
//   2. 后台文章的「管理」入口仅 admin 角色可见（见 App.vue 菜单）
// =============================================================
import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  { path: '/', name: 'home', component: () => import('../views/Home.vue') },
  { path: '/article/:id', name: 'article', component: () => import('../views/ArticleDetail.vue') },
  { path: '/tags', name: 'tags', component: () => import('../views/Tags.vue') },
  { path: '/login', name: 'login', component: () => import('../views/Login.vue') },
  { path: '/register', name: 'register', component: () => import('../views/Register.vue') },
  {
    path: '/admin',
    component: () => import('../views/admin/AdminLayout.vue'),
    // 登录守卫
    beforeEnter: (to, from, next) => {
      const token = localStorage.getItem('blog_token')
      if (!token) next({ path: '/login', query: { redirect: to.fullPath } })
      else next()
    },
    children: [
      { path: '', redirect: '/admin/articles' },
      { path: 'articles', name: 'admin-articles', component: () => import('../views/admin/AdminArticles.vue') },
      { path: 'edit', name: 'admin-edit-new', component: () => import('../views/admin/AdminEdit.vue') },
      { path: 'edit/:id', name: 'admin-edit', component: () => import('../views/admin/AdminEdit.vue') },
      { path: 'comments', name: 'admin-comments', component: () => import('../views/admin/AdminComments.vue') },
      { path: 'users', name: 'admin-users', component: () => import('../views/admin/AdminUsers.vue') },
      { path: 'logs', name: 'admin-logs', component: () => import('../views/admin/AdminLogs.vue') },
      { path: 'stat', name: 'admin-stat', component: () => import('../views/admin/AdminStat.vue') }
    ]
  },
  { path: '/:pathMatch(.*)*', redirect: '/' }
]

export default createRouter({
  history: createWebHistory(),
  routes
})
