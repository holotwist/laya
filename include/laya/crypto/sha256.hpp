#pragma once

#include <string>
#include <string_view>
#include <array>
#include <cstdint>

namespace laya::crypto {

using Hash256 = std::array<uint8_t, 32>;

class Sha256 {
public:
    Sha256();
    void update(const uint8_t* data, size_t length);
    void update(std::string_view data);
    Hash256 finalize();
    static Hash256 hash(std::string_view data);
    static std::string to_hex(const Hash256& hash);

private:
    void transform(const uint8_t* chunk);
    uint32_t state_[8];
    uint64_t count_{0};
    uint8_t buffer_[64];
};

} // namespace laya::crypto