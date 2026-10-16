#include "jwt.h"

#include "base64.h"
#include "sha256.h"

#include <cstdint>
#include <ctime>

namespace util {

namespace {

// HMAC-SHA256：返回 32 字节二进制摘要
std::string hmac_sha256(const std::string& key, const std::string& msg) {
  std::string k = key;
  // 密钥超过 64 字节时先做 SHA-256 压缩
  if (k.size() > 64) k = sha256(k);
  // ipad / opad
  std::string ipad(64, static_cast<char>(0x36));
  std::string opad(64, static_cast<char>(0x5c));
  for (size_t i = 0; i < k.size(); ++i) {
    ipad[i] ^= k[i];
    opad[i] ^= k[i];
  }
  return sha256(opad + sha256(ipad + msg));
}

// 常量时间比较，避免时序侧信道
bool constant_time_equal(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;
  unsigned char diff = 0;
  for (size_t i = 0; i < a.size(); ++i) diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
  return diff == 0;
}

}  // namespace

std::string jwt_sign(const nlohmann::json& payload, const std::string& secret,
                     long expire_seconds) {
  // 1) 头部：算法 HS256，类型 JWT
  nlohmann::json header = {{"alg", "HS256"}, {"typ", "JWT"}};
  // 2) 载荷：附加过期时间
  nlohmann::json body = payload;
  if (expire_seconds > 0 && !body.contains("exp")) {
    body["exp"] = static_cast<long long>(std::time(nullptr)) + expire_seconds;
  }
  // 3) 三段拼接
  std::string h = base64url_encode(header.dump());
  std::string b = base64url_encode(body.dump());
  std::string signing_input = h + "." + b;
  std::string sig = base64url_encode(hmac_sha256(secret, signing_input));
  return signing_input + "." + sig;
}

bool jwt_verify(const std::string& token, const std::string& secret,
                nlohmann::json& payload_out) {
  // 1) 按 '.' 分成三段
  auto p1 = token.find('.');
  auto p2 = token.rfind('.');
  if (p1 == std::string::npos || p2 == std::string::npos || p1 == p2) return false;
  std::string header_b64 = token.substr(0, p1);
  std::string payload_b64 = token.substr(p1 + 1, p2 - p1 - 1);
  std::string sig_b64 = token.substr(p2 + 1);
  if (header_b64.empty() || payload_b64.empty() || sig_b64.empty()) return false;

  // 2) 重新计算签名并常量时间比较
  std::string expect = base64url_encode(hmac_sha256(secret, header_b64 + "." + payload_b64));
  if (!constant_time_equal(expect, sig_b64)) return false;

  // 3) 解析载荷
  try {
    payload_out = nlohmann::json::parse(base64url_decode(payload_b64));
  } catch (...) {
    return false;
  }
  if (!payload_out.is_object()) return false;

  // 4) 过期校验
  if (payload_out.contains("exp") && payload_out["exp"].is_number_integer()) {
    long long exp = payload_out["exp"].get<long long>();
    if (exp < static_cast<long long>(std::time(nullptr))) return false;
  }
  return true;
}

}  // namespace util
