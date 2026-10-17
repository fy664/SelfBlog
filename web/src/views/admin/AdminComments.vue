<script setup>
// =============================================================
// 评论管理：按角色展示评论，可删除
//   admin：遍历最近 5 篇已发布文章，汇总其下评论（全站视角）
//   普通用户：只展示"自己发表的评论"（跨文章，user_id 过滤，含文章标题）
// 删除按钮仅对「自己的评论」或 admin 显示（后端同样校验）
// =============================================================
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { fetchArticles, fetchComments, fetchCommentsByUser, deleteComment } from '../../api'
import { useUserStore } from '../../store/user'

const router = useRouter()
const store = useUserStore()
const list = ref([])        // [{id, article_id, article_title, username, content, created_at, user_id}]
const loading = ref(true)
const myArticleIds = ref(new Set())  // 普通用户：自己文章的 id 集合（归属者可删文章下评论）

async function load() {
  loading.value = true
  try {
    if (store.isAdmin) {
      // admin：看全站（最近 5 篇已发布文章下的评论）
      const data = await fetchArticles({ page: 1, size: 5, status: 'published', sort: 'time' })
      const rows = []
      for (const a of data.items) {
        const comments = await fetchComments(a.id)
        for (const c of comments) {
          rows.push({
            id: c.id,
            article_id: a.id,
            article_title: a.title,
            username: c.username,
            content: c.content,
            created_at: c.created_at,
            user_id: c.user_id
          })
        }
      }
      rows.sort((x, y) => (x.created_at < y.created_at ? 1 : -1))
      list.value = rows
    } else {
      // 普通用户：自己发表的评论 ∪ 自己文章下的评论（去重合并，均带文章标题）
      const seen = new Set()
      const rows = []
      const push = (c, title) => {
        if (!seen.has(c.id)) {
          seen.add(c.id)
          rows.push({
            id: c.id,
            article_id: c.article_id,
            article_title: title || `文章 #${c.article_id}`,
            username: c.username,
            content: c.content,
            created_at: c.created_at,
            user_id: c.user_id
          })
        }
      }
      // ① 自己发表的全部评论（跨文章，接口按 user_id 过滤并带标题）
      const mine = await fetchCommentsByUser(store.user.id)
      for (const c of mine) push(c, c.article_title)
      // ② 自己文章下的全部评论（status=all 后端按作者过滤返回本人文章，含草稿）
      const data = await fetchArticles({ page: 1, size: 50, status: 'all', sort: 'time' })
      myArticleIds.value = new Set(data.items.map((a) => a.id))
      for (const a of data.items) {
        const comments = await fetchComments(a.id)
        for (const c of comments) push(c, a.title)
      }
      rows.sort((x, y) => (x.created_at < y.created_at ? 1 : -1))
      list.value = rows
    }
  } catch (e) {
    // 已由拦截器提示
  } finally {
    loading.value = false
  }
}

onMounted(load)

// 能否删除：自己的评论、管理员，或该评论所在文章的作者（归属者，后端同样校验）
function canDelete(c) {
  if (!store.user) return false
  if (store.isAdmin || store.user.id === c.user_id) return true
  return myArticleIds.value.has(c.article_id)
}

// 删除评论
async function removeComment(c) {
  try {
    await deleteComment(c.id)
    ElMessage.success('评论已删除')
    list.value = list.value.filter((x) => x.id !== c.id)
  } catch (e) {
    // 失败提示已由拦截器处理
  }
}

// 跳转到对应文章：SPA 内部跳转（不整页刷新，深层 URL 也不会触发资源解析错误）
function goArticle(id) {
  router.push(`/article/${id}`)
}
</script>

<template>
  <div class="panel card" v-loading="loading">
    <div class="panel-head">
      <h2 class="panel-title">评论管理</h2>
      <span class="hint">{{ store.isAdmin ? '展示最近 5 篇文章下的评论' : '展示你发表的评论和你文章下的评论' }}</span>
    </div>

    <div v-if="list.length === 0" class="empty">
      <el-empty description="暂无评论" :image-size="80" />
    </div>

    <div v-else class="comment-list">
      <div v-for="c in list" :key="c.id" class="row card">
        <div class="row-head">
          <b>{{ c.username }}</b>
          <span class="row-time">{{ c.created_at?.slice(0, 16).replace('T', ' ') }}</span>
        </div>
        <p class="row-body">{{ c.content }}</p>
        <div class="row-foot">
          <span class="row-article" @click="goArticle(c.article_id)">
            评论于《{{ c.article_title }}》
          </span>
          <el-button
            v-if="canDelete(c)"
            link
            type="danger"
            size="small"
            @click="removeComment(c)"
          >删除</el-button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.panel { padding: 20px; }
.panel-head {
  display: flex; justify-content: space-between; align-items: center;
  margin-bottom: 16px;
}
.panel-title { margin: 0; font-size: 18px; }
.hint { font-size: 12px; color: var(--text-2); }
.comment-list { display: flex; flex-direction: column; gap: 10px; }
.row { padding: 14px 16px; }
.row-head { display: flex; justify-content: space-between; margin-bottom: 6px; }
.row-time { font-size: 12px; color: var(--text-2); }
.row-body { margin: 0 0 8px; font-size: 14px; line-height: 1.7; }
.row-foot { border-top: 1px dashed var(--border); padding-top: 8px; display: flex; justify-content: space-between; align-items: center; }
.row-article { font-size: 12px; color: var(--primary); cursor: pointer; }
.empty { padding: 40px 0; }
</style>
