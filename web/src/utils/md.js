// =============================================================
// Markdown 渲染：marked 负责解析，highlight.js 负责代码高亮，
// DOMPurify 负责防 XSS（求职点：前端安全）
//
// 注意：marked v15 移除了内置 highlight 选项，改用 renderer
// 覆盖 code 节点的方式接入 hljs。
// =============================================================
import { marked } from 'marked'
import hljs from 'highlight.js'
import DOMPurify from 'dompurify'

marked.use({
  renderer: {
    // 代码块：带语言则高亮，否则自动识别
    code({ text, lang }) {
      const language = (lang || '').split(/\s+/)[0]
      let html
      if (language && hljs.getLanguage(language)) {
        html = hljs.highlight(text, { language, ignoreIllegals: true }).value
      } else {
        html = hljs.highlightAuto(text).value
      }
      return `<pre><code class="hljs language-${language || 'plaintext'}">${html}</code></pre>`
    }
  }
})

// 输入 Markdown 文本，输出净化后的 HTML
export function renderMarkdown(md) {
  const raw = marked.parse(md || '')
  return DOMPurify.sanitize(raw)
}
