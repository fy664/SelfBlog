#pragma once
// =============================================================
// 通用工具：时间、随机数与密码散列
// =============================================================
#include <string>

namespace util {

// 当前 UTC 时间，ISO8601 格式：2026-10-16T12:00:00Z
std::string now_iso();

// 生成 16 字节随机数（十六进制，32 字符），用于密码加盐
// 说明：使用 std::random_device；生产环境建议改用 CSPRNG
std::string random_hex_salt();

// 密码散列：格式 "盐$SHA256hex(盐+密码)"
std::string password_hash(const std::string& password);

// 校验密码：返回 true 表示与存储散列匹配
bool password_verify(const std::string& password, const std::string& stored);

}  // namespace util
