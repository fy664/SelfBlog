#pragma once
// =============================================================
// HTTP 路由与业务处理器
// 统一响应格式：{ "code": 0, "msg": "ok", "data": ... }
//   code=0 成功；非 0 为业务错误码
// 鉴权：Authorization: Bearer <JWT>
// =============================================================
#include "config.h"
#include "store/store.h"

#include <httplib.h>

#include <string>

namespace blog {

// 处理器共享上下文
struct Ctx {
  Repository* repo = nullptr;
  std::string jwt_secret;
  long jwt_expire_hours = 72;
};

// 注册全部路由（含 CORS 预检处理）
void register_routes(httplib::Server& svr, Ctx& ctx);

}  // namespace blog
