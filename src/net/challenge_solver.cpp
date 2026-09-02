#include "laya/net/challenge_solver.hpp"
#include "laya/crypto/sha256.hpp"
#include <thread>
#include <vector>
#include <algorithm>
#include <cctype>
#include <mutex>

namespace laya::net {

namespace {
std::string to_lower_str(std::string_view s) {
    std::string res(s);
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
    return res;
}
} // namespace

std::optional<std::string> ChallengeSolver::solve(
    std::string_view prefix, 
    std::string_view target_hex,
    ProgressCallback on_progress
) {
    std::string target_lower = to_lower_str(target_hex);
    unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
    
    std::atomic<bool> found{false};
    std::atomic<uint64_t> total_hashes{0};
    std::string solved_nonce;
    std::mutex nonce_mutex;

    std::vector<std::jthread> workers;
    workers.reserve(num_threads);

    for (unsigned int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            uint64_t nonce = t;
            uint64_t local_hashes = 0;

            while (!found.load(std::memory_order_relaxed)) {
                std::string nonce_str = std::to_string(nonce);
                std::string combined = std::string(prefix) + nonce_str;

                auto hash_res = crypto::Sha256::hash(combined);
                std::string hash_hex = crypto::Sha256::to_hex(hash_res);

                // Check condition: hash_hex <= target_hex
                if (hash_hex <= target_lower) {
                    found.store(true, std::memory_order_relaxed);
                    std::lock_guard lock(nonce_mutex);
                    solved_nonce = nonce_str;
                    break;
                }

                nonce += num_threads;
                local_hashes++;

                if (local_hashes % 50000 == 0) {
                    total_hashes.fetch_add(50000, std::memory_order_relaxed);
                    if (on_progress && t == 0) {
                        on_progress(total_hashes.load(std::memory_order_relaxed));
                    }
                }
            }
        });
    }

    // Wait for all threads to finish
    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    if (!solved_nonce.empty()) {
        return std::string(prefix) + ":" + solved_nonce;
    }
    return std::nullopt;
}

} // namespace laya::net