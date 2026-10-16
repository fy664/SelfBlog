#include "handlers.h"

#include "store/store.h"
#include "util/jwt.h"
#include "util/misc.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace blog {

namespace {

using nlohmann::json;

// ---------- 响应与请求辅助 ----------

// 统一响应包装：{code, msg, data}
void send_json(httplib::Response& res, int http_status, int code,
               const std::string& msg, const json& data) {
  json body = {{"code", code}, {"msg", msg}, {"data", data}};
  res.status = http_status;
  res.set_content(body.dump(), "application/json; charset=utf-8");
}

void ok(httplib::Response& res, const json& data, int http_status = 200) {
  send_json(res, http_status, 0, "ok", data);
}

void fail(httplib::Response& res, int code, const std::string& msg,
          int http_status = 400) {
  send_json(res, http_status, code, msg, json::object());
}

// 解析 JSON 请求体；失败返回 false
bool parse_body(const httplib::Request& req, httplib::Response& res, json& out) {
  try {
    out = json::parse(req.body);
  } catch (...) {
    fail(res, 400, "请求体不是合法 JSON", 400);
    return false;
  }
  if (!out.is_object()) {
    fail(res, 400, "请求体必须是 JSON 对象", 400);
    return false;
  }
  return true;
}

// 字符串去空白
std::string trim(const std::string& s) {
  size_t b = 0, e = s.size();
  while (b < e && std::isspace((unsigned char)s[b])) ++b;
  while (e > b && std::isspace((unsigned char)s[e - 1])) --e;
  return s.substr(b, e - b);
}

// ---------- 鉴权辅助 ----------

// 从 Authorization: Bearer <token> 提取 token
std::string bearer_token(const httplib::Request& req) {
  auto h = req.get_header_value("Authorization");
  if (h.rfind("Bearer ", 0) == 0) return h.substr(7);
  return "";
}

// 校验 JWT 并加载用户；失败时已写 401 响应
bool require_user(const httplib::Request& req, httplib::Response& res, Ctx& ctx,
                  User& out) {
  std::string token = bearer_token(req);
  json payload;
  if (token.empty() || !util::jwt_verify(token, ctx.jwt_secret, payload)) {
    fail(res, 401, "未登录或登录已过期", 401);
    return false;
  }
  if (!payload.contains("uid") || !payload["uid"].is_number_integer()) {
    fail(res, 401, "令牌无效", 401);
    return false;
  }
  int64_t uid = payload["uid"].get<int64_t>();
  if (!ctx.repo->get_user_by_id(uid, out)) {
    fail(res, 401, "用户不存在", 401);
    return false;
  }
  return true;
}

// 校验 username/password 基本规则
bool validate_credential(const std::string& username, const std::string& password,
                         std::string& err) {
  if (username.size() < 3 || username.size() > 32) {
    err = "用户名长度须为 3-32 字符";
    return false;
  }
  for (char c : username) {
    if (!std::isalnum((unsigned char)c) && c != '_') {
      err = "用户名只能包含字母、数字、下划线";
      return false;
    }
  }
  if (password.size() < 6 || password.size() > 64) {
    err = "密码长度须为 6-64 字符";
    return false;
  }
  return true;
}

// ---------- 文章参数校验 ----------

bool valid_status(const std::string& s) {
  return s == "draft" || s == "published";
}

// ---------- 各处理器 ----------

void handle_health(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  ok(res, {{"status", "up"}, {"time", util::now_iso()},
           {"store", ctx.repo ? "ready" : "uninitialized"}});
}

void handle_register(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  json body;
  if (!parse_body(req, res, body)) return;
  std::string username = trim(body.value("username", ""));
  std::string password = body.value("password", "");
  std::string err;
  if (!validate_credential(username, password, err)) {
    fail(res, 1001, err, 400);
    return;
  }
  if (!ctx.repo->create_user(username, util::password_hash(password), "author", err)) {
    fail(res, 1002, err, 409);
    return;
  }
  User u;
  ctx.repo->get_user_by_name(username, u);
  ok(res, {{"id", u.id}, {"username", u.username}}, 201);
}

void handle_login(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  json body;
  if (!parse_body(req, res, body)) return;
  std::string username = trim(body.value("username", ""));
  std::string password = body.value("password", "");
  User u;
  if (!ctx.repo->get_user_by_name(username, u) ||
      !util::password_verify(password, u.password)) {
    fail(res, 1003, "用户名或密码错误", 401);
    return;
  }
  json payload = {{"uid", u.id}, {"username", u.username}, {"role", u.role}};
  std::string token = util::jwt_sign(payload, ctx.jwt_secret, ctx.jwt_expire_hours * 3600);
  ok(res, {{"token", token},
           {"user", {{"id", u.id}, {"username", u.username}, {"role", u.role}}}});
}

void handle_article_list(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  int page = std::max(1, std::atoi(req.get_param_value("page").c_str()));
  int size = std::atoi(req.get_param_value("size").c_str());
  if (size <= 0 || size > 50) size = 10;
  std::string tag = req.get_param_value("tag");
  std::string sort = req.get_param_value("sort");
  if (sort != "views") sort = "time";

  // status=all 需要登录（含草稿）；默认只列出已发布
  bool include_all = false;
  if (req.get_param_value("status") == "all") {
    User u;
    if (require_user(req, res, ctx, u)) include_all = true;
    else return;
  }

  std::vector<Article> items;
  int64_t total = 0;
  if (!ctx.repo->list_articles(page, size, tag, sort, include_all, items, total)) {
    fail(res, 500, "查询失败", 500);
    return;
  }
  json arr = json::array();
  for (auto& a : items) {
    json tags = json::array();
    for (auto& t : a.tags) tags.push_back(t);
    arr.push_back({{"id", a.id},
                   {"title", a.title},
                   {"summary", a.summary},
                   {"status", a.status},
                   {"view_count", a.view_count},
                   {"user_id", a.user_id},
                   {"created_at", a.created_at},
                   {"updated_at", a.updated_at},
                   {"tags", tags}});
  }
  ok(res, {{"total", total}, {"page", page}, {"size", size}, {"items", arr}});
}

void handle_article_detail(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  // 路径形如 /api/articles/{id}，正则已捕获数字
  int64_t id = std::stoll(req.matches[1].str());
  Article a;
  if (!ctx.repo->get_article_by_id(id, a, true)) {
    fail(res, 404, "文章不存在", 404);
    return;
  }
  json tags = json::array();
  for (auto& t : a.tags) tags.push_back(t);
  ok(res, {{"id", a.id},
           {"title", a.title},
           {"content", a.content},
           {"summary", a.summary},
           {"cover_img", a.cover_img},
           {"status", a.status},
           {"view_count", a.view_count},
           {"user_id", a.user_id},
           {"created_at", a.created_at},
           {"updated_at", a.updated_at},
           {"tags", tags}});
}

void handle_article_create(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  User u;
  if (!require_user(req, res, ctx, u)) return;
  json body;
  if (!parse_body(req, res, body)) return;

  std::string title = trim(body.value("title", ""));
  std::string content = body.value("content", "");
  std::string summary = trim(body.value("summary", ""));
  std::string status = body.value("status", "draft");
  if (title.empty() || title.size() > 200) {
    fail(res, 1101, "标题长度须为 1-200 字符", 400);
    return;
  }
  if (content.size() > 200000) {
    fail(res, 1102, "正文过长（上限 200KB）", 400);
    return;
  }
  if (!valid_status(status)) {
    fail(res, 1103, "status 只能是 draft 或 published", 400);
    return;
  }
  if (summary.size() > 500) {
    fail(res, 1104, "摘要过长（上限 500 字符）", 400);
    return;
  }
  Article a;
  a.title = title;
  a.content = content;
  a.summary = summary;
  a.status = status;
  a.user_id = u.id;
  if (body.contains("tags") && body["tags"].is_array()) {
    for (auto& t : body["tags"]) {
      std::string name = trim(t.get<std::string>());
      if (!name.empty() && name.size() <= 32) a.tags.push_back(name);
    }
    if (a.tags.size() > 10) {
      fail(res, 1105, "标签最多 10 个", 400);
      return;
    }
  }
  std::string err;
  int64_t id = ctx.repo->create_article(a, err);
  if (id <= 0) {
    fail(res, 500, err.empty() ? "文章创建失败" : err, 500);
    return;
  }
  ok(res, {{"id", id}}, 201);
}

void handle_article_update(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  User u;
  if (!require_user(req, res, ctx, u)) return;
  int64_t id = std::stoll(req.matches[1].str());
  json body;
  if (!parse_body(req, res, body)) return;

  Article a;
  if (!ctx.repo->get_article_by_id(id, a, false)) {
    fail(res, 404, "文章不存在", 404);
    return;
  }
  // 权限：作者本人或管理员
  if (a.user_id != u.id && u.role != "admin") {
    fail(res, 403, "无权修改他人文章", 403);
    return;
  }
  std::string title = trim(body.value("title", a.title));
  std::string content = body.value("content", a.content);
  std::string summary = trim(body.value("summary", a.summary));
  std::string status = body.value("status", a.status);
  if (title.empty() || title.size() > 200) {
    fail(res, 1101, "标题长度须为 1-200 字符", 400);
    return;
  }
  if (content.size() > 200000) {
    fail(res, 1102, "正文过长（上限 200KB）", 400);
    return;
  }
  if (!valid_status(status)) {
    fail(res, 1103, "status 只能是 draft 或 published", 400);
    return;
  }
  a.title = title;
  a.content = content;
  a.summary = summary;
  a.status = status;
  a.tags.clear();
  if (body.contains("tags") && body["tags"].is_array()) {
    for (auto& t : body["tags"]) {
      std::string name = trim(t.get<std::string>());
      if (!name.empty() && name.size() <= 32) a.tags.push_back(name);
    }
  }
  std::string err;
  if (!ctx.repo->update_article(a, err)) {
    fail(res, 500, err.empty() ? "文章更新失败" : err, 500);
    return;
  }
  ok(res, {{"id", id}});
}

void handle_article_delete(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  User u;
  if (!require_user(req, res, ctx, u)) return;
  int64_t id = std::stoll(req.matches[1].str());
  Article a;
  if (!ctx.repo->get_article_by_id(id, a, false)) {
    fail(res, 404, "文章不存在", 404);
    return;
  }
  if (a.user_id != u.id && u.role != "admin") {
    fail(res, 403, "无权删除他人文章", 403);
    return;
  }
  std::string err;
  if (!ctx.repo->delete_article(id, err)) {
    fail(res, 500, err.empty() ? "文章删除失败" : err, 500);
    return;
  }
  ok(res, json::object());
}

void handle_tag_list(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  std::vector<Tag> tags;
  if (!ctx.repo->list_tags(tags)) {
    fail(res, 500, "查询失败", 500);
    return;
  }
  json arr = json::array();
  for (auto& t : tags) {
    arr.push_back({{"id", t.id}, {"name", t.name}, {"created_at", t.created_at}});
  }
  ok(res, arr);
}

void handle_comment_list(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  int64_t article_id = std::atoi(req.get_param_value("article_id").c_str());
  if (article_id <= 0) {
    fail(res, 1201, "缺少 article_id", 400);
    return;
  }
  std::vector<Comment> comments;
  if (!ctx.repo->list_comments(article_id, comments)) {
    fail(res, 500, "查询失败", 500);
    return;
  }
  json arr = json::array();
  for (auto& c : comments) {
    arr.push_back({{"id", c.id},
                   {"article_id", c.article_id},
                   {"user_id", c.user_id},
                   {"username", c.username},
                   {"content", c.content},
                   {"created_at", c.created_at}});
  }
  ok(res, arr);
}

void handle_comment_create(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  User u;
  if (!require_user(req, res, ctx, u)) return;
  json body;
  if (!parse_body(req, res, body)) return;
  int64_t article_id = body.value("article_id", 0LL);
  std::string content = trim(body.value("content", ""));
  if (article_id <= 0) {
    fail(res, 1201, "缺少 article_id", 400);
    return;
  }
  if (content.empty() || content.size() > 2000) {
    fail(res, 1202, "评论长度须为 1-2000 字符", 400);
    return;
  }
  Article a;
  if (!ctx.repo->get_article_by_id(article_id, a, false)) {
    fail(res, 404, "文章不存在", 404);
    return;
  }
  Comment c;
  c.article_id = article_id;
  c.user_id = u.id;
  c.username = u.username;
  c.content = content;
  std::string err;
  if (!ctx.repo->add_comment(c, err)) {
    fail(res, 500, err.empty() ? "评论发布失败" : err, 500);
    return;
  }
  ok(res, json::object(), 201);
}

void handle_stat_overview(const httplib::Request& req, httplib::Response& res, Ctx& ctx) {
  User u;
  if (!require_user(req, res, ctx, u)) return;
  int64_t article_count = 0, published_count = 0, view_total = 0, comment_count = 0;
  if (!ctx.repo->get_stats(article_count, published_count, view_total, comment_count)) {
    fail(res, 500, "统计失败", 500);
    return;
  }
  ok(res, {{"article_count", article_count},
           {"published_count", published_count},
           {"view_total", view_total},
           {"comment_count", comment_count}});
}

}  // namespace

// ---------- 路由注册 ----------

void register_routes(httplib::Server& svr, Ctx& ctx) {
  // CORS：预检直接放行，业务响应统一带跨域头
  svr.set_pre_routing_handler([&ctx](const httplib::Request& req, httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type,Authorization");
    if (req.method == "OPTIONS") {
      res.status = 204;
      return httplib::Server::HandlerResponse::Handled;
    }
    return httplib::Server::HandlerResponse::Unhandled;
  });

  svr.Get("/api/health", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_health(req, res, ctx);
  });

  svr.Post("/api/auth/register", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_register(req, res, ctx);
  });
  svr.Post("/api/auth/login", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_login(req, res, ctx);
  });

  svr.Get("/api/articles", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_article_list(req, res, ctx);
  });
  svr.Get(R"(/api/articles/(\d+))", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_article_detail(req, res, ctx);
  });
  svr.Post("/api/articles", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_article_create(req, res, ctx);
  });
  svr.Put(R"(/api/articles/(\d+))", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_article_update(req, res, ctx);
  });
  svr.Delete(R"(/api/articles/(\d+))", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_article_delete(req, res, ctx);
  });

  svr.Get("/api/tags", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_tag_list(req, res, ctx);
  });

  svr.Get("/api/comments", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_comment_list(req, res, ctx);
  });
  svr.Post("/api/comments", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_comment_create(req, res, ctx);
  });

  svr.Get("/api/stat/overview", [&ctx](const httplib::Request& req, httplib::Response& res) {
    handle_stat_overview(req, res, ctx);
  });
}

}  // namespace blog
