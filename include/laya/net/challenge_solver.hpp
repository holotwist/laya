#pragma once

#include <string>
#include <string_view>
#include <atomic>
#include <optional>
#include <functional>

namespace laya::net {

class ChallengeSolver {
public:
    using ProgressCallback = std::function<void(uint64_t hashes_computed)>;

    // Solves the PoW challenge using available CPU hardware threads
    static std::optional<std::string> solve(
        std::string_view prefix, 
        std::string_view target_hex,
        ProgressCallback on_progress = nullptr
    );
};

} // namespace laya::net