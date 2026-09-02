#include "laya/ui/app.hpp"
#include <clocale>
#include <thread>
#include <chrono>

namespace laya::ui {

App::App(std::filesystem::path audio_path, std::filesystem::path lrc_path)
    : audio_path_(std::move(audio_path)), lrc_path_(std::move(lrc_path)) {
    
    // Load LRC if exists
    if (std::filesystem::exists(lrc_path_)) {
        doc_.load_from_file(lrc_path_);
    }

    // Load Audio
    if (std::filesystem::exists(audio_path_)) {
        player_.load_file(audio_path_);
    }

    init_curses();
    update_layout();
}

App::~App() {
    cleanup_curses();
}

void App::init_curses() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE); // Non-blocking input
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_GREEN, -1);
    }
}

void App::cleanup_curses() {
    if (editor_win_) delwin(editor_win_);
    if (player_win_) delwin(player_win_);
    if (status_win_) delwin(status_win_);
    endwin();
}

void App::update_layout() {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    if (editor_win_) delwin(editor_win_);
    if (player_win_) delwin(player_win_);
    if (status_win_) delwin(status_win_);

    int main_height = max_y - 1;
    int split_x = (max_x * 6) / 10; // 60% Left Editor, 40% Right Player

    editor_win_ = newwin(main_height, split_x, 0, 0);
    player_win_ = newwin(main_height, max_x - split_x, 0, split_x);
    status_win_ = newwin(1, max_x, max_y - 1, 0);
}

void App::handle_input(int ch) {
    if (ch == ERR) return;

    switch (ch) {
        case 'q':
        case 'Q':
            running_ = false;
            break;

        case ' ':
            player_.toggle_play_pause();
            break;

        case KEY_DOWN:
        case 'j':
            if (selected_line_ + 1 < doc_.lines().size()) {
                selected_line_++;
            }
            break;

        case KEY_UP:
        case 'k':
            if (selected_line_ > 0) {
                selected_line_--;
            }
            break;

        // Stamp active audio position to selected line and advance
        case '\t':
        case '\n':
        case KEY_ENTER: {
            if (!doc_.lines().empty()) {
                doc_.set_timestamp(selected_line_, player_.get_position());
                status_message_ = "Stamped line " + std::to_string(selected_line_ + 1);
                if (selected_line_ + 1 < doc_.lines().size()) {
                    selected_line_++;
                }
            }
            break;
        }

        // Clear timestamp on current line
        case 'c':
            doc_.clear_timestamp(selected_line_);
            status_message_ = "Cleared timestamp on line " + std::to_string(selected_line_ + 1);
            break;

        // Audio seeking
        case KEY_LEFT:
        case 'h': {
            auto pos = player_.get_position();
            player_.seek(pos > core::Milliseconds(5000) ? pos - core::Milliseconds(5000) : core::Milliseconds(0));
            break;
        }
        case KEY_RIGHT:
        case 'l':
            player_.seek(player_.get_position() + core::Milliseconds(5000));
            break;

        // Nudge active timestamp by ±50ms
        case '[': {
            if (selected_line_ < doc_.lines().size()) {
                auto ts = doc_.lines()[selected_line_].timestamp;
                if (ts.has_value() && ts.value() > core::Milliseconds(50)) {
                    doc_.set_timestamp(selected_line_, ts.value() - core::Milliseconds(50));
                }
            }
            break;
        }
        case ']': {
            if (selected_line_ < doc_.lines().size()) {
                auto ts = doc_.lines()[selected_line_].timestamp;
                if (ts.has_value()) {
                    doc_.set_timestamp(selected_line_, ts.value() + core::Milliseconds(50));
                }
            }
            break;
        }

        // Save
        case 's':
            if (doc_.save_to_file(lrc_path_)) {
                status_message_ = "Saved to " + lrc_path_.string();
            } else {
                status_message_ = "Error saving file!";
            }
            break;

        case KEY_RESIZE:
            update_layout();
            break;
    }
}

void App::run() {
    while (running_) {
        int ch = getch();
        handle_input(ch);

        // Render views
        editor_view_.render(editor_win_, doc_, selected_line_, player_.get_position());
        player_view_.render(player_win_, player_, doc_);

        // Render status bar
        werase(status_win_);
        wattron(status_win_, A_REVERSE);
        mvwprintw(status_win_, 0, 0, " %s", status_message_.c_str());
        wattroff(status_win_, A_REVERSE);
        wrefresh(status_win_);

        // Frame rate limit (~30 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}

} // namespace laya::ui