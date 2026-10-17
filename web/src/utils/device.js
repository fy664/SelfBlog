// =============================================================
// 设备 ID 工具：生成/读取一个持久化的设备标识（localStorage）
// 用途：注册限设备（同一设备只能注册一个账号）
// 说明：Web 端无硬件级指纹，采用 localStorage 持久 UUID 是
//       业界通行的"软设备锁"方案；清缓存/换浏览器可绕过，
//       但对批量刷号已形成有效门槛。
// =============================================================

const KEY = 'blog_device_id'

function genUuid() {
  // UUID v4（crypto.randomUUID 优先，降级 Math.random）
  if (window.crypto && typeof window.crypto.randomUUID === 'function') {
    return window.crypto.randomUUID()
  }
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (c) => {
    const r = (Math.random() * 16) | 0
    const v = c === 'x' ? r : (r & 0x3) | 0x8
    return v.toString(16)
  })
}

export function getDeviceId() {
  let id = ''
  try {
    id = localStorage.getItem(KEY) || ''
  } catch (e) {
    /* localStorage 不可用（隐私模式等）时返回空，后端放行 */
  }
  if (!id) {
    id = genUuid()
    try {
      localStorage.setItem(KEY, id)
    } catch (e) {
      /* 同上 */
    }
  }
  return id
}
