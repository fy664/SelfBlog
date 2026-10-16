#include "misc.h"

#include "sha256.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <random>

namespace util {

std::string now_iso() {
  std::time_t t = std::time(nullptr);
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif
  char buf[32] = {0};
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buf);
}

std::string random_hex_salt() {
  const char* hex = "0123456789abcdef";
  std::random_device rd;
  std::string out;
  out.reserve(32);
  for (int i = 0; i < 16; ++i) {
    unsigned int v = rd() & 0xff;
    out.push_back(hex[v >> 4]);
    out.push_back(hex[v & 0x0f]);
  }
  return out;
}

std::string password_hash(const std::string& password) {
  std::string salt = random_hex_salt();
  return salt + "$" + sha256_hex(salt + password);
}

bool password_verify(const std::string& password, const std::string& stored) {
  // stored 格式："盐$hex"
  auto pos = stored.find('$');
  if (pos == std::string::npos) return false;
  std::string salt = stored.substr(0, pos);
  std::string expect = stored.substr(pos + 1);
  if (expect.size() != 64) return false;
  return expect == sha256_hex(salt + password);
}

}  // namespace util
