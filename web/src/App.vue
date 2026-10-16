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

// 当前主题色块（下拉触发器上的小圆点）
const currentThemeDot = computed(() => {
  const t = THEMES.find((x) => x.key === themeStore.theme)
  return t ? t.dot : THEMES[0].dot
})

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
          <!-- 主题切换：下拉菜单（六套 UI：晨光/纸感/暗夜/终端/杂志/星空） -->
          <el-dropdown trigger="click" @command="switchTheme">
            <span class="theme-trigger">
              主题
              <span
                class="theme-current-dot"
                :style="{ background: currentThemeDot }"
              />
            </span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item
                  v-for="t in themes"
                  :key="t.key"
                  :command="t.key"
                  :class="{ 'theme-active': themeStore.theme === t.key }"
                >
                  <span class="theme-item">
                    <i class="theme-item-dot" :style="{ background: t.dot }" />
                    {{ t.label }}
                  </span>
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>

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
  transition: all .15s; white-space: nowrap;
}
.nav-link:hover { color: var(--text); background: var(--code-bg); }
.nav-link.router-link-active { color: var(--primary); font-weight: 600; }
.nav-user { display: flex; align-items: center; gap: 10px; }
.user-chip { cursor: pointer; font-size: 14px; display: inline-flex; align-items: center; gap: 6px; }

/* 窄屏（≤640px）：收窄导航间距与字号，保证导航不溢出 */
@media (max-width: 640px) {
  .nav-inner { gap: 10px; }
  .nav-link { padding: 5px 8px; font-size: 13px; }
  .theme-trigger { padding: 4px 8px; font-size: 13px; }
  .brand { font-size: 16px; }
}

/* 主题切换：下拉触发器 + 菜单项 */
.theme-trigger {
  display: inline-flex; align-items: center; gap: 6px;
  font-size: 14px; color: var(--text); cursor: pointer;
  padding: 5px 10px; border: 1px solid var(--border); border-radius: 8px;
  background: var(--card); user-select: none;
  outline: none;
}
.theme-trigger:hover { border-color: var(--primary); color: var(--primary); }
.theme-current-dot {
  width: 12px; height: 12px; border-radius: 50%;
  display: inline-block; flex: none;
}
.theme-item { display: inline-flex; align-items: center; gap: 8px; font-size: 14px; }
.theme-item-dot {
  width: 12px; height: 12px; border-radius: 50%;
  display: inline-block; flex: none;
}
.theme-active .theme-item { font-weight: 600; }

.main-area { min-height: calc(100vh - 140px); padding-top: 28px; padding-bottom: 40px; }

.footer {
  border-top: 1px solid var(--border);
  padding: 24px 0;
  color: var(--text-2);
  font-size: 13px;
  text-align: center;
}
</style>
