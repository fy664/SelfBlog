<script setup>
// =============================================================
// Markdown 渲染组件：输入 md 文本，输出净化后的 HTML
// 安全性：先 marked 解析 → DOMPurify 清洗（防 XSS）
// =============================================================
import { computed } from 'vue'
import { renderMarkdown } from '../utils/md'

const props = defineProps({ md: { type: String, default: '' } })

const html = computed(() => renderMarkdown(props.md))
</script>

<template>
  <div class="md-body" v-html="html"></div>
</template>
