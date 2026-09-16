#pragma once

#include <chrono>
#include <string>
#include <optional>
#include <cstdint>

namespace laya::core {

    using Milliseconds = std::chrono::milliseconds;

    // Converts milliseconds to [mm:ss.xx] string
    std::string format_timestamp(Milliseconds ms);

    // Parses [mm:ss.xx] or [mm:ss.xxx] to Milliseconds
    std::optional<Milliseconds> parse_timestamp(std::string_view str);

} // namespace laya::core