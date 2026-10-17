#pragma once
// =============================================================
// JsonStore：本地开发模式的仓储实现
// 数据以 JSON 文件形式保存在 data_dir 下：
//   users.json / articles.json / tags.json / comments.json
// 特点：零外部依赖、开箱即用；仅用于开发与演示，
// 生产部署请使用 MySqlStore（见 mysql_store.h）。
// 线程安全：单互斥锁串行化全部操作（开发模式量级足够）。
// =============================================================
#include "store.h"

#include <nlohmann/json.hpp>

#include <mutex>
#include <string>

namespace blog {

class JsonStore : public Repository {
 public:
  // data_dir：数据目录，不存在时自动创建
  explicit JsonStore(const std::string& data_dir);

  // 用户
  bool create_user(const std::string& username, const std::string& password,
                   const std::string& role, const std::string& register_ip,
                   std::string& err) override;
  bool get_user_by_name(const std::string& username, User& out) override;
  bool get_user_by_id(int64_t id, User& out) override;
  bool user_ip_exists(const std::string& ip) override;
  bool list_users(std::vector<User>& out) override;

  // 文章
  int64_t create_article(const Article& a, std::string& err) override;
  bool get_article_by_id(int64_t id, Article& out, bool increment_view) override;
  bool list_articles(int page, int size, const std::string& tag,
                     const std::string& sort, bool include_all,
                     std::vector<Article>& out, int64_t& total) override;
  bool update_article(const Article& a, std::string& err) override;
  bool delete_article(int64_t id, std::string& err) override;

  // 标签
  bool list_tags(std::vector<Tag>& out) override;

  // 评论
  bool add_comment(const Comment& c, std::string& err) override;
  bool list_comments(int64_t article_id, std::vector<Comment>& out) override;

  // 登录日志
  bool record_login(const std::string& username, const std::string& ip,
                    const std::string& user_agent, bool success) override;
  bool list_login_logs(int limit, std::vector<LoginLog>& out) override;

  // 统计
  bool get_stats(int64_t& article_count, int64_t& published_count,
                 int64_t& view_total, int64_t& comment_count) override;

 private:
  // 通用读写：文件名 -> JSON 数组
  nlohmann::json load(const std::string& name);
  void save(const std::string& name, const nlohmann::json& data);

  std::string dir_;
  std::mutex mtx_;
};

}  // namespace blog
