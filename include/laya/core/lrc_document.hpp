#pragma once

#include "laya/core/time_utils.hpp"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <filesystem>

namespace laya::core {

struct LrcLine {
    std::optional<Milliseconds> timestamp;
    std::string text;
};

class LrcDocument {
public:
    LrcDocument() = default;

    bool load_from_file(const std::filesystem::path& path);
    bool save_to_file(const std::filesystem::path& path) const;
    void parse_content(std::string_view content);
    std::string serialize() const;

    // Line manipulations
    void add_line(std::optional<Milliseconds> ts, std::string text);
    void set_timestamp(size_t index, Milliseconds ts);
    void clear_timestamp(size_t index);

    // Metadata accessors (e.g. [ar: Artist], [ti: Title], [al: Album])
    void set_tag(const std::string& key, const std::string& value);
    std::optional<std::string> get_tag(const std::string& key) const;

    [[nodiscard]] const std::vector<LrcLine>& lines() const { return lines_; }
    [[nodiscard]] std::vector<LrcLine>& lines() { return lines_; }
    [[nodiscard]] const auto& metadata() const { return metadata_; }

private:
    std::vector<LrcLine> lines_;
    std::unordered_map<std::string, std::string> metadata_;
};

} // namespace laya::core