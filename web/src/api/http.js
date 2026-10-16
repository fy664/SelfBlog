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
  baseURL: '/api',        // 开发环境由 vite proxy 转发到 127.0.0.1:8080
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
    const msg = err.response?.data?.msg || err.message
    if (status === 401) {
      localStorage.removeItem('blog_token')
      localStorage.removeItem('blog_user')
      if (!location.pathname.startsWith('/login')) location.href = '/login'
    } else {
      ElMessage.error(msg || '网络错误')
    }
    return Promise.reject(err)
  }
)

export default http
