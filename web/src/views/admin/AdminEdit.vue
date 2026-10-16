<script setup>
// =============================================================
// 文章编辑页（新建 / 编辑共用）：
//   /admin/edit       -> 新建（空表单）
//   /admin/edit/:id   -> 编辑（加载已有文章）
// 功能：标题 / 摘要 / 标签 / 正文（Markdown，右侧实时预览）/
//       草稿或发布 二选一提交
// =============================================================
import { ref, onMounted, computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { fetchArticle, createArticle, updateArticle } from '../../api'
import MarkdownView from '../../components/MarkdownView.vue'

const route = useRoute()
const router = useRouter()

const editingId = computed(() => route.params.id || null)
const loading = ref(false)
const saving = ref(false)

// 表单状态
const form = ref({
  title: '',
  summary: '',
  content: '',
  tags: [],        // el-select multiple 的数组
  status: 'draft'
})

onMounted(async () => {
  if (editingId.value) {
    loading.value = true
    try {
      const a = await fetchArticle(editingId.value)
      form.value = {
        title: a.title,
        summary: a.summary || '',
        content: a.content,
        tags: a.tags || [],
        status: a.status
      }
    } finally {
      loading.value = false
    }
  }
})

// 提交：发布或存草稿
async function submit(asPublished) {
  const title = form.value.title.trim()
  if (!title) { ElMessage.warning('请填写标题'); return }
  if (form.value.tags.length > 10) { ElMessage.warning('标签最多 10 个'); return }

  saving.value = true
  const payload = { ...form.value, status: asPublished ? 'published' : 'draft' }
  try {
    let id
    if (editingId.value) {
      await updateArticle(editingId.value, payload)
      id = editingId.value
    } else {
      const data = await createArticle(payload)
      id = data.id
    }
    ElMessage.success(asPublished ? '已发布' : '已保存为草稿')
    router.push('/admin/articles')
  } catch (e) {
    // 错误由拦截器统一提示
  } finally {
    saving.value = false
  }
}
</script>

<template>
  <div class="editor card" v-loading="loading">
    <div class="panel-head">
      <h2 class="panel-title">{{ editingId ? '编辑文章' : '新建文章' }}</h2>
      <div class="actions">
        <el-button @click="router.push('/admin/articles')">返回列表</el-button>
        <el-button :loading="saving" @click="submit(false)">存草稿</el-button>
        <el-button type="primary" :loading="saving" @click="submit(true)">发布</el-button>
      </div>
    </div>

    <el-form label-position="top">
      <el-form-item label="标题">
        <el-input v-model="form.title" placeholder="文章标题（1-200 字）" maxlength="200" />
      </el-form-item>

      <el-form-item label="标签">
        <el-select
          v-model="form.tags"
          multiple
          filterable
          allow-create
          default-first-option
          placeholder="输入标签名后回车创建"
          style="width: 100%"
        >
          <el-option v-for="t in form.tags" :key="t" :label="t" :value="t" />
        </el-select>
      </el-form-item>

      <el-form-item label="摘要">
        <el-input v-model="form.summary" placeholder="一句话摘要（可选，最多 500 字）" maxlength="500" />
      </el-form-item>

      <el-form-item label="正文（Markdown）">
        <div class="md-split">
          <el-input
            v-model="form.content"
            type="textarea"
            :rows="18"
            placeholder="支持 Markdown 语法：标题 / 代码块 / 引用 / 表格…"
          />
          <div class="preview">
            <div class="preview-label">预览</div>
            <MarkdownView :md="form.content" />
          </div>
        </div>
      </el-form-item>
    </el-form>
  </div>
</template>

<style scoped>
.editor { padding: 20px; }
.panel-head {
  display: flex; justify-content: space-between; align-items: center;
  margin-bottom: 18px;
}
.panel-title { margin: 0; font-size: 18px; }
.md-split { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; width: 100%; }
.preview {
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 14px 16px;
  max-height: 560px;
  overflow-y: auto;
  background: #fff;
}
.preview-label { font-size: 12px; color: var(--text-2); margin-bottom: 8px; }

@media (max-width: 800px) {
  .md-split { grid-template-columns: 1fr; }
}
</style>
