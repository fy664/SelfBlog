<script setup>
// =============================================================
// 注册页：用户名 3-32 位（字母/数字/下划线），密码 6-64 位
// 校验规则与后端 validate_credential 保持一致
// =============================================================
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { register } from '../api'

const router = useRouter()
const form = ref({ username: '', password: '', confirm: '' })
const loading = ref(false)

async function submit() {
  const { username, password, confirm } = form.value
  if (!username || !password || !confirm) { ElMessage.warning('请填写完整信息'); return }
  if (!/^[A-Za-z0-9_]{3,32}$/.test(username)) {
    ElMessage.warning('用户名须为 3-32 位字母/数字/下划线')
    return
  }
  if (password.length < 6 || password.length > 64) {
    ElMessage.warning('密码长度须为 6-64 字符')
    return
  }
  if (password !== confirm) { ElMessage.warning('两次输入的密码不一致'); return }
  loading.value = true
  try {
    await register(username, password)
    ElMessage.success('注册成功，请登录')
    router.push('/login')
  } catch (e) {
    // 后端错误（如用户名已存在）已由拦截器弹出
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <div class="auth-wrap">
    <div class="auth-card card">
      <h1 class="auth-title">注册账号</h1>
      <el-form label-position="top">
        <el-form-item label="用户名">
          <el-input v-model="form.username" placeholder="3-32 位字母/数字/下划线" size="large" />
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="form.password" type="password" placeholder="6-64 位" size="large" show-password />
        </el-form-item>
        <el-form-item label="确认密码">
          <el-input v-model="form.confirm" type="password" placeholder="再次输入密码" size="large" show-password @keyup.enter="submit" />
        </el-form-item>
        <el-button type="primary" size="large" class="submit" :loading="loading" @click="submit">
          注册
        </el-button>
      </el-form>
      <p class="tip">
        已有账号？
        <router-link to="/login" class="link">直接登录</router-link>
      </p>
    </div>
  </div>
</template>

<style scoped>
.auth-wrap {
  min-height: 60vh;
  display: flex; align-items: center; justify-content: center;
}
.auth-card { width: 380px; max-width: 100%; padding: 36px 32px; }
.auth-title { margin: 0 0 24px; font-size: 22px; text-align: center; }
.submit { width: 100%; }
.tip { text-align: center; margin: 16px 0 0; font-size: 14px; color: var(--text-2); }
.link { color: var(--primary); font-weight: 600; }
</style>
