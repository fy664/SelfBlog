#include "logger.h"

#include "misc.h"  // util::now_iso()（同目录）

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <direct.h>  // _mkdir
#else
#include <sys/stat.h>  // mkdir
#endif

namespace blog {
namespace logger {

namespace {

std::mutex g_mtx;
std::string g_dir = "logs";
bool g_inited = false;

// 确保日志目录存在（不存在则创建，失败时静默忽略）
void ensure_dir(const std::string& dir) {
#ifdef _WIN32
  _mkdir(dir.c_str());
#else
  mkdir(dir.c_str(), 0755);
#endif
}

std::string level_str(Level l) {
  switch (l) {
    case LV_INFO: return "INFO";
    case LV_WARN: return "WARN";
    default:      return "ERROR";
  }
}

// 当前日期字符串 YYYYMMDD（本地时间）
std::string date_str() {
  std::time_t t = std::time(nullptr);
  std::tm tmv{};
#ifdef _WIN32
  localtime_s(&tmv, &t);
#else
  localtime_r(&t, &tmv);
#endif
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d%02d%02d",
                tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
  return buf;
}

}  // namespace

void init(const std::string& dir) {
  std::lock_guard<std::mutex> lock(g_mtx);
  g_dir = dir;
  g_inited = true;
  ensure_dir(g_dir);
}

void info(const std::string& msg) { write(LV_INFO, msg); }
void warn(const std::string& msg) { write(LV_WARN, msg); }
void error(const std::string& msg) { write(LV_ERROR, msg); }

void write(Level level, const std::string& msg) {
  std::lock_guard<std::mutex> lock(g_mtx);
  if (!g_inited) return;
  ensure_dir(g_dir);
  std::string path = g_dir + "/backend-" + date_str() + ".log";
  std::ofstream f(path, std::ios::app);
  if (!f) return;
  f << "[" << util::now_iso() << "] [" << level_str(level) << "] " << msg << "\n";
  f.flush();
}

}  // namespace logger
}  // namespace blog
