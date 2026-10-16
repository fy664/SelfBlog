#pragma once
// =============================================================
// JWT（JSON Web Token）签名与校验，算法 HS256（HMAC-SHA256）
// 自实现签名与验签，无第三方依赖；payload 使用 nlohmann::json
// 结构：header.payload.signature，三段均为 Base64URL
// 安全性说明：本实现用于教学/求职项目，密钥保存在 config.json；
// 生产环境应将密钥放入环境变量或密钥管理服务。
// =============================================================
#include <nlohmann/json.hpp>

#include <string>

namespace util {

// 生成签名（payload 自动附加 exp = now + expire_seconds；expire_seconds<=0 时不加）
std::string jwt_sign(const nlohmann::json& payload, const std::string& secret,
                     long expire_seconds);

// 校验签名并解析 payload；exp 过期、签名不符返回 false
bool jwt_verify(const std::string& token, const std::string& secret,
                nlohmann::json& payload_out);

}  // namespace util
