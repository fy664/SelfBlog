<script setup>
// =============================================================
// 用户管理：GET /api/admin/users（仅管理员）
// 展示所有注册账号：用户名 / 角色 / 注册 IP / 注册时间
// 普通用户（author）看不到本页，且后端接口也会拒绝（403）
// =============================================================
import { ref, onMounted } from 'vue'
import { fetchUsers } from '../../api'

const users = ref([])
const loading = ref(true)

onMounted(async () => {
  try {
    users.value = (await fetchUsers()).items || []
  } catch {
    users.value = []
  } finally {
    loading.value = false
  }
})

function roleLabel(r) {
  return r === 'admin' ? '管理员' : '作者'
}
</script>

<template>
  <div class="panel" v-loading="loading">
    <div class="head">
      <h3>用户管理</h3>
      <span class="hint">共 {{ users.length }} 个账号 · 同一 IP 仅允许注册一个账号</span>
    </div>

    <div class="card table-wrap">
      <table class="tbl">
        <thead>
          <tr>
            <th>ID</th>
            <th>用户名</th>
            <th>角色</th>
            <th>注册 IP</th>
            <th>注册时间</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="u in users" :key="u.id">
            <td class="mono">{{ u.id }}</td>
            <td>{{ u.username }}</td>
            <td>
              <span class="tag" :class="u.role === 'admin' ? 'adm' : 'auth'">
                {{ roleLabel(u.role) }}
              </span>
            </td>
            <td class="mono">{{ u.register_ip || '-' }}</td>
            <td class="mono">{{ u.created_at }}</td>
          </tr>
          <tr v-if="!users.length">
            <td colspan="5" class="empty">暂无用户</td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<style scoped>
.head {
  display: flex; align-items: baseline; justify-content: space-between;
  flex-wrap: wrap; gap: 8px; margin-bottom: 14px;
}
.head h3 { font-size: 18px; margin: 0; }
.hint { font-size: 12px; color: var(--text-2); }

.table-wrap { padding: 0; overflow-x: auto; }
.tbl { width: 100%; border-collapse: collapse; font-size: 13px; min-width: 520px; }
.tbl th, .tbl td {
  padding: 10px 14px; text-align: left;
  border-bottom: 1px solid var(--border);
  white-space: nowrap;
}
.tbl th { font-size: 12px; color: var(--text-2); font-weight: 600; background: var(--code-bg); }
.mono { font-family: 'JetBrains Mono', Consolas, monospace; font-size: 12px; }

.tag {
  display: inline-block; padding: 2px 10px; border-radius: 999px;
  font-size: 12px; font-weight: 600;
}
.tag.adm { background: rgba(99, 102, 241, .12); color: #4f46e5; }
.tag.auth { background: rgba(16, 185, 129, .12); color: #0d9488; }
.empty { text-align: center; color: var(--text-2); padding: 24px; }
</style>
