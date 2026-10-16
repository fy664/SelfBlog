<script setup>
// =============================================================
// 文章管理：表格列出全部文章（含草稿，status=all 需登录）
// 操作：新建 / 编辑 / 删除（删除需二次确认）
// =============================================================
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { fetchArticles, deleteArticle } from '../../api'

const router = useRouter()
const items = ref([])
const total = ref(0)
const page = ref(1)
const size = 10
const loading = ref(false)

async function load() {
  loading.value = true
  try {
    const data = await fetchArticles({ page: page.value, size, status: 'all', sort: 'time' })
    items.value = data.items
    total.value = data.total
  } finally {
    loading.value = false
  }
}

async function remove(id) {
  try {
    await ElMessageBox.confirm('删除后不可恢复，确认删除这篇文章？', '删除确认', {
      type: 'warning',
      confirmButtonText: '删除',
      cancelButtonText: '取消'
    })
  } catch (e) { return }  // 用户取消
  await deleteArticle(id)
  ElMessage.success('已删除')
  load()
}

function fmt(s) { return s ? s.slice(0, 10) : '' }

onMounted(load)
</script>

<template>
  <div class="panel card">
    <div class="panel-head">
      <h2 class="panel-title">文章管理</h2>
      <el-button type="primary" size="small" @click="router.push('/admin/edit')">+ 新建文章</el-button>
    </div>

    <el-table :data="items" v-loading="loading" stripe>
      <el-table-column prop="id" label="ID" width="60" />
      <el-table-column prop="title" label="标题" min-width="200" show-overflow-tooltip />
      <el-table-column label="标签" width="180">
        <template #default="{ row }">
          <el-tag v-for="t in row.tags" :key="t" size="small" effect="plain" style="margin-right:4px">{{ t }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column label="状态" width="80">
        <template #default="{ row }">
          <el-tag :type="row.status === 'published' ? 'success' : 'warning'" size="small">
            {{ row.status === 'published' ? '已发布' : '草稿' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="view_count" label="浏览" width="70" />
      <el-table-column label="创建时间" width="100">
        <template #default="{ row }">{{ fmt(row.created_at) }}</template>
      </el-table-column>
      <el-table-column label="操作" width="130" fixed="right">
        <template #default="{ row }">
          <el-button link type="primary" size="small" @click="router.push(`/article/${row.id}`)">查看</el-button>
          <el-button link type="primary" size="small" @click="router.push(`/admin/edit/${row.id}`)">编辑</el-button>
          <el-button link type="danger" size="small" @click="remove(row.id)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

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
.panel { padding: 20px; }
.panel-head {
  display: flex; justify-content: space-between; align-items: center;
  margin-bottom: 16px;
}
.panel-title { margin: 0; font-size: 18px; }
.pager { display: flex; justify-content: center; margin-top: 16px; }
</style>
