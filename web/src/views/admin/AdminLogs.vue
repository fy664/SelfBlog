<script setup>
// =============================================================
// 登录记录：GET /api/admin/login-logs（仅管理员）
// 展示所有登录/注册尝试：时间、用户名、IP、设备、成功与否
// =============================================================
import { ref, onMounted } from 'vue'
import { fetchLoginLogs } from '../../api'

const logs = ref([])
const loading = ref(true)

onMounted(async () => {
  try {
    logs.value = (await fetchLoginLogs(100)).items || []
  } catch {
    logs.value = []
  } finally {
    loading.value = false
  }
})

function fmtUa(ua) {
  if (!ua) return '-'
  // 精简 User-Agent：只取浏览器标识段
  if (ua.length <= 70) return ua
  return ua.slice(0, 70) + '…'
}
</script>

<template>
  <div class="panel" v-loading="loading">
    <div class="head">
      <h3>登录记录</h3>
      <span class="hint">最近 {{ logs.length }} 条 · 含成功与失败的登录/注册尝试</span>
    </div>

    <div class="card table-wrap">
      <table class="tbl">
        <thead>
          <tr>
            <th>时间</th>
            <th>用户名</th>
            <th>IP</th>
            <th>设备 / 浏览器</th>
            <th>结果</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="l in logs" :key="l.id">
            <td class="mono">{{ l.created_at }}</td>
            <td>{{ l.username }}</td>
            <td class="mono">{{ l.ip || '-' }}</td>
            <td class="ua">{{ fmtUa(l.user_agent) }}</td>
            <td>
              <span class="tag" :class="l.success ? 'ok' : 'fail'">
                {{ l.success ? '成功' : '失败' }}
              </span>
            </td>
          </tr>
          <tr v-if="!logs.length">
            <td colspan="5" class="empty">暂无登录记录</td>
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
.tbl { width: 100%; border-collapse: collapse; font-size: 13px; min-width: 560px; }
.tbl th, .tbl td {
  padding: 10px 14px; text-align: left;
  border-bottom: 1px solid var(--border);
  white-space: nowrap;
}
.tbl th { font-size: 12px; color: var(--text-2); font-weight: 600; background: var(--code-bg); }
.tbl td.ua { max-width: 280px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.mono { font-family: 'JetBrains Mono', Consolas, monospace; font-size: 12px; }

.tag {
  display: inline-block; padding: 2px 10px; border-radius: 999px;
  font-size: 12px; font-weight: 600;
}
.tag.ok { background: rgba(16, 185, 129, .12); color: #0d9488; }
.tag.fail { background: rgba(239, 68, 68, .12); color: #dc2626; }
.empty { text-align: center; color: var(--text-2); padding: 24px; }
</style>
