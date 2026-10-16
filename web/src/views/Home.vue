<script setup>
// =============================================================
// 首页：Hero 区 + 文章卡片流（两列）+ 标签快捷入口
// 数据：GET /articles?page=&size=&tag=&sort=
// =============================================================
import { ref, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { fetchArticles, fetchTags } from '../api'
import ArticleCard from '../components/ArticleCard.vue'

const route = useRoute()
const items = ref([])
const total = ref(0)
const loading = ref(false)
const page = ref(1)
const size = 12
const sort = ref('time')
const tags = ref([])
const activeTag = ref('')

async function load() {
  loading.value = true
  try {
    const data = await fetchArticles({
      page: page.value,
      size,
      tag: activeTag.value || undefined,
      sort: sort.value
    })
    items.value = data.items
    total.value = data.total
  } finally {
    loading.value = false
  }
}

async function loadTags() {
  try { tags.value = await fetchTags() } catch (e) { /* 标签加载失败不阻塞 */ }
}

// 切换标签：回到第一页重新查
function pickTag(name) {
  activeTag.value = activeTag.value === name ? '' : name
  page.value = 1
  load()
}

onMounted(() => { load(); loadTags() })

// 排序变化时刷新
watch(sort, () => { page.value = 1; load() })

// 顶部标签链接（从标签页跳回时带 tag 参数）
watch(() => route.query.tag, (t) => {
  if (t) { activeTag.value = t; page.value = 1; load() }
})
</script>

<template>
  <div>
    <!-- Hero -->
    <section class="hero">
      <h1 class="hero-title">记录 · 思考 · 成长</h1>
      <p class="hero-sub">C++ 全栈学习笔记与项目实践 —— SelfBlog 个人博客系统</p>
      <div class="hero-tags">
        <el-tag
          v-for="t in tags.slice(0, 8)"
          :key="t.id"
          :type="activeTag === t.name ? 'success' : 'info'"
          effect="plain"
          class="hero-tag"
          @click="pickTag(t.name)"
        >{{ t.name }}</el-tag>
      </div>
    </section>

    <!-- 文章流 -->
    <div class="feed-head">
      <span v-if="activeTag" class="feed-hint">
        标签：<b>{{ activeTag }}</b>（<a @click="pickTag(activeTag)" class="clear">清除</a>）
      </span>
      <span v-else class="feed-hint">共 {{ total }} 篇</span>
      <el-radio-group v-model="sort" size="small">
        <el-radio-button value="time">最新</el-radio-button>
        <el-radio-button value="views">热门</el-radio-button>
      </el-radio-group>
    </div>

    <div v-loading="loading" class="grid">
      <ArticleCard v-for="a in items" :key="a.id" :article="a" />
      <el-empty v-if="!loading && items.length === 0" description="还没有文章" />
    </div>

    <div class="pager">
      <el-pagination
        v-if="total > size"
        v-model:current-page="page"
        :page-size="size"
        :total="total"
        layout="prev, pager, next"
        background
        @current-change="load"
      />
    </div>
  </div>
</template>

<style scoped>
.hero {
  background: linear-gradient(135deg, #0f766e 0%, #134e4a 100%);
  border-radius: 16px;
  padding: 40px 32px;
  color: #fff;
  margin-bottom: 24px;
}
.hero-title { margin: 0 0 10px; font-size: 30px; font-weight: 800; }
.hero-sub { margin: 0 0 18px; opacity: .85; font-size: 15px; }
.hero-tags { display: flex; gap: 8px; flex-wrap: wrap; }
.hero-tag { cursor: pointer; }

.feed-head {
  display: flex; align-items: center; justify-content: space-between;
  margin-bottom: 14px;
}
.feed-hint { color: var(--text-2); font-size: 14px; }
.clear { color: var(--primary); cursor: pointer; }

.grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
  min-height: 200px;
}
.pager { display: flex; justify-content: center; margin-top: 24px; }

@media (max-width: 640px) {
  .hero { padding: 28px 20px; }
  .hero-title { font-size: 24px; }
  .grid { grid-template-columns: 1fr; }
}
</style>
