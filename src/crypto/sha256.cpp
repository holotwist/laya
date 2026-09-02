#include "laya/crypto/sha256.hpp"
#include <cstring>
#include <iomanip>
#include <sstream>

namespace laya::crypto {

namespace {
inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t sigma0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t sigma1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t gamma0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint32_t gamma1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};
} // namespace

Sha256::Sha256() {
    state_[0] = 0x6a09e667; state_[1] = 0xbb67ae85; state_[2] = 0x3c6ef372; state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f; state_[5] = 0x9b05688c; state_[6] = 0x1f83d9ab; state_[7] = 0x5be0cd19;
}

void Sha256::transform(const uint8_t* chunk) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(chunk[i * 4]) << 24) |
               (static_cast<uint32_t>(chunk[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(chunk[i * 4 + 2]) << 8) |
               (static_cast<uint32_t>(chunk[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
    }

    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
        uint32_t t2 = sigma0(a) + maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
    state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
}

void Sha256::update(const uint8_t* data, size_t length) {
    size_t buffer_idx = static_cast<size_t>(count_ % 64);
    count_ += length;

    size_t i = 0;
    if (buffer_idx > 0) {
        size_t needed = 64 - buffer_idx;
        if (length < needed) {
            std::memcpy(&buffer_[buffer_idx], data, length);
            return;
        }
        std::memcpy(&buffer_[buffer_idx], data, needed);
        transform(buffer_);
        i += needed;
    }

    for (; i + 63 < length; i += 64) {
        transform(&data[i]);
    }

    if (i < length) {
        std::memcpy(buffer_, &data[i], length - i);
    }
}

void Sha256::update(std::string_view data) {
    update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

Hash256 Sha256::finalize() {
    uint8_t pad = 0x80;
    update(&pad, 1);

    uint8_t zero = 0x00;
    while ((count_ % 64) != 56) {
        update(&zero, 1);
    }

    uint64_t bit_len = (count_ - 1 - ((count_ - 1) % 64 >= 56 ? 64 - (count_ - 1) % 64 + 56 : 56 - (count_ - 1) % 64)) * 8;
    bit_len = (count_ - 1) * 8; // Adjust bit length

    uint8_t len_bytes[8];
    for (int i = 0; i < 8; ++i) {
        len_bytes[7 - i] = static_cast<uint8_t>(bit_len >> (i * 8));
    }
    update(len_bytes, 8);

    Hash256 out;
    for (int i = 0; i < 8; ++i) {
        out[i * 4]     = static_cast<uint8_t>(state_[i] >> 24);
        out[i * 4 + 1] = static_cast<uint8_t>(state_[i] >> 16);
        out[i * 4 + 2] = static_cast<uint8_t>(state_[i] >> 8);
        out[i * 4 + 3] = static_cast<uint8_t>(state_[i]);
    }
    return out;
}

Hash256 Sha256::hash(std::string_view data) {
    Sha256 ctx;
    ctx.update(data);
    return ctx.finalize();
}

std::string Sha256::to_hex(const Hash256& hash) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : hash) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

} // namespace laya::crypto