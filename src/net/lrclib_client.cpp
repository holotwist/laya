#include "laya/net/lrclib_client.hpp"
#include "laya/net/challenge_solver.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace laya::net {

namespace {
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

const char* USER_AGENT = "Laya/0.1.0 (https://github.com/holotwist/laya)";
const char* BASE_URL = "https://lrclib.net/api";
} // namespace

LrclibClient::LrclibClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

LrclibClient::~LrclibClient() {
    curl_global_cleanup();
}

std::string LrclibClient::http_get(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

std::pair<long, std::string> LrclibClient::http_post(const std::string& url, const std::string& json_body, const std::string& token_header) {
    CURL* curl = curl_easy_init();
    std::string response;
    long http_code = 0;

    if (curl) {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!token_header.empty()) {
            std::string header_str = "X-Publish-Token: " + token_header;
            headers = curl_slist_append(headers, header_str.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return {http_code, response};
}

std::optional<LyricsResult> LrclibClient::get_lyrics(const TrackMetadata& meta) {
    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;

    char* esc_track = curl_easy_escape(curl, meta.track_name.c_str(), 0);
    char* esc_artist = curl_easy_escape(curl, meta.artist_name.c_str(), 0);
    char* esc_album = curl_easy_escape(curl, meta.album_name.c_str(), 0);

    std::ostringstream url;
    url << BASE_URL << "/get?track_name=" << esc_track
        << "&artist_name=" << esc_artist
        << "&album_name=" << esc_album
        << "&duration=" << meta.duration_seconds;

    curl_free(esc_track);
    curl_free(esc_artist);
    curl_free(esc_album);
    curl_easy_cleanup(curl);

    std::string resp = http_get(url.str());
    if (resp.empty()) return std::nullopt;

    try {
        auto j = json::parse(resp);
        if (j.contains("id")) {
            LyricsResult res;
            res.id = j.value("id", 0);
            res.track_name = j.value("trackName", "");
            res.artist_name = j.value("artistName", "");
            res.album_name = j.value("albumName", "");
            res.duration = static_cast<int>(j.value("duration", 0.0));
            res.plain_lyrics = j.value("plainLyrics", "");
            res.synced_lyrics = j.value("syncedLyrics", "");
            return res;
        }
    } catch (...) {}

    return std::nullopt;
}

bool LrclibClient::publish_lyrics(
    const TrackMetadata& meta,
    const std::string& plain_lyrics,
    const std::string& synced_lyrics,
    std::function<void(std::string_view)> status_cb
) {
    if (status_cb) status_cb("Requesting cryptographic challenge from LRCLIB");

    // Request Challenge
    std::string challenge_url = std::string(BASE_URL) + "/request-challenge";
    auto [code, body] = http_post(challenge_url, "{}");
    if (code != 200 && code != 201) return false;

    std::string prefix, target;
    try {
        auto j = json::parse(body);
        prefix = j.at("prefix").get<std::string>();
        target = j.at("target").get<std::string>();
    } catch (...) {
        return false;
    }

    // Solve PoW Challenge
    if (status_cb) status_cb("Solving Proof-of-Work challenge...");
    auto publish_token = ChallengeSolver::solve(prefix, target, [&](uint64_t hashes) {
        if (status_cb) {
            status_cb("Solving PoW: " + std::to_string(hashes / 1000) + "k hashes computed...");
        }
    });

    if (!publish_token.has_value()) return false;

    // Publish to LRCLIB
    if (status_cb) status_cb("Submitting lyrics to LRCLIB");
    json payload = {
        {"trackName", meta.track_name},
        {"artistName", meta.artist_name},
        {"albumName", meta.album_name},
        {"duration", meta.duration_seconds},
        {"plainLyrics", plain_lyrics},
        {"syncedLyrics", synced_lyrics}
    };

    std::string publish_url = std::string(BASE_URL) + "/publish";
    auto [pub_code, pub_body] = http_post(publish_url, payload.dump(), publish_token.value());

    return (pub_code == 200 || pub_code == 201);
}

} // namespace laya::net