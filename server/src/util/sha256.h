#pragma once
// =============================================================
// SHA-256 摘要（自实现，无第三方依赖）
// 对外提供两个接口：
//   sha256()      -> 32 字节二进制摘要（供 HMAC 使用）
//   sha256_hex()  -> 64 位十六进制小写字符串（供存储/展示）
// 内部实现为标准 FIPS 180-4 算法，包含 64 轮压缩函数。
// =============================================================
#include <string>

namespace util {

// 计算二进制 SHA-256 摘要（32 字节）
std::string sha256(const std::string& data);

// 计算十六进制 SHA-256 摘要（64 字符小写）
std::string sha256_hex(const std::string& data);

// 自检：与已知测试向量比对，用于启动时验证实现正确性
bool sha256_self_test();

}  // namespace util
