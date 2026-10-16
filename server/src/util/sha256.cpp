#include "sha256.h"

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

namespace util {

namespace {

// 64 轮压缩函数使用的常量 K[0..63]，前 80 个素数立方根的小数部分取前 32 位
const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

// 32 位循环右移
inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

// 单个 64 字节块的处理（消息调度 + 64 轮压缩）
void process_block(const unsigned char* block, uint32_t h[8]) {
  uint32_t w[64];
  // 1) 把 16 个 4 字节大端整数载入 W[0..15]
  for (int j = 0; j < 16; ++j) {
    uint32_t v = 0;
    for (int k = 0; k < 4; ++k)
      v = (v << 8) | block[j * 4 + k];
    w[j] = v;
  }
  // 2) 扩展出 W[16..63]
  for (int j = 16; j < 64; ++j) {
    uint32_t s0 = rotr(w[j - 15], 7) ^ rotr(w[j - 15], 18) ^ (w[j - 15] >> 3);
    uint32_t s1 = rotr(w[j - 2], 17) ^ rotr(w[j - 2], 19) ^ (w[j - 2] >> 10);
    w[j] = w[j - 16] + s0 + w[j - 7] + s1;
  }
  // 3) 初始化工作变量
  uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
  uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
  // 4) 64 轮压缩
  for (int j = 0; j < 64; ++j) {
    uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    uint32_t ch = (e & f) ^ (~e & g);
    uint32_t temp1 = hh + S1 + ch + K[j] + w[j];
    uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    uint32_t temp2 = S0 + maj;
    hh = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a;
    a = temp1 + temp2;
  }
  // 5) 累加到哈希状态
  h[0] += a; h[1] += b; h[2] += c; h[3] += d;
  h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

}  // namespace

std::string sha256(const std::string& data) {
  // 初始哈希值：前 8 个素数平方根的小数部分取前 32 位
  uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                   0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

  // 1) 填充：补 0x80，再补 0 到长度 mod 64 == 56，最后 8 字节写原长度（大端）
  std::string buf = data;
  buf.push_back(static_cast<char>(0x80));
  while (buf.size() % 64 != 56) buf.push_back('\0');
  // 原数据总位数（按 64 位大端写入；本场景数据远小于 2^61 字节）
  uint64_t bit_len = static_cast<uint64_t>(data.size()) * 8ULL;
  for (int i = 7; i >= 0; --i) buf.push_back(static_cast<char>((bit_len >> (i * 8)) & 0xff));

  // 2) 逐块处理
  for (size_t i = 0; i < buf.size(); i += 64)
    process_block(reinterpret_cast<const unsigned char*>(buf.data()) + i, h);

  // 3) 输出 32 字节大端摘要
  std::string out;
  out.reserve(32);
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<char>((h[i] >> 24) & 0xff));
    out.push_back(static_cast<char>((h[i] >> 16) & 0xff));
    out.push_back(static_cast<char>((h[i] >> 8) & 0xff));
    out.push_back(static_cast<char>(h[i] & 0xff));
  }
  return out;
}

std::string sha256_hex(const std::string& data) {
  std::string digest = sha256(data);
  std::ostringstream os;
  os << std::hex << std::setfill('0');
  for (unsigned char c : digest) os << std::setw(2) << static_cast<int>(c);
  return os.str();
}

bool sha256_self_test() {
  // FIPS 180-4 附录 B 的官方测试向量
  static const char* vec_empty = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
  static const char* vec_abc = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
  return sha256_hex("") == vec_empty && sha256_hex("abc") == vec_abc;
}

}  // namespace util
