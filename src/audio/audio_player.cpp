#include "laya/audio/audio_player.hpp"
#include "miniaudio.h"

namespace laya::audio {

AudioPlayer::AudioPlayer() 
    : engine_(std::make_unique<ma_engine>()),
      sound_(std::make_unique<ma_sound>()) {
    ma_engine_config config = ma_engine_config_init();
    ma_engine_init(&config, engine_.get());
}

AudioPlayer::~AudioPlayer() {
    if (is_loaded_) {
        ma_sound_uninit(sound_.get());
    }
    ma_engine_uninit(engine_.get());
}

AudioPlayer::AudioPlayer(AudioPlayer&&) noexcept = default;
AudioPlayer& AudioPlayer::operator=(AudioPlayer&&) noexcept = default;

bool AudioPlayer::load_file(const std::filesystem::path& path) {
    if (is_loaded_) {
        ma_sound_uninit(sound_.get());
        is_loaded_ = false;
        state_ = PlayerState::Stopped;
    }

    ma_result result = ma_sound_init_from_file(
        engine_.get(), 
        path.c_str(), 
        0, 
        nullptr, 
        nullptr, 
        sound_.get()
    );

    if (result != MA_SUCCESS) {
        return false;
    }

    is_loaded_ = true;
    state_ = PlayerState::Stopped;
    return true;
}

void AudioPlayer::play() {
    if (!is_loaded_) return;
    ma_sound_start(sound_.get());
    state_ = PlayerState::Playing;
}

void AudioPlayer::pause() {
    if (!is_loaded_) return;
    ma_sound_stop(sound_.get());
    state_ = PlayerState::Paused;
}

void AudioPlayer::toggle_play_pause() {
    if (state_ == PlayerState::Playing) {
        pause();
    } else {
        play();
    }
}

void AudioPlayer::stop() {
    if (!is_loaded_) return;
    ma_sound_stop(sound_.get());
    ma_sound_seek_to_pcm_frame(sound_.get(), 0);
    state_ = PlayerState::Stopped;
}

void AudioPlayer::seek(core::Milliseconds position) {
    if (!is_loaded_) return;
    float seconds = static_cast<float>(position.count()) / 1000.0f;
    ma_uint32 sampleRate;
    ma_sound_get_data_format(sound_.get(), nullptr, nullptr, &sampleRate, nullptr, 0);
    ma_uint64 frameIndex = static_cast<ma_uint64>(seconds * static_cast<float>(sampleRate));
    ma_sound_seek_to_pcm_frame(sound_.get(), frameIndex);
}

core::Milliseconds AudioPlayer::get_position() const {
    if (!is_loaded_) return core::Milliseconds(0);
    float cursor_seconds = 0.0f;
    ma_sound_get_cursor_in_seconds(sound_.get(), &cursor_seconds);
    return core::Milliseconds(static_cast<int64_t>(cursor_seconds * 1000.0f));
}

core::Milliseconds AudioPlayer::get_duration() const {
    if (!is_loaded_) return core::Milliseconds(0);
    float length_seconds = 0.0f;
    ma_sound_get_length_in_seconds(sound_.get(), &length_seconds);
    return core::Milliseconds(static_cast<int64_t>(length_seconds * 1000.0f));
}

} // namespace laya::audio