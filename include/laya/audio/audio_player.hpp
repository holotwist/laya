#pragma once

#include "laya/core/time_utils.hpp"
#include <string>
#include <memory>
#include <filesystem>

struct ma_engine;
struct ma_sound;

namespace laya::audio {

enum class PlayerState {
    Stopped,
    Playing,
    Paused
};

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    // Non-copyable, movable
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    AudioPlayer(AudioPlayer&&) noexcept;
    AudioPlayer& operator=(AudioPlayer&&) noexcept;

    bool load_file(const std::filesystem::path& path);
    void play();
    void pause();
    void toggle_play_pause();
    void stop();
    void seek(core::Milliseconds position);

    [[nodiscard]] core::Milliseconds get_position() const;
    [[nodiscard]] core::Milliseconds get_duration() const;
    [[nodiscard]] PlayerState get_state() const { return state_; }
    [[nodiscard]] bool is_loaded() const { return is_loaded_; }

private:
    std::unique_ptr<ma_engine> engine_;
    std::unique_ptr<ma_sound> sound_;
    PlayerState state_{PlayerState::Stopped};
    bool is_loaded_{false};
};

} // namespace laya::audio