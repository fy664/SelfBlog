import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// 前端开发服务器：/api 代理到本地 C++ 后端（127.0.0.1:8080）
export default defineConfig({
  // 相对路径资源：生产构建可被 Electron 以 file:// 协议直接加载
  base: './',
  plugins: [vue()],
  server: {
    host: '0.0.0.0',
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:8080',
        changeOrigin: true
      }
    }
  }
})
