// =============================================================
// 前端入口：挂载 Vue + Element Plus（全量引入，含中文语言包）
// 路由与 Pinia 在 main.js 统一注册，全局样式见 styles/main.css
// =============================================================
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import zhCn from 'element-plus/es/locale/lang/zh-cn'
import 'element-plus/dist/index.css'
import 'element-plus/theme-chalk/dark/css-vars.css'  // Element Plus 暗色组件变量
import 'highlight.js/styles/github.css'

import App from './App.vue'
import router from './router'
import { useThemeStore } from './store/theme'
import './styles/main.css'
import './styles/themes.css'

const app = createApp(App)
app.use(createPinia())
app.use(router)
app.use(ElementPlus, { locale: zhCn })

// 启动即恢复持久化主题（晨光/纸感/暗夜）
useThemeStore().init()

app.mount('#app')
