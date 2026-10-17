<script setup>
// =============================================================
// 后台布局：左侧菜单 + 右侧内容区（路由守卫在 router 中统一处理）
// 菜单：文章管理 / 新建文章 / 评论管理 / 数据看板
// =============================================================
import { useRouter, useRoute } from 'vue-router'
import { useUserStore } from '../../store/user'
import { computed } from 'vue'

const router = useRouter()
const route = useRoute()
const store = useUserStore()

const menus = [
  { path: '/admin/articles', label: '文章管理', icon: '📄' },
  { path: '/admin/edit', label: '新建文章', icon: '✏️' },
  { path: '/admin/comments', label: '评论管理', icon: '💬' },
  { path: '/admin/stat', label: '数据看板', icon: '📊' }
]

// 仅管理员可见的菜单（页面本身后端也做权限拦截）
const adminMenus = [
  { path: '/admin/users', label: '用户管理', icon: '👥' },
  { path: '/admin/logs', label: '登录记录', icon: '🛡️' }
]

// 渲染用：普通用户看不到 admin 专属菜单
const visibleMenus = computed(() =>
  store.isAdmin ? [...menus, ...adminMenus] : menus
)

function isActive(p) {
  // 文章管理高亮也覆盖 /admin/edit/:id
  if (p === '/admin/articles') return route.path === p
  if (p === '/admin/edit') return route.path.startsWith('/admin/edit')
  return route.path === p
}

function goHome() { router.push('/') }
</script>

<template>
  <div class="admin">
    <aside class="side card">
      <div class="side-head">
        <b>后台管理</b>
        <span class="side-sub" @click="goHome">← 返回前台</span>
      </div>
      <div
        v-for="m in visibleMenus"
        :key="m.path"
        class="menu-item"
        :class="{ active: isActive(m.path) }"
        @click="router.push(m.path)"
      >
        <span class="mi-icon">{{ m.icon }}</span>
        {{ m.label }}
      </div>
    </aside>

    <div class="content">
      <router-view />
    </div>
  </div>
</template>

<style scoped>
.admin {
  display: flex;
  gap: 20px;
  align-items: flex-start;
}
.side {
  width: 200px;
  padding: 16px 12px;
  position: sticky;
  top: 76px;
  flex-shrink: 0;
}
.side-head {
  display: flex; justify-content: space-between; align-items: center;
  padding: 0 8px 12px;
  border-bottom: 1px solid var(--border);
  margin-bottom: 10px;
  font-size: 15px;
}
.side-sub { font-size: 12px; color: var(--text-2); cursor: pointer; }
.menu-item {
  padding: 10px 12px;
  border-radius: 8px;
  font-size: 14px;
  cursor: pointer;
  color: var(--text-2);
  display: flex; align-items: center; gap: 8px;
  transition: all .15s;
}
.menu-item:hover { background: var(--code-bg); color: var(--text); }
.menu-item.active { background: var(--accent-soft); color: var(--primary-dark); font-weight: 600; }
.mi-icon { font-size: 15px; }

.content { flex: 1; min-width: 0; }

@media (max-width: 640px) {
  .admin { flex-direction: column; }
  .side { width: 100%; position: static; display: flex; flex-wrap: wrap; gap: 6px; }
  .side-head { width: 100%; }
  .menu-item { flex: 1; justify-content: center; min-width: 90px; }
}
</style>
