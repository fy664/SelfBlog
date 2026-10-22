# 全栈个人博客系统（Web + 桌面 exe）

>  C++ 后端 · MySQL 8 · 多 UI 前端 · Electron 桌面端 · 阿里云部署


---

## 一、项目定位

面向个人技术写作与求职背书的全栈工程作品，双形态交付：

| 形态 | 说明 | 状态 |
| --- | --- | --- |
| 形态 A：在线网站（Web） | 前后端分离 SPA，公网可访问 | 后端开发中 |
| 形态 B：桌面应用（exe） | Electron 打包 Windows 免安装 exe，复用 Web 前端 | 规划中 |

## 二、技术栈

| 层次 | 选型 |
| --- | --- |
| 后端 | C++17 · cpp-httplib（单头 HTTP 框架）· 线程池（框架内置） |
| 数据库 | MySQL 8（mysql_stmt 预编译防注入 + 连接池） |
| 鉴权 | JWT（HS256，HMAC-SHA256 自实现签名/验签） |
| JSON | nlohmann/json（单头文件） |
| 前端 | Vue3 + Element Plus / React + Ant Design（待定） |
| 桌面端 | Electron（规划中） |
| 部署 | 阿里云 ECS + Nginx + systemd |

## 三、目录结构

```
个人博客/
├── 全栈个人博客系统项目书.md   # 立项文档（需求/架构/计划/验收）
├── sql/
│   └── schema.sql              # MySQL 8 建库建表脚本（5 表）
├── seed_demo.py                # 演示数据播种（登录 admin → 发布示例文章）
├── server/                     # C++ 后端
│   ├── CMakeLists.txt          # 构建配置（自动探测 MySQL 开发库）
│   ├── build.bat               # Windows 一键构建（MSVC）
│   ├── config.json             # 服务配置（存储/端口/JWT/管理员）
│   ├── smoke_test.py           # 接口冒烟测试（18 项全链路）
│   ├── third_party/            # 单头文件依赖（httplib.h / json.hpp）
│   └── src/
│       ├── main.cpp            # 入口：配置→自检→仓储→路由→监听
│       ├── config.h            # 配置加载
│       ├── handlers.h/.cpp     # HTTP 路由与业务逻辑
│       ├── util/               # SHA-256 / Base64 / JWT / 时间与密码
│       └── store/              # Repository 抽象 + JsonStore + MySqlStore
├── web/                        # 前端（Vue3 + Vite + Element Plus）
│   ├── package.json
│   ├── vite.config.js          # base=./ 兼容 file://；/api 代理到 8080
│   ├── .env.production         # 生产 API 地址（Electron/部署用）
│   └── src/
│       ├── main.js / App.vue   # 入口与整体布局（顶部导航 + 主题切换）
│       ├── router/             # 路由（前台 + /admin 登录守卫）
│       ├── api/                # axios 封装 + 全部接口函数
│       ├── store/              # Pinia 用户状态 + 主题状态（theme.js）
│       ├── utils/md.js         # Markdown 渲染（marked + hljs + DOMPurify）
│       ├── styles/themes.css   # 三套 UI：晨光/纸感/暗夜（CSS 变量换肤）
│       ├── components/         # ArticleCard / MarkdownView
│       └── views/              # 首页 / 详情 / 标签 / 登录注册 / 后台管理
└── desktop/                    # Electron 桌面壳（加载 web/dist）
    ├── main.js                 # BrowserWindow 加载生产构建
    └── package.json            # electron + electron-builder（npm run dist 打 exe）
```

## 四、本地快速开始（Windows）

前置：Visual Studio（含 C++ 工具链）、CMake、Git。

```bat
cd server
build.bat            :: 产物 build\Release\blog_server.exe
```

运行（当前目录任意，config 自动向上查找）：

```bat
cd server
build\Release\blog_server.exe
```

默认 `store=mysql`（连接 127.0.0.1:3307 的本地 MySQL 8，见下节）；想零依赖跑通
就把 config 里 `store` 改为 `"json"`（数据落在 `server/data/`）。启动时若开启
`admin.enabled`，首次运行自动创建管理员账号（账号 admin，密码来自本地
`server/config.json`，仓库不包含真实密码）。

## 五、接入 MySQL 8（本机已完成）

本机现状（2026-10 已落地并验证）：MySQL **8.0.46** 安装在 `D:\MySQL8`，
数据目录 `D:\MySQL8\data`，运行端口 **3307**（避开旧版 MySQL 5.0 服务的 3306，两者互不影响）。
账号与密码见本地 `server/config.json`（仓库已忽略该文件，避免泄露真实凭据），库 `blog`，5 张表已建好。

**重启电脑后如何再次启动 MySQL 8**（手动前台启动，开一个终端挂着即可）：

```bat
D:\MySQL8\bin\mysqld --basedir=D:\MySQL8 --datadir=D:\MySQL8\data --port=3307 --console
```

**从零初始化（换机器/重装时用）**：

```bat
:: 1. 初始化数据目录（首次，root 无密码）
D:\MySQL8\bin\mysqld --initialize-insecure --basedir=D:\MySQL8 --datadir=D:\MySQL8\data

:: 2. 启动（或注册为 Windows 服务）
D:\MySQL8\bin\mysqld --basedir=D:\MySQL8 --datadir=D:\MySQL8\data --port=3307 --console

:: 3. 导入建表脚本 + 创建账号（本机工具 setup_mysql.py 一键完成，不入仓库）
python server\setup_mysql.py
```

`server/config.json` 为 MySQL 模式配置（`store=mysql`、端口 3307 等；真实密码仅存本地，
**该文件已被 .gitignore 忽略，不会进入仓库**）。

重新构建（CMake 已探测到 `D:\MySQL8\include` 与 `lib\libmysql.lib`，启用 MySqlStore，
并把 `libmysql.dll` / `libssl-3-x64.dll` / `libcrypto-3-x64.dll` 拷贝到产物目录）：

```bat
cd server && build.bat
```

**服务器（Ubuntu）接入**：

```bash
sudo apt install -y mysql-server libmysqlclient-dev
sudo mysql < sql/schema.sql
cmake -S . -B build && cmake --build build -j   # 自动探测 /usr/include/mysql 启用 MySQL
```

## 六、前端快速开始（Vue3 + Element Plus）

```bat
cd web
npm install        :: 首次
npm run dev        :: http://127.0.0.1:5173 （/api 自动代理到后端 8080）
```

访问 `http://127.0.0.1:5173`：
- 首页文章流 / 文章详情（Markdown 渲染）/ 标签页 均为公开访问
- `/login` 登录（管理员账号 admin，密码见本地 `server/config.json`）
- `/admin` 后台管理：文章 CRUD（含草稿）、评论查看、数据看板

**六套 UI（多主题）**：导航栏「主题」下拉一键切换，选择自动持久化 ——
| 主题 | 风格 | 参考开源作品 |
| --- | --- | --- |
| 晨光（默认） | 卡片化亮色，青绿主色 | hexo-theme-butterfly |
| 纸感 | 米白极简 + 衬线标题 | hugo-PaperMod |
| 暗夜 | 深色柔和文字 + Element 暗色组件 | Nightfall |
| 终端 | 等宽字体 + 荧光绿 + 直角 | M-Terminal / Hacker Blog |
| 杂志 | 报纸米黄 + 衬线大标题 + 深红点缀 | Hipaper |
| 星空 | 深蓝紫 + 玻璃拟态卡片 | Astro Starread / Fuwari |

主题系统见 `web/src/store/theme.js` + `web/src/styles/themes.css`（CSS 变量换肤，
Element Plus 主色联动）。响应式已覆盖移动/平板/桌面断点。

演示数据（可选，本机工具不入仓库）：

```bat
python seed_demo.py     :: 用 admin 登录后端并发布一篇示例文章
```

生产构建：`npm run build`（base=./，兼容 file:// 与 Electron），产物在 `web/dist`。

## 六·5、桌面端（Electron 本地应用）

```bat
cd desktop
npm install        :: 首次（国内镜像：ELECTRON_MIRROR=https://npmmirror.com/mirrors/electron/）
npm start          :: 以开发方式启动桌面应用（加载 web/dist）
npm run dist       :: 打包 Windows 安装程序（NSIS，输出 desktop/release/SelfBlog Setup *.exe）
```

桌面端复用同一套前端（含三套主题），API 指向本机后端
（`web/.env.production` 中 VITE_API_BASE=http://127.0.0.1:8080/api）。
前提：后端 blog_server 与 MySQL 已按上文启动。

## 七、API 一览

统一响应：`{ "code": 0, "msg": "ok", "data": ... }`，鉴权头 `Authorization: Bearer <token>`。

| 方法 | 路径 | 说明 | 鉴权 |
| --- | --- | --- | --- |
| GET | /api/health | 健康检查 | 否 |
| POST | /api/auth/register | 注册 `{username,password}` | 否 |
| POST | /api/auth/login | 登录，返回 JWT | 否 |
| GET | /api/articles?page=&size=&tag=&sort=time\|views&status=published\|all | 文章列表 | status=all 需登录 |
| GET | /api/articles/{id} | 文章详情（浏览 +1） | 否 |
| POST | /api/articles | 新建（草稿/发布 + 标签） | 是 |
| PUT | /api/articles/{id} | 编辑（作者/管理员） | 是 |
| DELETE | /api/articles/{id} | 删除（作者/管理员） | 是 |
| GET | /api/tags | 标签列表 | 否 |
| GET | /api/comments?article_id= | 文章评论 | 否 |
| POST | /api/comments | 发布评论 | 是 |
| GET | /api/stat/overview | 数据看板（文章/浏览/评论数） | 是 |

## 八、技术要点

1. **MySQL 连接池**：固定大小 + 互斥锁/条件变量 + 探活重建 + RAII 租约，避免每请求建连。
2. **SQL 注入防护**：全部写操作走 `mysql_stmt_*` 预编译绑定，不拼接 SQL。
3. **JWT 自实现**：HMAC-SHA256 签名 + 常量时间比较验签 + exp 过期校验；`util/` 下无第三方依赖。
4. **Repository 模式**：上层业务与存储解耦，JsonStore/MySqlStore 一键切换，利于本地开发与生产一致。
5. **SHA-256 自实现**：含 FIPS 官方测试向量自检，启动时验证实现正确性。

## 九、开发路线

- [x] 第 1 周：项目书、DB 建模、API 设计
- [x] 后端：HTTP 框架、仓储、JWT、文章/标签/评论核心接口（冒烟 18/18 通过）
- [x] 前端：Vue3 + Element Plus，前台（列表/详情/标签）+ 后台（文章/评论/看板）已联调
- [ ] 性能与安全：Redis 缓存/限流、XSS 深度防护、浏览计数落库优化
- [ ] 桌面端：Electron 壳、本地草稿、一键发布
- [ ] 测试修复与上线：阿里云部署、README、简历描述

## 十、安全说明

- 密码散列当前为 `盐$SHA256hex`（教学演示）；生产建议换 bcrypt/argon2。
- JWT 密钥放 config.json；生产应改环境变量/密钥管理。
- 本地 MySQL 8 与旧 MySQL 5.0（3306）端口隔离，互不影响。
