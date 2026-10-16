<script setup>
// =============================================================
// 根组件：顶部导航 + 内容出口
// 导航项：首页 / 标签 / 后台（登录后）；右侧显示登录或用户菜单
// =============================================================
import { computed } from 'vue'
import { useRouter } from 'vue-router'
import { useUserStore } from './store/user'
import { useThemeStore, THEMES } from './store/theme'

const router = useRouter()
const store = useUserStore()
const themeStore = useThemeStore()

const isLoggedIn = computed(() => store.isLoggedIn)
const themes = THEMES

// 切换主题（theme.js 负责应用 + 持久化）
function switchTheme(key) {
  themeStore.setTheme(key)
}

// 登出：清状态并回首页
function handleLogout() {
  store.logout()
  router.push('/')
}

// 跳登录页（带上当前地址，登录后可跳回）
function goLogin() {
  router.push({ path: '/login', query: { redirect: router.currentRoute.value.fullPath } })
}
</script>

<template>
  <div class="site">
    <header class="nav">
      <div class="container nav-inner">
        <router-link to="/" class="brand">
          <span class="brand-mark">S</span>
          <span class="brand-name">SelfBlog</span>
        </router-link>

        <nav class="nav-links">
          <router-link to="/" class="nav-link">首页</router-link>
          <router-link to="/tags" class="nav-link">标签</router-link>
          <router-link v-if="isLoggedIn" to="/admin" class="nav-link">后台</router-link>
        </nav>

        <div class="nav-user">
          <!-- 主题切换：三色块圆点（晨光/纸感/暗夜） -->
          <div class="theme-switch" title="切换主题">
            <span
              v-for="t in themes"
              :key="t.key"
              class="theme-dot"
              :class="{ active: themeStore.theme === t.key }"
              :style="{ background: t.dot }"
              @click="switchTheme(t.key)"
            />
          </div>

          <template v-if="isLoggedIn">
            <el-dropdown>
              <span class="user-chip">
                {{ store.user.username }}
                <el-tag v-if="store.isAdmin" size="small" type="success">管理员</el-tag>
              </span>
              <template #dropdown>
                <el-dropdown-menu>
                  <el-dropdown-item @click="router.push('/admin')">后台管理</el-dropdown-item>
                  <el-dropdown-item divided @click="handleLogout">退出登录</el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </template>
          <template v-else>
            <el-button size="small" text @click="router.push('/register')">注册</el-button>
            <el-button size="small" type="primary" @click="goLogin">登录</el-button>
          </template>
        </div>
      </div>
    </header>

    <main class="container main-area">
      <router-view />
    </main>

    <footer class="footer">
      <div class="container">
        SelfBlog · C++ / MySQL 8 全栈作品 · <a href="/admin">后台入口</a>
      </div>
    </footer>
  </div>
</template>

<style scoped>
/* 顶部导航：sticky 吸顶 + 半透明背景 */
.nav {
  position: sticky;
  top: 0;
  z-index: 100;
  background: var(--nav-bg);
  backdrop-filter: blur(8px);
  border-bottom: 1px solid var(--border);
}
.nav-inner { display: flex; align-items: center; gap: 24px; height: 60px; }
.brand { display: flex; align-items: center; gap: 8px; font-weight: 800; font-size: 18px; }
.brand-mark {
  width: 28px; height: 28px; border-radius: 8px;
  background: var(--primary); color: #fff;
  display: inline-flex; align-items: center; justify-content: center;
  font-size: 16px;
}
.nav-links { display: flex; gap: 4px; flex: 1; }
.nav-link {
  padding: 6px 12px; border-radius: 8px; font-size: 15px; color: var(--text-2);
  transition: all .15s;
}
.nav-link:hover { color: var(--text); background: var(--code-bg); }
.nav-link.router-link-active { color: var(--primary); font-weight: 600; }
.nav-user { display: flex; align-items: center; gap: 10px; }
.user-chip { cursor: pointer; font-size: 14px; display: inline-flex; align-items: center; gap: 6px; }

/* 主题切换色块 */
.theme-switch { display: flex; gap: 6px; align-items: center; }
.theme-dot {
  width: 16px; height: 16px; border-radius: 50%;
  cursor: pointer;
  border: 2px solid transparent;
  opacity: .55;
  transition: all .15s;
}
.theme-dot:hover { opacity: 1; transform: scale(1.15); }
.theme-dot.active { opacity: 1; border-color: var(--text-2); transform: scale(1.15); }

.main-area { min-height: calc(100vh - 140px); padding-top: 28px; padding-bottom: 40px; }

.footer {
  border-top: 1px solid var(--border);
  padding: 24px 0;
  color: var(--text-2);
  font-size: 13px;
  text-align: center;
}
</style>
