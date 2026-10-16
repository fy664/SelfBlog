#pragma once
// =============================================================
// 服务配置加载（config.json -> Config 结构体）
// 字段缺失时使用默认值，保证零配置也能启动。
// =============================================================
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace blog {

struct Config {
  std::string host = "0.0.0.0";
  int port = 8080;

  // 存储后端：json（本地开发） / mysql（服务器部署）
  std::string store = "json";
  std::string data_dir = "./data";  // JsonStore 数据目录

  // JWT
  std::string jwt_secret = "***REDACTED_SECRET***";
  long jwt_expire_hours = 72;

  // MySQL（store=mysql 时生效）
  struct MySqlCfg {
    std::string host = "127.0.0.1";
    int port = 3306;
    std::string user = "blog";
    std::string password = "***REDACTED***";
    std::string database = "blog";
    int pool_size = 8;
  } mysql;

  // 启动时引导创建的管理员账号（仅当不存在同名用户时）
  struct AdminCfg {
    bool enabled = false;
    std::string username = "admin";
    std::string password = "***REDACTED***";
  } admin;

  // 从 JSON 对象填充（缺失字段保留默认值）
  static Config from_json(const nlohmann::json& j) {
    Config c;
    if (j.contains("host")) c.host = j["host"].get<std::string>();
    if (j.contains("port")) c.port = j["port"].get<int>();
    if (j.contains("store")) c.store = j["store"].get<std::string>();
    if (j.contains("data_dir")) c.data_dir = j["data_dir"].get<std::string>();
    if (j.contains("jwt_secret")) c.jwt_secret = j["jwt_secret"].get<std::string>();
    if (j.contains("jwt_expire_hours")) c.jwt_expire_hours = j["jwt_expire_hours"].get<long>();
    if (j.contains("mysql")) {
      const auto& m = j["mysql"];
      if (m.contains("host")) c.mysql.host = m["host"].get<std::string>();
      if (m.contains("port")) c.mysql.port = m["port"].get<int>();
      if (m.contains("user")) c.mysql.user = m["user"].get<std::string>();
      if (m.contains("password")) c.mysql.password = m["password"].get<std::string>();
      if (m.contains("database")) c.mysql.database = m["database"].get<std::string>();
      if (m.contains("pool_size")) c.mysql.pool_size = m["pool_size"].get<int>();
    }
    if (j.contains("admin")) {
      const auto& a = j["admin"];
      if (a.contains("enabled")) c.admin.enabled = a["enabled"].get<bool>();
      if (a.contains("username")) c.admin.username = a["username"].get<std::string>();
      if (a.contains("password")) c.admin.password = a["password"].get<std::string>();
    }
    return c;
  }

  // 读取配置文件；文件不存在时返回 false（由调用方决定是否生成默认文件）
  static Config load(const std::string& path) {
    nlohmann::json j;
    std::ifstream in(path);
    if (in) {
      try {
        in >> j;
      } catch (...) {
        j = nlohmann::json::object();
      }
    }
    return from_json(j);
  }
};

}  // namespace blog
