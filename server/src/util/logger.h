#pragma once
// =============================================================
// 后端日志模块：按天滚动的文件日志
//   - 日志目录：main.cpp 调用 logger::init() 指定（默认 ./logs）
//   - 文件名：backend-YYYYMMDD.log
//   - 级别：INFO / WARN / ERROR（低于阈值的行会被丢弃）
//   - 线程安全：互斥锁串行写入，多线程请求可安全记日志
// =============================================================
#include <mutex>
#include <string>

namespace blog {
namespace logger {

// 初始化日志系统（目录不存在会自动创建）；进程启动时调用一次
void init(const std::string& dir);

// 级别（注意：勿用 ERROR/INFO 作枚举名，Windows SDK 有同名宏）
enum Level { LV_INFO = 0, LV_WARN = 1, LV_ERROR = 2 };

// 写一条日志（自动带时间戳与级别前缀）
void write(Level level, const std::string& msg);

// 便捷入口（定义在 logger.cpp）
void info(const std::string& msg);
void warn(const std::string& msg);
void error(const std::string& msg);

}  // namespace logger
}  // namespace blog
