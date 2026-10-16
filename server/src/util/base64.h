#pragma once
// =============================================================
// Base64 / Base64URL 编解码（自实现，无第三方依赖）
// Base64URL：RFC 4648 §5，把 + / 替换为 - _，去掉末尾 = 填充
// =============================================================
#include <string>

namespace util {

// 标准 Base64 编码
std::string base64_encode(const std::string& data);

// 标准 Base64 解码（自动跳过空白字符，非法输入返回空串）
std::string base64_decode(const std::string& in);

// Base64URL 编码（JWT 签名段使用）
std::string base64url_encode(const std::string& data);

// Base64URL 解码（自动补齐 = 填充）
std::string base64url_decode(const std::string& in);

}  // namespace util
