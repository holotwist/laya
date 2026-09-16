#include "laya/core/time_utils.hpp"

#include <iomanip>
#include <regex>
#include <sstream>

namespace laya::core {

std::string format_timestamp(Milliseconds ms) {
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(ms);
    auto hundredths = (ms.count() % 1000) / 10;

    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(total_seconds);
    auto seconds = total_seconds - minutes;

    std::ostringstream oss;
    oss << "[" 
        << std::setfill('0') << std::setw(2) << minutes.count() << ":"
        << std::setfill('0') << std::setw(2) << seconds.count() << "."
        << std::setfill('0') << std::setw(2) << hundredths
        << "]";
    return oss.str();
}

std::optional<Milliseconds> parse_timestamp(std::string_view str) {
    // Matches [mm:ss.xx] or [mm:ss.xxx]
    static const std::regex pattern(R"(\[(\d{2,}):(\d{2})\.(\d{2,3})\])");
    std::string s(str);
    std::smatch match;

    if (std::regex_search(s, match, pattern)) {
        int minutes = std::stoi(match[1].str());
        int seconds = std::stoi(match[2].str());
        std::string fraction_str = match[3].str();
        int ms_val = 0;

        if (fraction_str.length() == 2) {
            ms_val = std::stoi(fraction_str) * 10;
        } else {
            ms_val = std::stoi(fraction_str);
        }

        return Milliseconds((minutes * 60 + seconds) * 1000 + ms_val);
    }
    return std::nullopt;
}
}  // namespace laya::core