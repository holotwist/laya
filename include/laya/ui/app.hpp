#pragma once

#include "laya/core/lrc_document.hpp"
#include "laya/audio/audio_player.hpp"
#include "laya/ui/editor_view.hpp"
#include "laya/ui/player_view.hpp"
#include <string>
#include <filesystem>

namespace laya::ui {

class App {
public:
    App(std::filesystem::path audio_path, std::filesystem::path lrc_path);
    ~App();

    void run();

private:
    void init_curses();
    void cleanup_curses();
    void handle_input(int ch);
    void update_layout();

    std::filesystem::path audio_path_;
    std::filesystem::path lrc_path_;
    core::LrcDocument doc_;
    audio::AudioPlayer player_;

    EditorView editor_view_;
    PlayerView player_view_;

    WINDOW* editor_win_{nullptr};
    WINDOW* player_win_{nullptr};
    WINDOW* status_win_{nullptr};

    size_t selected_line_{0};
    bool running_{true};
    std::string status_message_{"Ready"};
};

} // namespace laya::ui