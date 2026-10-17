import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// 前端开发服务器：/api 代理到本地 C++ 后端（127.0.0.1:8080）
// base 按构建模式区分：
//   deploy（npm run build --mode deploy，部署到 web 根路径）→ 绝对路径 /，
//     保证 /article/2 等深层 URL 下资源引用不解析错（否则相对路径会变成 /article/assets/... 404 白屏）
//   其他模式（Electron / 本地）→ 相对路径 ./，兼容 file:// 协议直接加载
export default defineConfig(({ mode }) => ({
  base: mode === 'deploy' ? '/' : './',
  plugins: [vue()],
  server: {
    host: '0.0.0.0',
    port: 5173,
    // 允许任意 Host 访问（dev 联调用：Cloudflare 隧道等外网域名）
    allowedHosts: true,
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:8080',
        changeOrigin: true
      }
    }
  }
}))
