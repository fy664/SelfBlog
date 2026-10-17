<script setup>
// =============================================================
// 文章详情页：标题 + meta + Markdown 正文 + 评论区
// 评论需登录；浏览计数由后端在 GET 详情时自动 +1
// =============================================================
import { ref, onMounted, computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { fetchArticle, fetchComments, createComment, deleteComment } from '../api'
import MarkdownView from '../components/MarkdownView.vue'
import { useUserStore } from '../store/user'

const route = useRoute()
const router = useRouter()
const store = useUserStore()

const article = ref(null)
const loading = ref(true)
const comments = ref([])
const commentText = ref('')
const submitting = ref(false)

const isLoggedIn = computed(() => store.isLoggedIn)

async function load() {
  loading.value = true
  try {
    article.value = await fetchArticle(route.params.id)
    await loadComments()
  } finally {
    loading.value = false
  }
}

async function loadComments() {
  comments.value = await fetchComments(route.params.id)
}

// 能否删除某条评论：自己的评论、管理员，或该文章的作者（归属者）
function canDelete(c) {
  if (!store.user) return false
  if (store.isAdmin || store.user.id === c.user_id) return true
  return article.value && article.value.user_id === store.user.id
}

// 删除评论：作者本人或管理员（后端同样校验）
async function removeComment(c) {
  try {
    await deleteComment(c.id)
    ElMessage.success('评论已删除')
    await loadComments()
  } catch (e) {
    // 失败提示已由拦截器处理
  }
}

// 发布评论：未登录先跳登录页
async function submitComment() {
  if (!isLoggedIn.value) {
    router.push({ path: '/login', query: { redirect: route.fullPath } })
    return
  }
  const text = commentText.value.trim()
  if (!text) { ElMessage.warning('评论不能为空'); return }
  submitting.value = true
  try {
    await createComment(route.params.id, text)
    commentText.value = ''
    ElMessage.success('评论成功')
    await loadComments()
  } finally {
    submitting.value = false
  }
}

onMounted(load)
</script>

<template>
  <div v-loading="loading">
    <template v-if="article">
      <!-- 文章头 -->
      <div class="head">
        <div class="tags">
          <el-tag v-for="t in article.tags" :key="t" size="small" effect="plain">{{ t }}</el-tag>
        </div>
        <h1 class="title">{{ article.title }}</h1>
        <div class="meta">
          <span>{{ article.created_at?.slice(0, 10) }}</span>
          <span class="dot">·</span>
          <span>{{ article.view_count }} 次浏览</span>
          <span v-if="article.status === 'draft'" class="draft">草稿</span>
        </div>
      </div>

      <!-- 正文 -->
      <div class="card content-card">
        <MarkdownView :md="article.content" />
      </div>

      <!-- 评论区 -->
      <section class="comments">
        <h3 class="comments-title">评论（{{ comments.length }}）</h3>

        <div class="comment-editor card">
          <el-input
            v-model="commentText"
            type="textarea"
            :rows="3"
            maxlength="2000"
            show-word-limit
            placeholder="友善评论，理性讨论…"
          />
          <div class="editor-bar">
            <el-button :loading="submitting" type="primary" size="small" @click="submitComment">
              {{ isLoggedIn ? '发布评论' : '登录后评论' }}
            </el-button>
          </div>
        </div>

        <div class="comment-list">
          <div v-for="c in comments" :key="c.id" class="comment card">
            <div class="c-head">
              <b>{{ c.username }}</b>
              <div class="c-right">
                <span class="c-time">{{ c.created_at?.slice(0, 16).replace('T', ' ') }}</span>
                <el-button
                  v-if="canDelete(c)"
                  link
                  type="danger"
                  size="small"
                  @click="removeComment(c)"
                >删除</el-button>
              </div>
            </div>
            <p class="c-body">{{ c.content }}</p>
          </div>
          <el-empty v-if="comments.length === 0" description="还没有评论，来抢沙发" :image-size="60" />
        </div>
      </section>
    </template>
  </div>
</template>

<style scoped>
.head { margin-bottom: 20px; }
.tags { margin-bottom: 10px; display: flex; gap: 6px; }
.title { margin: 0 0 10px; font-size: 30px; font-weight: 800; line-height: 1.3; }
.meta { color: var(--text-2); font-size: 13px; }
.dot { margin: 0 6px; }
.draft { color: #b45309; border: 1px solid #f59e0b; border-radius: 4px; padding: 1px 6px; font-size: 12px; margin-left: 8px; }

.content-card { padding: 32px 36px; margin-bottom: 28px; }

.comments-title { margin: 0 0 14px; font-size: 18px; }
.comment-editor { padding: 16px; margin-bottom: 16px; }
.editor-bar { display: flex; justify-content: flex-end; margin-top: 10px; }
.comment-list { display: flex; flex-direction: column; gap: 10px; }
.comment { padding: 14px 16px; }
.c-head { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; }
.c-right { display: flex; align-items: center; gap: 10px; }
.c-time { color: var(--text-2); font-size: 12px; }
.c-body { margin: 0; font-size: 14px; line-height: 1.7; }

@media (max-width: 640px) {
  .title { font-size: 24px; }
  .content-card { padding: 20px 16px; }
}
</style>
