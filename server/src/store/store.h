#pragma once
// =============================================================
// 数据层抽象（Repository 模式）
// 上层业务（handlers）只依赖本接口，不关心底层是 JSON 文件还是 MySQL：
//   - JsonStore：本地开发 / 演示，零依赖，数据落盘为 JSON 文件
//   - MySqlStore：生产部署（服务器），MySQL 8 + 预编译语句防注入
// 通过 config.json 的 "store" 字段切换。
// =============================================================
#include <cstdint>
#include <string>
#include <vector>

namespace blog {

// ---- 领域模型（DTO）----

struct User {
  int64_t id = 0;
  std::string username;
  std::string password;   // 存储格式：盐$SHA256hex
  std::string avatar;
  std::string role;       // admin / author
  std::string created_at;
};

struct Article {
  int64_t id = 0;
  std::string title;
  std::string content;
  std::string summary;
  std::string cover_img;
  std::string status;      // draft / published
  int64_t view_count = 0;
  int64_t user_id = 0;
  std::string created_at;
  std::string updated_at;
  std::vector<std::string> tags;  // 标签名列表（联表/内联）
};

struct Tag {
  int64_t id = 0;
  std::string name;
  std::string created_at;
};

struct Comment {
  int64_t id = 0;
  int64_t article_id = 0;
  int64_t user_id = 0;
  std::string username;   // 冗余作者名，便于列表展示
  std::string content;
  std::string created_at;
};

// ---- 仓储接口 ----

class Repository {
 public:
  virtual ~Repository() = default;

  // 用户
  virtual bool create_user(const std::string& username,
                           const std::string& password,
                           const std::string& role, std::string& err) = 0;
  virtual bool get_user_by_name(const std::string& username, User& out) = 0;
  virtual bool get_user_by_id(int64_t id, User& out) = 0;

  // 文章
  virtual int64_t create_article(const Article& a, std::string& err) = 0;
  virtual bool get_article_by_id(int64_t id, Article& out, bool increment_view) = 0;
  virtual bool list_articles(int page, int size, const std::string& tag,
                             const std::string& sort, bool include_all,
                             std::vector<Article>& out, int64_t& total) = 0;
  virtual bool update_article(const Article& a, std::string& err) = 0;
  virtual bool delete_article(int64_t id, std::string& err) = 0;

  // 标签
  virtual bool list_tags(std::vector<Tag>& out) = 0;

  // 评论
  virtual bool add_comment(const Comment& c, std::string& err) = 0;
  virtual bool list_comments(int64_t article_id, std::vector<Comment>& out) = 0;

  // 统计
  virtual bool get_stats(int64_t& article_count, int64_t& published_count,
                         int64_t& view_total, int64_t& comment_count) = 0;
};

}  // namespace blog
