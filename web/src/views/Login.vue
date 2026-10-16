<script setup>
// =============================================================
// 登录页：居中卡片表单，成功后按 redirect 参数跳回原页面
// =============================================================
import { ref } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import { login } from '../api'
import { useUserStore } from '../store/user'

const router = useRouter()
const route = useRoute()
const store = useUserStore()

const form = ref({ username: '', password: '' })
const loading = ref(false)

async function submit() {
  const { username, password } = form.value
  if (!username || !password) { ElMessage.warning('请填写用户名和密码'); return }
  loading.value = true
  try {
    const data = await login(username, password)
    store.setAuth(data.token, data.user)
    ElMessage.success('登录成功')
    const redirect = typeof route.query.redirect === 'string' ? route.query.redirect : '/admin'
    router.push(redirect)
  } catch (e) {
    // 错误提示已由拦截器统一弹出
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <div class="auth-wrap">
    <div class="auth-card card">
      <h1 class="auth-title">登录 SelfBlog</h1>
      <el-form label-position="top" @submit.prevent="submit">
        <el-form-item label="用户名">
          <el-input v-model="form.username" placeholder="用户名" size="large" autocomplete="username" />
        </el-form-item>
        <el-form-item label="密码">
          <el-input
            v-model="form.password"
            type="password"
            placeholder="密码"
            size="large"
            autocomplete="current-password"
            show-password
            @keyup.enter="submit"
          />
        </el-form-item>
        <el-button type="primary" size="large" class="submit" :loading="loading" @click="submit">
          登录
        </el-button>
      </el-form>
      <p class="tip">
        还没有账号？
        <router-link to="/register" class="link">立即注册</router-link>
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
