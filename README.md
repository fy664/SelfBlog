# 全栈个人博客系统（Web + 桌面 exe）

> 求职主力项目 · C++ 后端 · MySQL 8 · 多 UI 前端 · Electron 桌面端 · 阿里云部署
> 详细立项文档见《全栈个人博客系统项目书.md》（同目录）。

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
└── server/                     # C++ 后端
    ├── CMakeLists.txt          # 构建配置（自动探测 MySQL 开发库）
    ├── build.bat               # Windows 一键构建（MSVC）
    ├── config.json             # 服务配置（存储/端口/JWT/管理员）
    ├── third_party/            # 单头文件依赖（httplib.h / json.hpp）
    └── src/
        ├── main.cpp            # 入口：配置→自检→仓储→路由→监听
        ├── config.h            # 配置加载
        ├── handlers.h/.cpp     # HTTP 路由与业务逻辑
        ├── util/               # SHA-256 / Base64 / JWT / 时间与密码
        └── store/              # Repository 抽象 + JsonStore + MySqlStore
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
`admin.enabled`，首次运行自动创建管理员账号（admin/***REDACTED***，上线前务必改）。

## 五、接入 MySQL 8（本机已完成）

本机现状（2026-10 已落地并验证）：MySQL **8.0.46** 安装在 `D:\MySQL8`，
数据目录 `D:\MySQL8\data`，运行端口 **3307**（避开旧版 MySQL 5.0 服务的 3306，两者互不影响）。
账号 `blog / ***REDACTED***`，库 `blog`，5 张表已建好，后端 MySQL 模式冒烟测试 18/18 通过。

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

:: 3. 导入建表脚本 + 创建账号（仓库内置 setup_mysql.py 一键完成）
python server\setup_mysql.py
```

`server/config.json` 当前配置（MySQL 模式）：

```json
{ "store": "mysql",
  "mysql": { "host": "127.0.0.1", "port": 3307, "user": "blog",
             "password": "***REDACTED***", "database": "blog", "pool_size": 8 } }
```

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

## 六、API 一览

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

## 七、技术要点（面试可讲）

1. **MySQL 连接池**：固定大小 + 互斥锁/条件变量 + 探活重建 + RAII 租约，避免每请求建连。
2. **SQL 注入防护**：全部写操作走 `mysql_stmt_*` 预编译绑定，不拼接 SQL。
3. **JWT 自实现**：HMAC-SHA256 签名 + 常量时间比较验签 + exp 过期校验；`util/` 下无第三方依赖。
4. **Repository 模式**：上层业务与存储解耦，JsonStore/MySqlStore 一键切换，利于本地开发与生产一致。
5. **SHA-256 自实现**：含 FIPS 官方测试向量自检，启动时验证实现正确性。

## 八、开发路线（对照项目书 8 周计划）

- [x] 第 1 周：项目书、DB 建模、API 设计
- [x] 后端起步：HTTP 框架、仓储、JWT、文章/标签/评论核心接口
- [ ] 前端开发：Vue3/React + 列表/详情/后台管理
- [ ] 性能与安全：Redis 缓存/限流、XSS 防护、浏览计数落库
- [ ] 桌面端：Electron 壳、本地草稿、一键发布
- [ ] 测试修复与上线：阿里云部署、README、简历描述

## 九、安全说明（MVP 取舍）

- 密码散列当前为 `盐$SHA256hex`（教学演示）；生产建议换 bcrypt/argon2。
- JWT 密钥放 config.json；生产应改环境变量/密钥管理。
- 本地 MySQL 8 与旧 MySQL 5.0（3306）端口隔离，互不影响。
