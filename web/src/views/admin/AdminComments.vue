<script setup>
// =============================================================
// 评论管理：列出全站最新评论（含所属文章），可删除
// 说明：后端当前仅提供按文章查评论，这里对每篇文章查询会较慢，
// 故以最近发布的文章逐个拉取前 N 篇的方式汇总（MVP 展示用）
// =============================================================
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { fetchArticles, fetchComments, deleteComment } from '../../api'

const list = ref([])        // [{id, article_id, article_title, username, content, created_at}]
const loading = ref(true)

async function load() {
  loading.value = true
  try {
    // 取最近 5 篇已发布文章，拉取各自评论
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
          created_at: c.created_at
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

// 删除任意评论（管理员权限，后端校验）
async function removeComment(c) {
  try {
    await deleteComment(c.id)
    ElMessage.success('评论已删除')
    list.value = list.value.filter((x) => x.id !== c.id)
  } catch (e) {
    // 失败提示已由拦截器处理
  }
}

// 跳转到对应文章
function goArticle(id) {
  location.href = `/article/${id}`
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
          <el-button link type="danger" size="small" @click="removeComment(c)">删除</el-button>
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
