#pragma once

#include "laya/core/lrc_document.hpp"
#include <string>
#include <optional>
#include <functional>

namespace laya::net {

struct TrackMetadata {
    std::string track_name;
    std::string artist_name;
    std::string album_name;
    int duration_seconds{0};
};

struct LyricsResult {
    int id{0};
    std::string track_name;
    std::string artist_name;
    std::string album_name;
    int duration{0};
    bool instrumental{false};
    std::string plain_lyrics;
    std::string synced_lyrics;
};

class LrclibClient {
public:
    LrclibClient();
    ~LrclibClient();

    // Fetch lyrics for exact match
    std::optional<LyricsResult> get_lyrics(const TrackMetadata& meta);

    // Search lyrics by keyword query
    std::vector<LyricsResult> search_lyrics(const std::string& query);

    // Request challenge, solve PoW, and publish lyrics to LRCLIB
    // Returns pair of <success, detailed_message>
    std::pair<bool, std::string> publish_lyrics(
        const TrackMetadata& meta,
        const std::string& plain_lyrics,
        const std::string& synced_lyrics,
        std::function<void(std::string_view step_desc)> status_cb = nullptr
    );

private:
    std::string http_get(const std::string& url);
    std::pair<long, std::string> http_post(const std::string& url, const std::string& json_body, const std::string& token_header = "");
};

} // namespace laya::net