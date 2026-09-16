#pragma once

#include "laya/audio/audio_player.hpp"
#include "laya/core/lrc_document.hpp"
#include <ncurses.h>

namespace laya::ui {

class PlayerView {
public:
    void render(WINDOW* win, const audio::AudioPlayer& player, const core::LrcDocument& doc);
};

} // namespace laya::ui