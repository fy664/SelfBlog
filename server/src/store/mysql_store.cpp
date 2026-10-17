#include "mysql_store.h"

#ifdef HAVE_MYSQL

#include "../util/logger.h"

#include <mysql.h>

#include <cstdio>
#include <cstring>
#include <memory>

namespace blog {

// ---------- 预编译语句工具 ----------

namespace {

// 执行 INSERT/UPDATE/DELETE 等无结果集语句
bool exec_stmt(MYSQL* c, const std::string& sql,
               const std::vector<std::string>& params) {
  MYSQL_STMT* stmt = mysql_stmt_init(c);
  if (!stmt) return false;
  if (mysql_stmt_prepare(stmt, sql.c_str(), (unsigned long)sql.size()) != 0) {
    fprintf(stderr, "[MySqlStore] prepare失败: %s\n", mysql_stmt_error(stmt));
    logger::error(std::string("[MySqlStore] prepare失败: ") + mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }
  std::vector<MYSQL_BIND> binds;
  std::vector<unsigned long> lens;
  std::unique_ptr<bool[]> nulls;
  if (!params.empty()) {
    binds.resize(params.size());
    lens.resize(params.size());
    nulls.reset(new bool[params.size()]());
    std::memset(binds.data(), 0, binds.size() * sizeof(MYSQL_BIND));
    for (size_t i = 0; i < params.size(); ++i) {
      lens[i] = (unsigned long)params[i].size();
      binds[i].buffer_type = MYSQL_TYPE_STRING;
      binds[i].buffer = (void*)params[i].data();
      binds[i].buffer_length = lens[i];
      binds[i].length = &lens[i];
      binds[i].is_null = &nulls[i];
    }
    if (mysql_stmt_bind_param(stmt, binds.data()) != 0) {
      fprintf(stderr, "[MySqlStore] bind_param失败: %s\n", mysql_stmt_error(stmt));
      logger::error(std::string("[MySqlStore] bind_param失败: ") + mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return false;
    }
  }
  bool ok = mysql_stmt_execute(stmt) == 0;
  if (!ok) {
    fprintf(stderr, "[MySqlStore] execute失败: %s\n", mysql_stmt_error(stmt));
    logger::error(std::string("[MySqlStore] execute失败: ") + mysql_stmt_error(stmt));
  }
  mysql_stmt_close(stmt);
  return ok;
}

// 查询语句：返回行集合（全部列按字符串读取，NULL 转空串）
bool query_rows(MYSQL* c, const std::string& sql,
                const std::vector<std::string>& params,
                std::vector<std::vector<std::string>>& rows) {
  rows.clear();
  MYSQL_STMT* stmt = mysql_stmt_init(c);
  if (!stmt) return false;
  if (mysql_stmt_prepare(stmt, sql.c_str(), (unsigned long)sql.size()) != 0) {
    fprintf(stderr, "[MySqlStore] prepare失败: %s\n", mysql_stmt_error(stmt));
    logger::error(std::string("[MySqlStore] prepare失败: ") + mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }
  std::vector<MYSQL_BIND> binds;
  std::vector<unsigned long> lens;
  std::unique_ptr<bool[]> nulls;
  if (!params.empty()) {
    binds.resize(params.size());
    lens.resize(params.size());
    nulls.reset(new bool[params.size()]());
    std::memset(binds.data(), 0, binds.size() * sizeof(MYSQL_BIND));
    for (size_t i = 0; i < params.size(); ++i) {
      lens[i] = (unsigned long)params[i].size();
      binds[i].buffer_type = MYSQL_TYPE_STRING;
      binds[i].buffer = (void*)params[i].data();
      binds[i].buffer_length = lens[i];
      binds[i].length = &lens[i];
      binds[i].is_null = &nulls[i];
    }
    if (mysql_stmt_bind_param(stmt, binds.data()) != 0) {
      fprintf(stderr, "[MySqlStore] bind_param失败: %s\n", mysql_stmt_error(stmt));
      logger::error(std::string("[MySqlStore] bind_param失败: ") + mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return false;
    }
  }
  if (mysql_stmt_execute(stmt) != 0) {
    fprintf(stderr, "[MySqlStore] execute失败: %s\n", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }
  // 结果集元数据
  MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
  if (!meta) {  // 非 SELECT（无结果集），视为成功但无行
    mysql_stmt_close(stmt);
    return true;
  }
  unsigned int ncol = mysql_num_fields(meta);
  std::vector<MYSQL_BIND> rbinds(ncol);
  std::vector<std::string> cells(ncol);
  std::vector<unsigned long> rlens(ncol, 0);
  std::unique_ptr<bool[]> rnull(new bool[ncol]());
  std::unique_ptr<bool[]> rerr(new bool[ncol]());
  std::memset(rbinds.data(), 0, rbinds.size() * sizeof(MYSQL_BIND));
  for (unsigned int i = 0; i < ncol; ++i) {
    cells[i].resize(1024, '\0');  // 预留 1KB；超长列会截断（业务字段足够）
    rbinds[i].buffer_type = MYSQL_TYPE_STRING;
    rbinds[i].buffer = (void*)cells[i].data();
    rbinds[i].buffer_length = (unsigned long)cells[i].size();
    rbinds[i].length = &rlens[i];
    rbinds[i].is_null = &rnull[i];
    rbinds[i].error = &rerr[i];
  }
  mysql_stmt_bind_result(stmt, rbinds.data());
  for (;;) {
    int rc = mysql_stmt_fetch(stmt);
    if (rc == 100 /* MYSQL_NO_DATA */) break;
    if (rc != 0) continue;  // 数据截断等非致命错误跳过
    std::vector<std::string> row;
    row.reserve(ncol);
    for (unsigned int i = 0; i < ncol; ++i) {
      if (rnull[i]) {
        row.emplace_back();
      } else {
        row.emplace_back(cells[i].data(), rlens[i]);
      }
    }
    rows.push_back(std::move(row));
  }
  mysql_free_result(meta);
  mysql_stmt_close(stmt);
  return true;
}

// 通用：把查询行转成 Article（第 0 列起按固定顺序映射）
void fill_article(const std::vector<std::string>& r, Article& a) {
  a.id = std::stoll(r[0]);
  a.title = r[1];
  a.content = r[2];
  a.summary = r[3];
  a.cover_img = r[4];
  a.status = r[5];
  a.view_count = std::stoll(r[6]);
  a.user_id = std::stoll(r[7]);
  a.created_at = r[8];
  a.updated_at = r[9];
}

}  // namespace

// ---------- 实现 ----------

MySqlStore::MySqlStore(const Config& cfg)
    : pool_({cfg.host, cfg.user, cfg.password, cfg.database, cfg.port, cfg.pool_size}) {}

bool MySqlStore::create_user(const std::string& username, const std::string& password,
                             const std::string& role, const std::string& register_ip,
                             const std::string& device_id, std::string& err) {
  auto lease = pool_.acquire();
  if (!lease) { err = "数据库连接不可用"; return false; }
  // 预编译 INSERT，username 唯一约束冲突即失败
  if (!exec_stmt(lease.get(),
                 "INSERT INTO `user`(`username`,`password`,`role`,`register_ip`,`device_id`)"
                 " VALUES(?,?,?,?,?)",
                 {username, password, role, register_ip, device_id})) {
    err = "创建用户失败（可能用户名已存在）";
    return false;
  }
  return true;
}

bool MySqlStore::user_device_exists(const std::string& device_id) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  if (device_id.empty()) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT COUNT(*) FROM `user` WHERE `device_id`=? LIMIT 1",
                  {device_id}, rows) || rows.empty()) {
    return false;
  }
  return rows[0][0] != "0";
}

bool MySqlStore::list_users(std::vector<User>& out) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT `id`,`username`,`role`,`register_ip`,`device_id`,`created_at` "
                  "FROM `user` ORDER BY `id`",
                  {}, rows)) {
    return false;
  }
  for (auto& r : rows) {
    User u;
    u.id = std::stoll(r[0]);
    u.username = r[1];
    u.role = r[2];
    u.register_ip = r[3];
    u.device_id = r[4];
    u.created_at = r[5];
    out.push_back(u);
  }
  return true;
}

bool MySqlStore::get_user_by_name(const std::string& username, User& out) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT `id`,`username`,`password`,`role`,`register_ip`,`device_id`,`created_at` "
                  "FROM `user` WHERE `username`=? LIMIT 1",
                  {username}, rows) ||
      rows.empty()) {
    return false;
  }
  out.id = std::stoll(rows[0][0]);
  out.username = rows[0][1];
  out.password = rows[0][2];
  out.role = rows[0][3];
  out.register_ip = rows[0][4];
  out.device_id = rows[0][5];
  out.created_at = rows[0][6];
  return true;
}

bool MySqlStore::get_user_by_id(int64_t id, User& out) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT `id`,`username`,`password`,`role`,`register_ip`,`device_id`,`created_at` "
                  "FROM `user` WHERE `id`=? LIMIT 1",
                  {std::to_string(id)}, rows) ||
      rows.empty()) {
    return false;
  }
  out.id = id;
  out.username = rows[0][1];
  out.password = rows[0][2];
  out.role = rows[0][3];
  out.register_ip = rows[0][4];
  out.device_id = rows[0][5];
  out.created_at = rows[0][6];
  return true;
}

int64_t MySqlStore::create_article(const Article& a, std::string& err) {
  auto lease = pool_.acquire();
  if (!lease) { err = "数据库连接不可用"; return 0; }

  // 1) 标签：INSERT IGNORE 幂等创建
  for (const auto& t : a.tags) {
    exec_stmt(lease.get(), "INSERT IGNORE INTO `tag`(`name`) VALUES(?)", {t});
  }
  // 2) 文章主体
  if (!exec_stmt(lease.get(),
                 "INSERT INTO `article`(`title`,`content`,`summary`,`cover_img`,"
                 "`status`,`view_count`,`user_id`) VALUES(?,?,?,?,?,0,?)",
                 {a.title, a.content, a.summary, a.cover_img, a.status,
                  std::to_string(a.user_id)})) {
    err = "文章创建失败";
    return 0;
  }
  MYSQL_STMT* stmt = mysql_stmt_init(lease.get());
  if (!stmt) { err = "数据库错误"; return 0; }
  // 注意：长度参数必须与 SQL 实际长度一致（"SELECT LAST_INSERT_ID()" 为 23 字节）
  mysql_stmt_prepare(stmt, "SELECT LAST_INSERT_ID()",
                     (unsigned long)std::strlen("SELECT LAST_INSERT_ID()"));
  mysql_stmt_execute(stmt);
  std::vector<MYSQL_BIND> rb(1);
  std::memset(rb.data(), 0, sizeof(MYSQL_BIND));
  unsigned long long last_id = 0;
  unsigned long len = 0;
  bool is_null = false;
  rb[0].buffer_type = MYSQL_TYPE_LONGLONG;
  rb[0].buffer = &last_id;
  rb[0].is_null = &is_null;
  rb[0].length = &len;
  mysql_stmt_bind_result(stmt, rb.data());
  mysql_stmt_fetch(stmt);
  mysql_stmt_close(stmt);
  int64_t article_id = (int64_t)last_id;
  if (article_id <= 0) { err = "文章创建失败"; return 0; }

  // 3) 关联标签
  for (const auto& t : a.tags) {
    exec_stmt(lease.get(),
              "INSERT INTO `article_tag`(`article_id`,`tag_id`) "
              "SELECT ?,`id` FROM `tag` WHERE `name`=?",
              {std::to_string(article_id), t});
  }
  return article_id;
}

bool MySqlStore::get_article_by_id(int64_t id, Article& out, bool increment_view) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT `id`,`title`,`content`,`summary`,`cover_img`,`status`,"
                  "`view_count`,`user_id`,`created_at`,`updated_at` "
                  "FROM `article` WHERE `id`=? LIMIT 1",
                  {std::to_string(id)}, rows) ||
      rows.empty()) {
    return false;
  }
  fill_article(rows[0], out);

  // 浏览量 +1
  if (increment_view) {
    exec_stmt(lease.get(), "UPDATE `article` SET `view_count`=`view_count`+1 WHERE `id`=?",
              {std::to_string(id)});
    out.view_count += 1;
  }
  // 标签
  if (!query_rows(lease.get(),
                  "SELECT t.`name` FROM `tag` t "
                  "JOIN `article_tag` at ON at.`tag_id`=t.`id` "
                  "WHERE at.`article_id`=? ORDER BY t.`id`",
                  {std::to_string(id)}, rows)) {
    return false;
  }
  for (auto& r : rows) out.tags.push_back(r[0]);
  return true;
}

bool MySqlStore::list_articles(int page, int size, const std::string& tag,
                               const std::string& sort, bool include_all,
                               int64_t owner_id,
                               std::vector<Article>& out, int64_t& total) {
  auto lease = pool_.acquire();
  if (!lease) return false;

  // 动态拼接 WHERE（参数全部走占位符，防注入）
  std::string where = include_all ? "" : " WHERE a.`status`='published'";
  std::vector<std::string> params;
  // 普通用户管理列表：只看自己的文章（含草稿）；admin 传 0 不看全部
  if (owner_id > 0) {
    where += (where.empty() ? " WHERE " : " AND ");
    where += "a.`user_id`=?";
    params.push_back(std::to_string(owner_id));
  }
  if (!tag.empty()) {
    where += (where.empty() ? " WHERE " : " AND ");
    where += "a.`id` IN (SELECT at.`article_id` FROM `article_tag` at "
             "JOIN `tag` t ON t.`id`=at.`tag_id` WHERE t.`name`=?)";
    params.push_back(tag);
  }

  // 总数
  std::vector<std::vector<std::string>> cnt;
  query_rows(lease.get(), "SELECT COUNT(*) FROM `article` a" + where, params, cnt);
  total = cnt.empty() ? 0 : std::stoll(cnt[0][0]);

  // 数据（分页）
  std::string order = (sort == "views") ? " ORDER BY a.`view_count` DESC"
                                        : " ORDER BY a.`created_at` DESC";
  std::string sql =
      "SELECT a.`id`,`title`,`content`,`summary`,`cover_img`,`status`,"
      "`view_count`,`user_id`,`created_at`,`updated_at` FROM `article` a" +
      where + order + " LIMIT ? OFFSET ?";
  std::vector<std::string> page_params = params;
  page_params.push_back(std::to_string(size));
  page_params.push_back(std::to_string((page - 1) * size));

  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(), sql, page_params, rows)) return false;
  for (auto& r : rows) {
    Article a;
    fill_article(r, a);
    // 每篇文章的标签（N+1 查询，MVP 可接受；后续可 GROUP_CONCAT 优化）
    std::vector<std::vector<std::string>> trows;
    if (query_rows(lease.get(),
                   "SELECT t.`name` FROM `tag` t "
                   "JOIN `article_tag` at ON at.`tag_id`=t.`id` "
                   "WHERE at.`article_id`=? ORDER BY t.`id`",
                   {std::to_string(a.id)}, trows)) {
      for (auto& tr : trows) a.tags.push_back(tr[0]);
    }
    out.push_back(std::move(a));
  }
  return true;
}

bool MySqlStore::update_article(const Article& a, std::string& err) {
  auto lease = pool_.acquire();
  if (!lease) { err = "数据库连接不可用"; return false; }
  if (!exec_stmt(lease.get(),
                 "UPDATE `article` SET `title`=?,`content`=?,`summary`=?,"
                 "`cover_img`=?,`status`=? WHERE `id`=?",
                 {a.title, a.content, a.summary, a.cover_img, a.status,
                  std::to_string(a.id)})) {
    err = "文章更新失败";
    return false;
  }
  // 重建标签关联
  exec_stmt(lease.get(), "DELETE FROM `article_tag` WHERE `article_id`=?",
            {std::to_string(a.id)});
  for (const auto& t : a.tags) {
    exec_stmt(lease.get(), "INSERT IGNORE INTO `tag`(`name`) VALUES(?)", {t});
    exec_stmt(lease.get(),
              "INSERT INTO `article_tag`(`article_id`,`tag_id`) SELECT ?,`id` "
              "FROM `tag` WHERE `name`=?",
              {std::to_string(a.id), t});
  }
  return true;
}

bool MySqlStore::delete_article(int64_t id, std::string& err) {
  auto lease = pool_.acquire();
  if (!lease) { err = "数据库连接不可用"; return false; }
  // 外键 ON DELETE CASCADE 自动清理 article_tag 与 comment
  if (!exec_stmt(lease.get(), "DELETE FROM `article` WHERE `id`=?", {std::to_string(id)})) {
    err = "文章删除失败";
    return false;
  }
  return true;
}

bool MySqlStore::list_tags(std::vector<Tag>& out) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT `id`,`name`,`created_at` FROM `tag` ORDER BY `id`", {}, rows)) {
    return false;
  }
  for (auto& r : rows) {
    Tag t;
    t.id = std::stoll(r[0]);
    t.name = r[1];
    t.created_at = r[2];
    out.push_back(t);
  }
  return true;
}

bool MySqlStore::add_comment(const Comment& c, std::string& err) {
  auto lease = pool_.acquire();
  if (!lease) { err = "数据库连接不可用"; return false; }
  if (!exec_stmt(lease.get(),
                 "INSERT INTO `comment`(`article_id`,`user_id`,`content`) VALUES(?,?,?)",
                 {std::to_string(c.article_id), std::to_string(c.user_id), c.content})) {
    err = "评论发布失败";
    return false;
  }
  return true;
}

bool MySqlStore::list_comments(int64_t article_id, std::vector<Comment>& out) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT c.`id`,c.`article_id`,c.`user_id`,u.`username`,"
                  "c.`content`,c.`created_at` FROM `comment` c "
                  "JOIN `user` u ON u.`id`=c.`user_id` "
                  "WHERE c.`article_id`=? ORDER BY c.`created_at`",
                  {std::to_string(article_id)}, rows)) {
    return false;
  }
  for (auto& r : rows) {
    Comment c;
    c.id = std::stoll(r[0]);
    c.article_id = std::stoll(r[1]);
    c.user_id = std::stoll(r[2]);
    c.username = r[3];
    c.content = r[4];
    c.created_at = r[5];
    out.push_back(c);
  }
  return true;
}

bool MySqlStore::get_comment_by_id(int64_t id, Comment& out) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT c.`id`,c.`article_id`,c.`user_id`,u.`username`,"
                  "c.`content`,c.`created_at` FROM `comment` c "
                  "JOIN `user` u ON u.`id`=c.`user_id` WHERE c.`id`=?",
                  {std::to_string(id)}, rows)) {
    return false;
  }
  if (rows.empty()) return false;
  out.id = std::stoll(rows[0][0]);
  out.article_id = std::stoll(rows[0][1]);
  out.user_id = std::stoll(rows[0][2]);
  out.username = rows[0][3];
  out.content = rows[0][4];
  out.created_at = rows[0][5];
  return true;
}

bool MySqlStore::delete_comment(int64_t id, std::string& err) {
  auto lease = pool_.acquire();
  if (!lease) { err = "数据库连接不可用"; return false; }
  if (!exec_stmt(lease.get(), "DELETE FROM `comment` WHERE `id`=?",
                 {std::to_string(id)})) {
    err = "评论删除失败";
    return false;
  }
  return true;
}

bool MySqlStore::record_login(const std::string& username, const std::string& ip,
                              const std::string& user_agent, bool success) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  // UA 截断到 255 字符（列宽上限），避免超长被拒
  std::string ua = user_agent;
  if (ua.size() > 255) ua.resize(255);
  return exec_stmt(lease.get(),
                   "INSERT INTO `login_log`(`username`,`ip`,`user_agent`,`success`)"
                   " VALUES(?,?,?,?)",
                   {username, ip, ua, success ? "1" : "0"});
}

bool MySqlStore::list_login_logs(int limit, std::vector<LoginLog>& out) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  if (!query_rows(lease.get(),
                  "SELECT `id`,`username`,`ip`,`user_agent`,`success`,`created_at`"
                  " FROM `login_log` ORDER BY `id` DESC LIMIT ?",
                  {std::to_string(limit)}, rows)) {
    return false;
  }
  for (auto& r : rows) {
    LoginLog l;
    l.id = std::stoll(r[0]);
    l.username = r[1];
    l.ip = r[2];
    l.user_agent = r[3];
    l.success = r[4] == "1";
    l.created_at = r[5];
    out.push_back(l);
  }
  return true;
}

bool MySqlStore::get_stats(int64_t& article_count, int64_t& published_count,
                           int64_t& view_total, int64_t& comment_count) {
  auto lease = pool_.acquire();
  if (!lease) return false;
  std::vector<std::vector<std::string>> rows;
  query_rows(lease.get(),
             "SELECT (SELECT COUNT(*) FROM `article`),"
             "(SELECT COUNT(*) FROM `article` WHERE `status`='published'),"
             "(SELECT IFNULL(SUM(`view_count`),0) FROM `article`),"
             "(SELECT COUNT(*) FROM `comment`)",
             {}, rows);
  if (rows.empty()) return false;
  article_count = std::stoll(rows[0][0]);
  published_count = std::stoll(rows[0][1]);
  view_total = std::stoll(rows[0][2]);
  comment_count = std::stoll(rows[0][3]);
  return true;
}

}  // namespace blog

#endif  // HAVE_MYSQL
