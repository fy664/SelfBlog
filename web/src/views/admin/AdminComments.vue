<script setup>
// =============================================================
// 评论管理：按角色展示评论，可删除
//   admin：看全部已发布文章下的评论
//   普通用户：只看自己文章下的评论（后端 status=all 按作者过滤）
// 删除按钮仅对「自己的评论」或 admin 显示（后端同样校验）
// =============================================================
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { fetchArticles, fetchComments, deleteComment } from '../../api'
import { useUserStore } from '../../store/user'

const router = useRouter()
const store = useUserStore()
const list = ref([])        // [{id, article_id, article_title, username, content, created_at}]
const loading = ref(true)

async function load() {
  loading.value = true
  try {
    // admin 看全部已发布文章；普通用户用 status=all（后端按 owner 过滤为本人文章）
    const status = store.isAdmin ? 'published' : 'all'
    const data = await fetchArticles({ page: 1, size: 5, status, sort: 'time' })
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
    // 按时间倒序
    rows.sort((x, y) => (x.created_at < y.created_at ? 1 : -1))
    list.value = rows
  } catch (e) {
    // 已由拦截器提示
  } finally {
    loading.value = false
  }
}

onMounted(load)

// 能否删除：自己的评论，或管理员可删任意（后端同样校验）
function canDelete(c) {
  return store.user && (store.user.id === c.user_id || store.isAdmin)
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
      <span class="hint">展示最近 5 篇文章下的评论</span>
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
