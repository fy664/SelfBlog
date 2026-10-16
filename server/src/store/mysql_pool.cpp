#include "mysql_pool.h"

#ifdef HAVE_MYSQL

namespace blog {

MySqlPool::MySqlPool(const Config& cfg) : cfg_(cfg) {
  for (int i = 0; i < cfg_.pool_size; ++i) create_one();
}

MySqlPool::~MySqlPool() {
  std::lock_guard<std::mutex> lock(mtx_);
  for (MYSQL* c : free_) mysql_close(c);
  free_.clear();
}

void MySqlPool::create_one() {
  MYSQL* c = mysql_init(nullptr);
  if (!c) return;
  if (!mysql_real_connect(c, cfg_.host.c_str(), cfg_.user.c_str(),
                          cfg_.password.c_str(), cfg_.database.c_str(),
                          cfg_.port, nullptr, 0)) {
    fprintf(stderr, "[MySqlPool] 连接失败: %s\n", mysql_error(c));
    mysql_close(c);
    return;
  }
  // 连接级 UTF-8
  mysql_set_character_set(c, "utf8mb4");
  free_.push_back(c);
}

MYSQL* MySqlPool::acquire() {
  std::unique_lock<std::mutex> lock(mtx_);
  cv_.wait(lock, [this] { return !free_.empty(); });
  MYSQL* c = free_.front();
  free_.pop_front();
  return c;
}

void MySqlPool::release(MYSQL* c) {
  if (!c) return;
  std::lock_guard<std::mutex> lock(mtx_);
  // 探活：失败则关掉并重建
  if (mysql_ping(c) != 0) {
    mysql_close(c);
    create_one();
  } else {
    free_.push_back(c);
  }
  cv_.notify_one();
}

}  // namespace blog

#endif  // HAVE_MYSQL
