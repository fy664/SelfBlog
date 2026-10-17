#pragma once
// =============================================================
// MySqlStore：生产部署用的 MySQL 8 仓储实现
// 仅在 HAVE_MYSQL 编译宏开启时参与编译（CMake 自动探测 mysql.h）
// 全部写操作使用 mysql_stmt_* 预编译语句，杜绝 SQL 注入
// （参数通过 MYSQL_BIND 绑定，不拼接字符串）。
// =============================================================
#ifdef HAVE_MYSQL

#include "mysql_pool.h"
#include "store.h"

#include <string>

namespace blog {

class MySqlStore : public Repository {
 public:
  struct Config {
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    int port = 3306;
    int pool_size = 8;
  };

  explicit MySqlStore(const Config& cfg);

  bool create_user(const std::string& username, const std::string& password,
                   const std::string& role, std::string& err) override;
  bool get_user_by_name(const std::string& username, User& out) override;
  bool get_user_by_id(int64_t id, User& out) override;

  int64_t create_article(const Article& a, std::string& err) override;
  bool get_article_by_id(int64_t id, Article& out, bool increment_view) override;
  bool list_articles(int page, int size, const std::string& tag,
                     const std::string& sort, bool include_all,
                     std::vector<Article>& out, int64_t& total) override;
  bool update_article(const Article& a, std::string& err) override;
  bool delete_article(int64_t id, std::string& err) override;

  bool list_tags(std::vector<Tag>& out) override;

  bool add_comment(const Comment& c, std::string& err) override;
  bool list_comments(int64_t article_id, std::vector<Comment>& out) override;

  bool record_login(const std::string& username, const std::string& ip,
                    const std::string& user_agent, bool success) override;
  bool list_login_logs(int limit, std::vector<LoginLog>& out) override;

  bool get_stats(int64_t& article_count, int64_t& published_count,
                 int64_t& view_total, int64_t& comment_count) override;

 private:
  MySqlPool pool_;
};

}  // namespace blog

#endif  // HAVE_MYSQL
