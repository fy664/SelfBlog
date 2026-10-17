#include "json_store.h"

#include "../util/misc.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>

namespace blog {

namespace {
const char* kUsers = "users.json";
const char* kArticles = "articles.json";
const char* kTags = "tags.json";
const char* kComments = "comments.json";
const char* kLoginLogs = "login_logs.json";
}  // namespace

JsonStore::JsonStore(const std::string& data_dir) : dir_(data_dir) {
  // 确保数据目录存在
  std::error_code ec;
  std::filesystem::create_directories(dir_, ec);
}

// ---------- 通用文件读写 ----------

nlohmann::json JsonStore::load(const std::string& name) {
  std::ifstream in(dir_ + "/" + name, std::ios::binary);
  nlohmann::json data = nlohmann::json::array();
  if (in) {
    try {
      in >> data;
    } catch (...) {
      data = nlohmann::json::array();  // 文件损坏则重置（开发模式容错）
    }
  }
  if (!data.is_array()) data = nlohmann::json::array();
  return data;
}

void JsonStore::save(const std::string& name, const nlohmann::json& data) {
  std::ofstream out(dir_ + "/" + name, std::ios::binary | std::ios::trunc);
  out << data.dump();
}

// ---------- 用户 ----------

bool JsonStore::create_user(const std::string& username,
                            const std::string& password,
                            const std::string& role,
                            const std::string& register_ip,
                            const std::string& device_id,
                            std::string& err) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto users = load(kUsers);
  for (auto& u : users) {
    if (u["username"].get<std::string>() == username) {
      err = "用户名已存在";
      return false;
    }
  }
  int64_t id = 1;
  for (auto& u : users) id = std::max(id, u["id"].get<int64_t>() + 1);
  users.push_back({{"id", id},
                   {"username", username},
                   {"password", password},
                   {"avatar", nullptr},
                   {"role", role},
                   {"register_ip", register_ip},
                   {"device_id", device_id},
                   {"created_at", util::now_iso()}});
  save(kUsers, users);
  return true;
}

bool JsonStore::user_device_exists(const std::string& device_id) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (device_id.empty()) return false;
  auto users = load(kUsers);
  for (auto& u : users) {
    if (u.value("device_id", "") == device_id) return true;
  }
  return false;
}

bool JsonStore::list_users(std::vector<User>& out) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto users = load(kUsers);
  for (auto& u : users) {
    User x;
    x.id = u["id"].get<int64_t>();
    x.username = u["username"].get<std::string>();
    x.role = u.value("role", "author");
    x.register_ip = u.value("register_ip", "");
    x.device_id = u.value("device_id", "");
    x.created_at = u.value("created_at", "");
    out.push_back(x);
  }
  return true;
}

bool JsonStore::get_user_by_name(const std::string& username, User& out) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto users = load(kUsers);
  for (auto& u : users) {
    if (u["username"].get<std::string>() == username) {
      out.id = u["id"].get<int64_t>();
      out.username = u["username"].get<std::string>();
      out.password = u["password"].get<std::string>();
      out.role = u["role"].get<std::string>();
      out.register_ip = u.value("register_ip", "");
      out.device_id = u.value("device_id", "");
      out.created_at = u["created_at"].get<std::string>();
      return true;
    }
  }
  return false;
}

bool JsonStore::get_user_by_id(int64_t id, User& out) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto users = load(kUsers);
  for (auto& u : users) {
    if (u["id"].get<int64_t>() == id) {
      out.id = id;
      out.username = u["username"].get<std::string>();
      out.password = u["password"].get<std::string>();
      out.role = u["role"].get<std::string>();
      out.register_ip = u.value("register_ip", "");
      out.device_id = u.value("device_id", "");
      out.created_at = u["created_at"].get<std::string>();
      return true;
    }
  }
  return false;
}

// ---------- 文章 ----------

int64_t JsonStore::create_article(const Article& a, std::string& err) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto articles = load(kArticles);
  int64_t id = 1;
  for (auto& art : articles) id = std::max(id, art["id"].get<int64_t>() + 1);
  std::string now = util::now_iso();
  nlohmann::json tags = nlohmann::json::array();
  for (const auto& t : a.tags) tags.push_back(t);
  articles.push_back({{"id", id},
                      {"title", a.title},
                      {"content", a.content},
                      {"summary", a.summary},
                      {"cover_img", a.cover_img},
                      {"status", a.status},
                      {"view_count", 0},
                      {"user_id", a.user_id},
                      {"created_at", now},
                      {"updated_at", now},
                      {"tags", tags}});
  save(kArticles, articles);

  // 同步维护标签表（tags.json）
  auto tags_list = load(kTags);
  for (const auto& t : a.tags) {
    bool exists = false;
    for (auto& tg : tags_list)
      if (tg["name"].get<std::string>() == t) { exists = true; break; }
    if (!exists) {
      int64_t tid = 1;
      for (auto& tg : tags_list) tid = std::max(tid, tg["id"].get<int64_t>() + 1);
      tags_list.push_back({{"id", tid}, {"name", t}, {"created_at", util::now_iso()}});
    }
  }
  save(kTags, tags_list);
  return id;
}

bool JsonStore::get_article_by_id(int64_t id, Article& out, bool increment_view) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto articles = load(kArticles);
  for (auto& art : articles) {
    if (art["id"].get<int64_t>() == id) {
      if (increment_view) {
        art["view_count"] = art["view_count"].get<int64_t>() + 1;
        save(kArticles, articles);
      }
      out.id = id;
      out.title = art["title"].get<std::string>();
      out.content = art["content"].get<std::string>();
      out.summary = art.value("summary", "");
      out.cover_img = art.value("cover_img", "");
      out.status = art["status"].get<std::string>();
      out.view_count = art["view_count"].get<int64_t>();
      out.user_id = art["user_id"].get<int64_t>();
      out.created_at = art["created_at"].get<std::string>();
      out.updated_at = art.value("updated_at", art["created_at"].get<std::string>());
      if (art.contains("tags") && art["tags"].is_array())
        for (auto& t : art["tags"]) out.tags.push_back(t.get<std::string>());
      return true;
    }
  }
  return false;
}

bool JsonStore::list_articles(int page, int size, const std::string& tag,
                              const std::string& sort, bool include_all,
                              int64_t owner_id,
                              std::vector<Article>& out, int64_t& total) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto articles = load(kArticles);

  // 1) 过滤
  std::vector<nlohmann::json> filtered;
  for (auto& art : articles) {
    std::string status = art["status"].get<std::string>();
    if (!include_all && status != "published") continue;
    // 普通用户管理列表：只看自己的文章（含草稿）；admin 传 0 看全部
    if (owner_id > 0 && art["user_id"].get<int64_t>() != owner_id) continue;
    if (!tag.empty()) {
      bool hit = false;
      if (art.contains("tags") && art["tags"].is_array())
        for (auto& t : art["tags"])
          if (t.get<std::string>() == tag) { hit = true; break; }
      if (!hit) continue;
    }
    filtered.push_back(art);
  }

  // 2) 排序：time=按时间倒序（默认）；views=按浏览量倒序
  if (sort == "views") {
    std::sort(filtered.begin(), filtered.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
      return a["view_count"].get<int64_t>() > b["view_count"].get<int64_t>();
    });
  } else {
    std::sort(filtered.begin(), filtered.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
      return a["created_at"].get<std::string>() > b["created_at"].get<std::string>();
    });
  }

  total = static_cast<int64_t>(filtered.size());

  // 3) 分页
  size_t start = static_cast<size_t>((page - 1) * size);
  for (size_t i = start; i < filtered.size() && out.size() < static_cast<size_t>(size); ++i) {
    Article a;
    a.id = filtered[i]["id"].get<int64_t>();
    a.title = filtered[i]["title"].get<std::string>();
    a.content = filtered[i]["content"].get<std::string>();
    a.summary = filtered[i].value("summary", "");
    a.status = filtered[i]["status"].get<std::string>();
    a.view_count = filtered[i]["view_count"].get<int64_t>();
    a.user_id = filtered[i]["user_id"].get<int64_t>();
    a.created_at = filtered[i]["created_at"].get<std::string>();
    a.updated_at = filtered[i].value("updated_at", a.created_at);
    if (filtered[i].contains("tags") && filtered[i]["tags"].is_array())
      for (auto& t : filtered[i]["tags"]) a.tags.push_back(t.get<std::string>());
    out.push_back(a);
  }
  return true;
}

bool JsonStore::update_article(const Article& a, std::string& err) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto articles = load(kArticles);
  for (auto& art : articles) {
    if (art["id"].get<int64_t>() == a.id) {
      art["title"] = a.title;
      art["content"] = a.content;
      art["summary"] = a.summary;
      art["cover_img"] = a.cover_img;
      art["status"] = a.status;
      art["updated_at"] = util::now_iso();
      nlohmann::json tags = nlohmann::json::array();
      for (const auto& t : a.tags) tags.push_back(t);
      art["tags"] = tags;
      save(kArticles, articles);
      return true;
    }
  }
  err = "文章不存在";
  return false;
}

bool JsonStore::delete_article(int64_t id, std::string& err) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto articles = load(kArticles);
  bool found = false;
  nlohmann::json keep = nlohmann::json::array();
  for (auto& art : articles) {
    if (art["id"].get<int64_t>() == id) {
      found = true;
      continue;
    }
    keep.push_back(art);
  }
  if (!found) {
    err = "文章不存在";
    return false;
  }
  save(kArticles, keep);

  // 级联删除该文章下的评论
  auto comments = load(kComments);
  nlohmann::json ckeep = nlohmann::json::array();
  for (auto& c : comments)
    if (c["article_id"].get<int64_t>() != id) ckeep.push_back(c);
  save(kComments, ckeep);
  return true;
}

// ---------- 标签 ----------

bool JsonStore::list_tags(std::vector<Tag>& out) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto tags_list = load(kTags);
  for (auto& t : tags_list) {
    Tag tg;
    tg.id = t["id"].get<int64_t>();
    tg.name = t["name"].get<std::string>();
    tg.created_at = t["created_at"].get<std::string>();
    out.push_back(tg);
  }
  return true;
}

// ---------- 评论 ----------

bool JsonStore::add_comment(const Comment& c, std::string& err) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto comments = load(kComments);
  int64_t id = 1;
  for (auto& cm : comments) id = std::max(id, cm["id"].get<int64_t>() + 1);
  comments.push_back({{"id", id},
                      {"article_id", c.article_id},
                      {"user_id", c.user_id},
                      {"username", c.username},
                      {"content", c.content},
                      {"created_at", util::now_iso()}});
  save(kComments, comments);
  return true;
}

bool JsonStore::list_comments(int64_t article_id, std::vector<Comment>& out) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto comments = load(kComments);
  for (auto& cm : comments) {
    if (cm["article_id"].get<int64_t>() != article_id) continue;
    Comment c;
    c.id = cm["id"].get<int64_t>();
    c.article_id = article_id;
    c.user_id = cm["user_id"].get<int64_t>();
    c.username = cm.value("username", "");
    c.content = cm["content"].get<std::string>();
    c.created_at = cm["created_at"].get<std::string>();
    out.push_back(c);
  }
  // 按时间正序
  std::sort(out.begin(), out.end(), [](const Comment& a, const Comment& b) {
    return a.created_at < b.created_at;
  });
  return true;
}

bool JsonStore::list_comments_by_user(int64_t user_id, std::vector<Comment>& out) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto comments = load(kComments);
  for (auto& cm : comments) {
    if (cm["user_id"].get<int64_t>() != user_id) continue;
    Comment c;
    c.id = cm["id"].get<int64_t>();
    c.article_id = cm["article_id"].get<int64_t>();
    c.user_id = user_id;
    c.username = cm.value("username", "");
    c.content = cm["content"].get<std::string>();
    c.created_at = cm["created_at"].get<std::string>();
    out.push_back(c);
  }
  // 按时间倒序（最新在前）
  std::sort(out.begin(), out.end(), [](const Comment& a, const Comment& b) {
    return a.created_at > b.created_at;
  });
  return true;
}

bool JsonStore::get_comment_by_id(int64_t id, Comment& out) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto comments = load(kComments);
  for (auto& cm : comments) {
    if (cm["id"].get<int64_t>() != id) continue;
    out.id = cm["id"].get<int64_t>();
    out.article_id = cm["article_id"].get<int64_t>();
    out.user_id = cm["user_id"].get<int64_t>();
    out.username = cm.value("username", "");
    out.content = cm["content"].get<std::string>();
    out.created_at = cm["created_at"].get<std::string>();
    return true;
  }
  return false;
}

bool JsonStore::delete_comment(int64_t id, std::string& err) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto comments = load(kComments);
  bool found = false;
  nlohmann::json keep = nlohmann::json::array();
  for (auto& cm : comments) {
    if (cm["id"].get<int64_t>() == id) {
      found = true;
      continue;
    }
    keep.push_back(cm);
  }
  if (!found) {
    err = "评论不存在";
    return false;
  }
  save(kComments, keep);
  return true;
}

// ---------- 登录日志 ----------

bool JsonStore::record_login(const std::string& username, const std::string& ip,
                             const std::string& user_agent, bool success) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto logs = load(kLoginLogs);
  int64_t id = 1;
  for (auto& lg : logs) id = std::max(id, lg["id"].get<int64_t>() + 1);
  logs.push_back({{"id", id},
                  {"username", username},
                  {"ip", ip},
                  {"user_agent", user_agent},
                  {"success", success},
                  {"created_at", util::now_iso()}});
  save(kLoginLogs, logs);
  return true;
}

bool JsonStore::list_login_logs(int limit, std::vector<LoginLog>& out) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto logs = load(kLoginLogs);
  // 按 id 倒序取最近 limit 条
  for (int64_t i = static_cast<int64_t>(logs.size()) - 1;
       i >= 0 && out.size() < static_cast<size_t>(limit); --i) {
    LoginLog l;
    l.id = logs[i]["id"].get<int64_t>();
    l.username = logs[i].value("username", "");
    l.ip = logs[i].value("ip", "");
    l.user_agent = logs[i].value("user_agent", "");
    l.success = logs[i].value("success", false);
    l.created_at = logs[i].value("created_at", "");
    out.push_back(l);
  }
  return true;
}

// ---------- 统计 ----------

bool JsonStore::get_stats(int64_t& article_count, int64_t& published_count,
                          int64_t& view_total, int64_t& comment_count) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto articles = load(kArticles);
  auto comments = load(kComments);
  article_count = static_cast<int64_t>(articles.size());
  published_count = 0;
  view_total = 0;
  for (auto& art : articles) {
    if (art["status"].get<std::string>() == "published") ++published_count;
    view_total += art["view_count"].get<int64_t>();
  }
  comment_count = static_cast<int64_t>(comments.size());
  return true;
}

}  // namespace blog
