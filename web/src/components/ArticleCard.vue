<script setup>
// =============================================================
// 文章卡片：首页文章流中的单卡
// 数据来自后端列表接口（含 id/title/summary/tags/view_count/created_at/status）
// 点击跳转文章详情页
// =============================================================
import { useRouter } from 'vue-router'

const props = defineProps({ article: { type: Object, required: true } })
const router = useRouter()

// 时间格式化：YYYY-MM-DD
function fmtDate(s) {
  if (!s) return ''
  return s.slice(0, 10)
}
</script>

<template>
  <article class="card card-hover article-card" @click="router.push(`/article/${article.id}`)">
    <!-- 顶色条：取主题主色（--hero-from），随主题切换自动换色 -->
    <div class="cover">
      <span class="cover-tag">{{ article.status === 'draft' ? '草稿' : '文章' }}</span>
    </div>

    <div class="body">
      <h2 class="title">{{ article.title }}</h2>
      <p class="summary">{{ article.summary || '（暂无摘要）' }}</p>

      <div class="meta">
        <div class="tags">
          <el-tag v-for="t in article.tags.slice(0, 3)" :key="t" size="small" effect="plain">{{ t }}</el-tag>
        </div>
        <div class="info">
          <span>{{ fmtDate(article.created_at) }}</span>
          <span class="dot">·</span>
          <span>{{ article.view_count }} 次浏览</span>
        </div>
      </div>
    </div>
  </article>
</template>

<style scoped>
.article-card { overflow: hidden; cursor: pointer; display: flex; flex-direction: column; }
.cover {
  height: 56px;
  background: var(--cover, var(--hero-from, #0f766e));  /* 主题色条，随 data-theme 自动变化 */
  display: flex; align-items: center; justify-content: flex-end;
  padding: 0 12px;
}
.cover-tag {
  font-size: 11px; color: #fff; background: rgba(0,0,0,.25);
  padding: 2px 8px; border-radius: 99px;
}
.body { padding: 16px 18px 14px; }
.title {
  margin: 0 0 8px; font-size: 17px; font-weight: 700;
  display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden;
}
.summary {
  margin: 0 0 12px; font-size: 14px; color: var(--text-2);
  display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden;
}
.meta { display: flex; align-items: center; justify-content: space-between; gap: 8px; }
.tags { display: flex; gap: 4px; flex-wrap: wrap; }
.info { color: var(--text-2); font-size: 12px; white-space: nowrap; }
.dot { margin: 0 4px; }
</style>
