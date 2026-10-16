<script setup>
// =============================================================
// 数据看板：GET /api/stat/overview 的核心指标卡片
// =============================================================
import { ref, onMounted } from 'vue'
import { fetchStat } from '../../api'

const stat = ref({ article_count: 0, published_count: 0, view_total: 0, comment_count: 0 })
const loading = ref(true)

onMounted(async () => {
  try {
    stat.value = await fetchStat()
  } finally {
    loading.value = false
  }
})

// 卡片配置：图标 + 颜色 + 数值 + 说明
const cards = [
  { key: 'article_count', label: '文章总数', icon: '📄', color: '#0f766e' },
  { key: 'published_count', label: '已发布', icon: '🚀', color: '#1d4ed8' },
  { key: 'view_total', label: '总浏览量', icon: '👁️', color: '#b45309' },
  { key: 'comment_count', label: '评论数', icon: '💬', color: '#be185d' }
]
</script>

<template>
  <div class="panel" v-loading="loading">
    <div class="cards">
      <div v-for="c in cards" :key="c.key" class="stat-card card card-hover">
        <div class="stat-icon" :style="{ background: c.color }">{{ c.icon }}</div>
        <div class="stat-body">
          <div class="stat-value">{{ stat[c.key] }}</div>
          <div class="stat-label">{{ c.label }}</div>
        </div>
      </div>
    </div>

    <div class="tip card">
      更多统计（文章趋势、标签分布、评论排行）将在下一迭代接入，当前为 MVP 核心指标。
    </div>
  </div>
</template>

<style scoped>
.cards {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}
.stat-card { display: flex; align-items: center; gap: 16px; padding: 22px; }
.stat-icon {
  width: 52px; height: 52px; border-radius: 12px;
  display: flex; align-items: center; justify-content: center;
  font-size: 24px; color: #fff;
}
.stat-value { font-size: 28px; font-weight: 800; line-height: 1.2; }
.stat-label { font-size: 13px; color: var(--text-2); }
.tip { margin-top: 16px; padding: 14px 18px; font-size: 13px; color: var(--text-2); }

@media (max-width: 640px) {
  .cards { grid-template-columns: 1fr; }
}
</style>
