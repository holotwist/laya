#include "laya/ui/app.hpp"
#include "laya/net/lrclib_client.hpp"
#include <clocale>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <sstream>
#include <array>
#include <cstdio>
#include <memory>

namespace laya::ui {

namespace {
std::mutex g_app_mutex;
std::atomic<bool> g_network_busy{false};

std::string read_system_clipboard() {
    // Tries Wayland (wl-paste) and X11 (xclip / xsel)
    const char* cmds[] = {
        "wl-paste 2>/dev/null",
        "xclip -selection clipboard -o 2>/dev/null",
        "xsel --clipboard --output 2>/dev/null"
    };

    struct PipeCloser {
        void operator()(FILE* fp) const {
            if (fp) pclose(fp);
        }
    };

    for (const char* cmd : cmds) {
        std::unique_ptr<FILE, PipeCloser> pipe(popen(cmd, "r"));
        if (!pipe) continue;
        
        std::array<char, 512> buffer;
        std::string output;
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            output += buffer.data();
        }
        if (!output.empty()) {
            return output;
        }
    }
    return "";
}

std::string extract_plain_lyrics(const core::LrcDocument& doc) {
    std::ostringstream oss;
    for (const auto& line : doc.lines()) {
        oss << line.text << "\n";
    }
    return oss.str();
}
} // namespace

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
        init_pair(3, COLOR_YELLOW, -1);
        init_pair(4, COLOR_RED, -1);
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

void App::enter_edit_mode() {
    mode_ = AppMode::Edit;
    original_snapshot_ = doc_.lines();
    if (doc_.lines().empty()) {
        doc_.lines().push_back({std::nullopt, ""});
        selected_line_ = 0;
    }
    cursor_col_ = (selected_line_ < doc_.lines().size()) ? doc_.lines()[selected_line_].text.size() : 0;
    curs_set(1);
    status_message_ = "EDIT MODE (Press 'e' or 'Esc' to exit & preserve unchanged timestamps)";
}

void App::exit_edit_mode() {
    mode_ = AppMode::Sync;
    curs_set(0);

    if (doc_.lines().size() == 1 && doc_.lines()[0].text.empty()) {
        doc_.lines().clear();
    }

    std::vector<bool> snapshot_used(original_snapshot_.size(), false);

    for (size_t i = 0; i < doc_.lines().size(); ++i) {
        auto& line = doc_.lines()[i];

        if (i < original_snapshot_.size() && line.text == original_snapshot_[i].text) {
            line.timestamp = original_snapshot_[i].timestamp;
            snapshot_used[i] = true;
            continue;
        }

        if (!line.text.empty()) {
            for (size_t j = 0; j < original_snapshot_.size(); ++j) {
                if (!snapshot_used[j] && line.text == original_snapshot_[j].text) {
                    line.timestamp = original_snapshot_[j].timestamp;
                    snapshot_used[j] = true;
                    break;
                }
            }
        }
    }

    if (selected_line_ >= doc_.lines().size() && !doc_.lines().empty()) {
        selected_line_ = doc_.lines().size() - 1;
    }
    status_message_ = "Switched to Sync Mode. Preserved matching timestamps.";
}

void App::handle_edit_input(int ch) {
    auto& lines = doc_.lines();

    switch (ch) {
        case 'e':
        case 'E':
        case 27: // ESC
            exit_edit_mode();
            break;

        case KEY_LEFT:
            if (cursor_col_ > 0) {
                cursor_col_--;
            } else if (selected_line_ > 0) {
                selected_line_--;
                cursor_col_ = lines[selected_line_].text.size();
            }
            break;

        case KEY_RIGHT:
            if (selected_line_ < lines.size() && cursor_col_ < lines[selected_line_].text.size()) {
                cursor_col_++;
            } else if (selected_line_ + 1 < lines.size()) {
                selected_line_++;
                cursor_col_ = 0;
            }
            break;

        case KEY_UP:
            if (selected_line_ > 0) {
                selected_line_--;
                cursor_col_ = std::min(cursor_col_, lines[selected_line_].text.size());
            }
            break;

        case KEY_DOWN:
            if (selected_line_ + 1 < lines.size()) {
                selected_line_++;
                cursor_col_ = std::min(cursor_col_, lines[selected_line_].text.size());
            }
            break;

        case KEY_HOME:
        case 1: // Ctrl+A
            cursor_col_ = 0;
            break;

        case KEY_END:
        case 5: // Ctrl+E
            if (selected_line_ < lines.size()) {
                cursor_col_ = lines[selected_line_].text.size();
            }
            break;

        case '\n':
        case KEY_ENTER:
        case 13: {
            if (selected_line_ < lines.size()) {
                std::string current_text = lines[selected_line_].text;
                std::string left = current_text.substr(0, cursor_col_);
                std::string right = current_text.substr(cursor_col_);

                lines[selected_line_].text = left;
                lines.insert(lines.begin() + static_cast<ptrdiff_t>(selected_line_ + 1), {std::nullopt, right});

                selected_line_++;
                cursor_col_ = 0;
            }
            break;
        }

        case KEY_BACKSPACE:
        case 127:
        case 8: {
            if (selected_line_ < lines.size()) {
                auto& line = lines[selected_line_];
                if (cursor_col_ > 0) {
                    line.text.erase(cursor_col_ - 1, 1);
                    cursor_col_--;
                } else if (selected_line_ > 0) {
                    size_t prev_len = lines[selected_line_ - 1].text.size();
                    lines[selected_line_ - 1].text += line.text;
                    lines.erase(lines.begin() + static_cast<ptrdiff_t>(selected_line_));
                    selected_line_--;
                    cursor_col_ = prev_len;
                }
            }
            break;
        }

        case KEY_DC: {
            if (selected_line_ < lines.size()) {
                auto& line = lines[selected_line_];
                if (cursor_col_ < line.text.size()) {
                    line.text.erase(cursor_col_, 1);
                } else if (selected_line_ + 1 < lines.size()) {
                    line.text += lines[selected_line_ + 1].text;
                    lines.erase(lines.begin() + static_cast<ptrdiff_t>(selected_line_ + 1));
                }
            }
            break;
        }

        default:
            if (ch >= 32 && ch <= 126 && selected_line_ < lines.size()) {
                lines[selected_line_].text.insert(lines[selected_line_].text.begin() + static_cast<ptrdiff_t>(cursor_col_), static_cast<char>(ch));
                cursor_col_++;
            }
            break;
    }
}

void App::handle_sync_input(int ch) {
    switch (ch) {
        case 'q':
        case 'Q':
            running_ = false;
            break;

        case 'e':
        case 'E': {
            std::lock_guard lock(g_app_mutex);
            enter_edit_mode();
            break;
        }

        case ' ':
            player_.toggle_play_pause();
            break;

        case KEY_DOWN:
        case 'j': {
            std::lock_guard lock(g_app_mutex);
            if (selected_line_ + 1 < doc_.lines().size()) {
                selected_line_++;
            }
            break;
        }

        case KEY_UP:
        case 'k': {
            std::lock_guard lock(g_app_mutex);
            if (selected_line_ > 0) {
                selected_line_--;
            }
            break;
        }

        // Jump audio playback directly to current line's timestamp
        case 'g':
        case 'G': {
            std::lock_guard lock(g_app_mutex);
            if (selected_line_ < doc_.lines().size()) {
                const auto& ts = doc_.lines()[selected_line_].timestamp;
                if (ts.has_value()) {
                    player_.seek(ts.value());
                    status_message_ = "Seeked to " + core::format_timestamp(ts.value());
                }
            }
            break;
        }

        // Stamp active audio position to selected line and advance
        case '\t':
        case '\n':
        case KEY_ENTER: {
            std::lock_guard lock(g_app_mutex);
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
        case 'c': {
            std::lock_guard lock(g_app_mutex);
            doc_.clear_timestamp(selected_line_);
            status_message_ = "Cleared timestamp on line " + std::to_string(selected_line_ + 1);
            break;
        }

        // Audio seeking (-5s / +5s)
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
            std::lock_guard lock(g_app_mutex);
            if (selected_line_ < doc_.lines().size()) {
                auto ts = doc_.lines()[selected_line_].timestamp;
                if (ts.has_value() && ts.value() > core::Milliseconds(50)) {
                    doc_.set_timestamp(selected_line_, ts.value() - core::Milliseconds(50));
                }
            }
            break;
        }
        case ']': {
            std::lock_guard lock(g_app_mutex);
            if (selected_line_ < doc_.lines().size()) {
                auto ts = doc_.lines()[selected_line_].timestamp;
                if (ts.has_value()) {
                    doc_.set_timestamp(selected_line_, ts.value() + core::Milliseconds(50));
                }
            }
            break;
        }

        // Save LRC file
        case 's':
        case 'S': {
            std::lock_guard lock(g_app_mutex);
            if (doc_.save_to_file(lrc_path_)) {
                status_message_ = "Saved to " + lrc_path_.string();
            } else {
                status_message_ = "Error saving file!";
            }
            break;
        }

        // Paste / Open lyrics from System Clipboard (o or Ctrl+V)
        case 'o':
        case 'O':
        case 22: { // ASCII 22 = Ctrl+V
            std::string clip = read_system_clipboard();
            if (!clip.empty()) {
                std::lock_guard lock(g_app_mutex);
                doc_.parse_content(clip);
                selected_line_ = 0;
                status_message_ = "Loaded " + std::to_string(doc_.lines().size()) + " lines from clipboard";
            } else {
                status_message_ = "Clipboard is empty or (wl-paste/xclip) not installed.";
            }
            break;
        }

        // Fetch from LRCLIB
        case 'f':
        case 'F': {
            if (g_network_busy.load()) {
                status_message_ = "Network task already running...";
                break;
            }

            g_network_busy.store(true);
            status_message_ = "Fetching lyrics from LRCLIB...";

            std::thread([this]() {
                net::LrclibClient client;
                net::TrackMetadata meta;

                {
                    std::lock_guard lock(g_app_mutex);
                    meta.track_name = doc_.get_tag("ti").value_or(audio_path_.stem().string());
                    meta.artist_name = doc_.get_tag("ar").value_or("");
                    meta.album_name = doc_.get_tag("al").value_or("");
                    meta.duration_seconds = static_cast<int>(player_.get_duration().count() / 1000);
                }

                auto result = client.get_lyrics(meta);

                std::lock_guard lock(g_app_mutex);
                if (result.has_value()) {
                    if (!result->synced_lyrics.empty()) {
                        doc_.parse_content(result->synced_lyrics);
                        status_message_ = "Loaded synced lyrics from LRCLIB!";
                    } else if (!result->plain_lyrics.empty()) {
                        doc_.parse_content(result->plain_lyrics);
                        status_message_ = "Loaded unsynced lyrics from LRCLIB (Ready to sync).";
                    } else {
                        status_message_ = "Track found on LRCLIB but has no lyrics.";
                    }
                    selected_line_ = 0;
                } else {
                    status_message_ = "No matching lyrics found on LRCLIB.";
                }

                g_network_busy.store(false);
            }).detach();
            break;
        }

        // Publish to LRCLIB (Solves PoW and uploads)
        case 'p':
        case 'P': {
            if (g_network_busy.load()) {
                status_message_ = "Network task already running";
                break;
            }

            g_network_busy.store(true);

            std::thread([this]() {
                net::LrclibClient client;
                net::TrackMetadata meta;
                std::string synced_lyrics;
                std::string plain_lyrics;

                {
                    std::lock_guard lock(g_app_mutex);
                    meta.track_name = doc_.get_tag("ti").value_or(audio_path_.stem().string());
                    meta.artist_name = doc_.get_tag("ar").value_or("");
                    meta.album_name = doc_.get_tag("al").value_or("");
                    meta.duration_seconds = static_cast<int>(player_.get_duration().count() / 1000);

                    synced_lyrics = doc_.serialize();
                    plain_lyrics = extract_plain_lyrics(doc_);
                }

                bool success = client.publish_lyrics(
                    meta,
                    plain_lyrics,
                    synced_lyrics,
                    [this](std::string_view msg) {
                        std::lock_guard lock(g_app_mutex);
                        status_message_ = std::string(msg);
                    }
                );

                std::lock_guard lock(g_app_mutex);
                if (success) {
                    status_message_ = "Successfully published to LRCLIB";
                } else {
                    status_message_ = "Failed to publish to LRCLIB.";
                }

                g_network_busy.store(false);
            }).detach();
            break;
        }

        case KEY_RESIZE:
            update_layout();
            break;
    }
}

void App::handle_input(int ch) {
    if (ch == ERR) return;

    if (mode_ == AppMode::Edit) {
        std::lock_guard lock(g_app_mutex);
        handle_edit_input(ch);
    } else {
        handle_sync_input(ch);
    }
}

void App::run() {
    while (running_) {
        int ch = getch();
        handle_input(ch);

        {
            std::lock_guard lock(g_app_mutex);
            // Render player view
            player_view_.render(player_win_, player_, doc_);

            // Render status bar
            werase(status_win_);
            wattron(status_win_, A_REVERSE);
            mvwprintw(status_win_, 0, 0, " %s", status_message_.c_str());
            wattroff(status_win_, A_REVERSE);
            wrefresh(status_win_);

            // Render editor view last so cursor stays placed inside the editor
            editor_view_.render(editor_win_, doc_, selected_line_, cursor_col_, mode_ == AppMode::Edit);
        }

        // Target ~30 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}

} // namespace laya::ui