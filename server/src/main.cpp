// =============================================================
// 全栈个人博客系统 - C++ 后端主程序
//
// 启动流程：
//   1. 加载 config.json（缺失时生成默认配置）
//   2. 自检 SHA-256 实现（与 FIPS 官方测试向量比对）
//   3. 按 store 字段初始化仓储：
//        json  -> JsonStore（本地开发，零依赖）
//        mysql -> MySqlStore（生产部署，MySQL 8，需 HAVE_MYSQL 编译）
//   4. 可选：引导创建管理员账号（config.admin）
//   5. 注册路由并监听端口
//
// 编译：
//   Windows: build.bat（MSVC + CMake）
//   Ubuntu : cmake -S . -B build && cmake --build build -j
// =============================================================
#include "config.h"
#include "handlers.h"
#include "store/json_store.h"
#include "store/store.h"
#include "util/misc.h"
#include "util/sha256.h"

#include <httplib.h>

#include <cstdio>
#include <fstream>
#include <memory>

#ifdef HAVE_MYSQL
#include "store/mysql_store.h"
#endif

namespace {

// 生成一份默认 config.json（便于首次启动）
void ensure_default_config(const std::string& path) {
  std::ifstream in(path);
  if (in) return;
  std::ofstream out(path);
  out << R"({
  "host": "0.0.0.0",
  "port": 8080,
  "store": "json",
  "data_dir": "./data",
  "jwt_secret": "***REDACTED_SECRET***",
  "jwt_expire_hours": 72,
  "mysql": {
    "host": "127.0.0.1",
    "port": 3306,
    "user": "blog",
    "password": "***REDACTED***",
    "database": "blog",
    "pool_size": 8
  },
  "admin": {
    "enabled": true,
    "username": "admin",
    "password": "***REDACTED***"
  }
}
)";
  std::printf("[启动] 已生成默认配置: %s\n", path.c_str());
}

// 定位配置文件：优先当前目录，其次父目录（兼容从 build/Release 启动）
std::string find_config() {
  if (std::ifstream("./config.json")) return "./config.json";
  if (std::ifstream("../config.json")) return "../config.json";
  ensure_default_config("./config.json");
  return "./config.json";
}

}  // namespace

int main() {
  std::printf("============================================\n");
  std::printf(" 全栈个人博客系统 - C++ 后端\n");
  std::printf("============================================\n");

  // 1) 配置
  std::string config_path = find_config();
  blog::Config cfg = blog::Config::load(config_path);
  std::printf("[配置] host=%s port=%d store=%s\n", cfg.host.c_str(), cfg.port,
              cfg.store.c_str());

  // 2) SHA-256 自检
  if (util::sha256_self_test()) {
    std::printf("[自检] SHA-256 实现通过官方测试向量\n");
  } else {
    std::printf("[错误] SHA-256 自检失败，请检查实现\n");
    return 1;
  }

  // 3) 仓储初始化
  std::unique_ptr<blog::Repository> repo;
  if (cfg.store == "mysql") {
#ifdef HAVE_MYSQL
    blog::MySqlStore::Config mc;
    mc.host = cfg.mysql.host;
    mc.port = cfg.mysql.port;
    mc.user = cfg.mysql.user;
    mc.password = cfg.mysql.password;
    mc.database = cfg.mysql.database;
    mc.pool_size = cfg.mysql.pool_size;
    repo.reset(new blog::MySqlStore(mc));
    std::printf("[存储] MySqlStore 已启用（%s:%d/%s，池大小 %d）\n",
                mc.host.c_str(), mc.port, mc.database.c_str(), mc.pool_size);
#else
    std::printf("[错误] 配置 store=mysql 但本机未编译 MySQL 支持\n");
    std::printf("[提示] Windows 需安装 MySQL 开发库后重新 cmake；Ubuntu 执行 apt install libmysqlclient-dev\n");
    return 1;
#endif
  } else {
    repo.reset(new blog::JsonStore(cfg.data_dir));
    std::printf("[存储] JsonStore 已启用（数据目录 %s）\n", cfg.data_dir.c_str());
  }

  // 4) 管理员引导创建
  if (cfg.admin.enabled && !cfg.admin.username.empty() && !cfg.admin.password.empty()) {
    blog::User admin;
    if (!repo->get_user_by_name(cfg.admin.username, admin)) {
      std::string err;
      if (repo->create_user(cfg.admin.username,
                            util::password_hash(cfg.admin.password), "admin",
                            "", err)) {
        std::printf("[引导] 管理员账号已创建: %s（密码见 config.json）\n",
                    cfg.admin.username.c_str());
      } else {
        std::printf("[警告] 管理员创建失败: %s\n", err.c_str());
      }
    }
  }

  // 5) 路由与监听
  blog::Ctx ctx;
  ctx.repo = repo.get();
  ctx.jwt_secret = cfg.jwt_secret;
  ctx.jwt_expire_hours = cfg.jwt_expire_hours;

  httplib::Server svr;
  blog::register_routes(svr, ctx);

  std::printf("[监听] 启动 HTTP 服务 http://%s:%d\n", cfg.host.c_str(), cfg.port);
  std::printf("       接口一览：/api/health /api/auth/register|login "
              "/api/articles /api/tags /api/comments /api/stat/overview\n");

  if (!svr.listen(cfg.host.c_str(), cfg.port)) {
    std::printf("[错误] 端口 %d 监听失败（可能被占用）\n", cfg.port);
    return 1;
  }
  return 0;
}
