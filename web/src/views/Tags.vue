<script setup>
// =============================================================
// 标签页：展示全站标签，点击跳首页并按标签过滤
// =============================================================
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { fetchTags } from '../api'

const router = useRouter()
const tags = ref([])
const loading = ref(true)

onMounted(async () => {
  try {
    tags.value = await fetchTags()
  } finally {
    loading.value = false
  }
})

// 跳首页并带上 tag 参数（Home.vue 监听 route.query.tag）
function goTag(name) {
  router.push({ path: '/', query: { tag: name } })
}
</script>

<template>
  <div v-loading="loading">
    <h1 class="page-title">标签</h1>
    <p class="sub">按主题聚合文章，点击进入对应列表</p>

    <div class="tag-cloud">
      <div
        v-for="t in tags"
        :key="t.id"
        class="tag-item card card-hover"
        @click="goTag(t.name)"
      >
        <span class="tag-name"># {{ t.name }}</span>
      </div>
      <el-empty v-if="!loading && tags.length === 0" description="暂无标签" />
    </div>
  </div>
</template>

<style scoped>
.sub { color: var(--text-2); font-size: 14px; margin: 0 0 20px; }
.tag-cloud { display: flex; flex-wrap: wrap; gap: 12px; }
.tag-item {
  padding: 10px 18px;
  cursor: pointer;
  font-size: 15px;
}
.tag-item:hover .tag-name { color: var(--primary); }
.tag-name { font-weight: 600; }
</style>
