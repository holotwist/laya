#include "laya/core/lrc_document.hpp"
#include <fstream>
#include <sstream>
#include <regex>

namespace laya::core {

bool LrcDocument::load_from_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    lines_.clear();
    metadata_.clear();

    std::stringstream buffer;
    buffer << file.rdbuf();
    parse_content(buffer.str());
    return true;
}

bool LrcDocument::save_to_file(const std::filesystem::path& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << serialize();
    return true;
}

void LrcDocument::parse_content(std::string_view content) {
    std::istringstream stream{std::string(content)};
    std::string line_str;

    static const std::regex meta_regex(R"(\[(\w+):(.*)\])");
    static const std::regex time_regex(R"(^(\[\d{2,}:\d{2}\.\d{2,3}\])(.*)$)");

    while (std::getline(stream, line_str)) {
        if (line_str.empty()) continue;
        if (!line_str.empty() && line_str.back() == '\r') {
            line_str.pop_back();
        }

        std::smatch match;
        if (std::regex_match(line_str, match, time_regex)) {
            auto ts = parse_timestamp(match[1].str());
            std::string text = match[2].str();
            // Trim leading space if present
            if (!text.empty() && text.front() == ' ') {
                text.erase(0, 1);
            }
            lines_.push_back({ts, std::move(text)});
        } else if (std::regex_match(line_str, match, meta_regex)) {
            metadata_[match[1].str()] = match[2].str();
        } else {
            // Raw text line without timestamp
            lines_.push_back({std::nullopt, line_str});
        }
    }
}

std::string LrcDocument::serialize() const {
    std::ostringstream oss;
    
    // Write standard metadata tags
    for (const auto& [tag, val] : metadata_) {
        oss << "[" << tag << ":" << val << "]\n";
    }

    // Write lyric lines
    for (const auto& line : lines_) {
        if (line.timestamp.has_value()) {
            oss << format_timestamp(line.timestamp.value()) << " " << line.text << "\n";
        } else {
            oss << line.text << "\n";
        }
    }

    return oss.str();
}

void LrcDocument::add_line(std::optional<Milliseconds> ts, std::string text) {
    lines_.push_back({ts, std::move(text)});
}

void LrcDocument::set_timestamp(size_t index, Milliseconds ts) {
    if (index < lines_.size()) {
        lines_[index].timestamp = ts;
    }
}

void LrcDocument::clear_timestamp(size_t index) {
    if (index < lines_.size()) {
        lines_[index].timestamp = std::nullopt;
    }
}

void LrcDocument::set_tag(const std::string& key, const std::string& value) {
    metadata_[key] = value;
}

std::optional<std::string> get_tag(const std::string& key);

} // namespace lrctool::core