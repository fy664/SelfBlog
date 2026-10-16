#pragma once
// =============================================================
// MySQL 连接池（仅 HAVE_MYSQL 编译时启用）
// 设计要点（面试可讲）：
//   1. 固定大小连接池：构造函数一次性建立 pool_size 条连接
//   2. 获取/归还：互斥锁 + 条件变量；池空时阻塞等待（避免突发建连）
//   3. 归还时用 mysql_ping 探活，失效连接自动重建
//   4. RAII（Lease）确保异常路径也能归还连接
// =============================================================
#ifdef HAVE_MYSQL

#include <mysql.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>

namespace blog {

class MySqlPool {
 public:
  struct Config {
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    int port = 3306;
    int pool_size = 8;
  };

  explicit MySqlPool(const Config& cfg);
  ~MySqlPool();

  // 阻塞获取一条连接；失败返回 nullptr
  MYSQL* acquire();

  // 归还连接（内部探活，失效则重建）
  void release(MYSQL* c);

  // RAII 连接租约：析构时自动归还
  class Lease {
   public:
    Lease(MySqlPool* pool, MYSQL* conn) : pool_(pool), conn_(conn) {}
    ~Lease() { if (pool_ && conn_) pool_->release(conn_); }
    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;
    MYSQL* get() const { return conn_; }
    MYSQL* operator->() const { return conn_; }

   private:
    MySqlPool* pool_;
    MYSQL* conn_;
  };

 private:
  void create_one();  // 新建一条连接并放入空闲队列

  Config cfg_;
  std::deque<MYSQL*> free_;
  std::mutex mtx_;
  std::condition_variable cv_;
};

}  // namespace blog

#endif  // HAVE_MYSQL
