// =============================================================
// axios 实例：统一请求地址、超时、Token 注入、错误处理
//
// 关键约定（与后端 handlers.cpp 一致）：
//   后端统一响应 {code, msg, data}，code=0 表示成功
//   鉴权请求携带 Authorization: Bearer <token>
// =============================================================
import axios from 'axios'
import { ElMessage } from 'element-plus'

const http = axios.create({
  // 开发环境走相对路径 /api（vite proxy 转发到后端）
  // 生产构建（Electron / 部署）通过 VITE_API_BASE 注入后端绝对地址
  baseURL: import.meta.env.VITE_API_BASE || '/api',
  timeout: 10000
})

// 请求拦截：自动带 JWT
http.interceptors.request.use((config) => {
  const token = localStorage.getItem('blog_token')
  if (token) config.headers.Authorization = `Bearer ${token}`
  return config
})

// 响应拦截：统一解包 {code,msg,data}；401 清登录态并跳登录页
http.interceptors.response.use(
  (resp) => {
    const body = resp.data
    if (body && typeof body === 'object' && 'code' in body) {
      if (body.code !== 0) {
        ElMessage.error(body.msg || '请求失败')
        return Promise.reject(new Error(body.msg || 'request failed'))
      }
      return body.data
    }
    return body
  },
  (err) => {
    const status = err.response?.status
    const data = err.response?.data
    // 统一错误日志：时间戳 + 方法/地址/状态码 + 后端消息，便于排查
    const ts = new Date().toISOString()
    const url = err.config?.url || ''
    const method = (err.config?.method || '').toUpperCase()
    const msg = data?.msg || err.message || '网络错误'
    console.error(`[API ${ts}] ${method} ${url} -> ${status} | ${msg}`)

    if (status === 401) {
      localStorage.removeItem('blog_token')
      localStorage.removeItem('blog_user')
      if (!location.pathname.startsWith('/login')) location.href = '/login'
    } else if (status === 500) {
      // 后端已统一返回标准 JSON；兜底文案避免裸 500 时用户看到英文通用错误
      ElMessage.error(data?.msg || '服务器内部错误，请稍后重试或联系管理员查看日志')
    } else {
      ElMessage.error(msg || '网络错误')
    }
    return Promise.reject(err)
  }
)

export default http
