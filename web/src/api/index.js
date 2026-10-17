// =============================================================
// API 函数集：每个方法对应后端一个路由
// 返回的是后端解包后的 data（见 http.js 拦截器）
// =============================================================
import http from './http'

// ---- 鉴权 ----
export const register = (username, password) =>
  http.post('/auth/register', { username, password })

export const login = (username, password) =>
  http.post('/auth/login', { username, password })

// ---- 文章 ----
// 参数说明：
//   page/size 分页；tag 按标签过滤；sort=time|views 排序
//   status=published（默认）| all（需要登录，含草稿）
export const fetchArticles = (params) => http.get('/articles', { params })

export const fetchArticle = (id) => http.get(`/articles/${id}`)

export const createArticle = (data) => http.post('/articles', data)

export const updateArticle = (id, data) => http.put(`/articles/${id}`, data)

export const deleteArticle = (id) => http.delete(`/articles/${id}`)

// ---- 标签 ----
export const fetchTags = () => http.get('/tags')

// ---- 评论 ----
export const fetchComments = (articleId) =>
  http.get('/comments', { params: { article_id: articleId } })

export const createComment = (articleId, content) =>
  http.post('/comments', { article_id: articleId, content })

// ---- 统计（数据看板）----
export const fetchStat = () => http.get('/stat/overview')

// ---- 登录日志（后台，仅管理员）----
export const fetchLoginLogs = (limit = 50) =>
  http.get('/admin/login-logs', { params: { limit } })

// ---- 用户列表（后台，仅管理员）----
export const fetchUsers = () => http.get('/admin/users')
