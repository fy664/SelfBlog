// =============================================================
// SelfBlog 桌面端入口（Electron）
// 加载 web 生产构建（web/dist），渲染进程内即可访问本机后端
// API（构建时已注入 http://127.0.0.1:8080）。
// =============================================================
const { app, BrowserWindow } = require('electron')
const path = require('path')

function createWindow() {
  const win = new BrowserWindow({
    width: 1280,
    height: 820,
    minWidth: 960,
    minHeight: 640,
    title: 'SelfBlog · 个人博客',
    autoHideMenuBar: true,
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false
    }
  })

  // 加载生产构建产物（与 web/dist 保持同步）
  const entry = path.join(__dirname, '..', 'web', 'dist', 'index.html')
  console.log('[selfblog] entry:', entry)
  win.loadFile(entry).then(
    () => console.log('[selfblog] loadFile ok'),
    (err) => console.log('[selfblog] loadFile err:', err && err.message)
  )
}

app.whenReady().then(() => {
  createWindow()
  // macOS 惯例：点击 Dock 图标无窗口时重建
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
