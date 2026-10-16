#include "base64.h"

namespace util {

namespace {

// Base64 字母表
const char B64_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// 反向查表：字符 -> 6 位值；-1 表示非法
inline int b64_index(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

}  // namespace

std::string base64_encode(const std::string& data) {
  std::string out;
  out.reserve(((data.size() + 2) / 3) * 4);
  size_t i = 0;
  // 每 3 字节 -> 4 个字符
  while (i + 3 <= data.size()) {
    uint32_t v = (static_cast<unsigned char>(data[i]) << 16) |
                 (static_cast<unsigned char>(data[i + 1]) << 8) |
                 static_cast<unsigned char>(data[i + 2]);
    out.push_back(B64_TABLE[(v >> 18) & 0x3f]);
    out.push_back(B64_TABLE[(v >> 12) & 0x3f]);
    out.push_back(B64_TABLE[(v >> 6) & 0x3f]);
    out.push_back(B64_TABLE[v & 0x3f]);
    i += 3;
  }
  // 尾部不足 3 字节：补 = 对齐
  size_t remain = data.size() - i;
  if (remain == 1) {
    uint32_t v = static_cast<unsigned char>(data[i]) << 16;
    out.push_back(B64_TABLE[(v >> 18) & 0x3f]);
    out.push_back(B64_TABLE[(v >> 12) & 0x3f]);
    out.push_back('=');
    out.push_back('=');
  } else if (remain == 2) {
    uint32_t v = (static_cast<unsigned char>(data[i]) << 16) |
                 (static_cast<unsigned char>(data[i + 1]) << 8);
    out.push_back(B64_TABLE[(v >> 18) & 0x3f]);
    out.push_back(B64_TABLE[(v >> 12) & 0x3f]);
    out.push_back(B64_TABLE[(v >> 6) & 0x3f]);
    out.push_back('=');
  }
  return out;
}

std::string base64_decode(const std::string& in) {
  std::string out;
  out.reserve((in.size() / 4) * 3);
  uint32_t acc = 0;
  int bits = 0;
  for (char c : in) {
    if (c == '\n' || c == '\r' || c == ' ') continue;
    if (c == '=') break;  // 填充结束
    int idx = b64_index(c);
    if (idx < 0) return std::string();  // 非法字符，解码失败
    acc = (acc << 6) | static_cast<uint32_t>(idx);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out.push_back(static_cast<char>((acc >> bits) & 0xff));
    }
  }
  return out;
}

std::string base64url_encode(const std::string& data) {
  std::string out = base64_encode(data);
  // URL 安全替换
  for (char& c : out) {
    if (c == '+') c = '-';
    if (c == '/') c = '_';
  }
  // 去掉末尾 '='
  while (!out.empty() && out.back() == '=') out.pop_back();
  return out;
}

std::string base64url_decode(const std::string& in) {
  // 反转 URL 安全字符
  std::string s = in;
  for (char& c : s) {
    if (c == '-') c = '+';
    if (c == '_') c = '/';
  }
  // 补齐 '=' 使长度是 4 的倍数
  while (s.size() % 4 != 0) s.push_back('=');
  return base64_decode(s);
}

}  // namespace util
