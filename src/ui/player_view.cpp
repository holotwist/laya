#include "laya/ui/player_view.hpp"

namespace laya::ui {

void PlayerView::render(WINDOW* win, const audio::AudioPlayer& player, const core::LrcDocument& doc) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " [ Mini Player & Controls ] ");

    int width = getmaxx(win);

    // Playback status
    const char* status_str = "STOPPED";
    if (player.get_state() == audio::PlayerState::Playing) status_str = "PLAYING";
    else if (player.get_state() == audio::PlayerState::Paused) status_str = "PAUSED";

    mvwprintw(win, 2, 2, "Status : %s", status_str);

    // Position / Duration
    auto pos = player.get_position();
    auto dur = player.get_duration();
    mvwprintw(win, 3, 2, "Time   : %s / %s", 
              core::format_timestamp(pos).c_str(), 
              core::format_timestamp(dur).c_str());

    // Progress Bar
    int bar_width = width - 6;
    if (bar_width > 4 && dur.count() > 0) {
        float progress = static_cast<float>(pos.count()) / static_cast<float>(dur.count());
        int filled = static_cast<int>(progress * static_cast<float>(bar_width));
        mvwprintw(win, 5, 2, "[");
        for (int i = 0; i < bar_width; ++i) {
            waddch(win, (i < filled) ? '=' : '-');
        }
        waddch(win, ']');
    }

    // Metadata section
    mvwprintw(win, 7, 2, "Metadata");
    mvwprintw(win, 8, 2, "Title : %s", doc.get_tag("ti").value_or("N/A").c_str());
    mvwprintw(win, 9, 2, "Artist: %s", doc.get_tag("ar").value_or("N/A").c_str());
    mvwprintw(win, 10, 2, "Album : %s", doc.get_tag("al").value_or("N/A").c_str());

    // Keybindings Cheat-Sheet
    int help_y = 12;
    mvwprintw(win, help_y++, 2, "Keybindings");
    mvwprintw(win, help_y++, 2, "o / Ctrl+V: Paste lyrics from clipboard");
    mvwprintw(win, help_y++, 2, "Space     : Play / Pause");
    mvwprintw(win, help_y++, 2, "Tab/Enter : Stamp time & Next line");
    mvwprintw(win, help_y++, 2, "g         : Jump audio to line time");
    mvwprintw(win, help_y++, 2, "j/k / ↑/↓ : Navigate lines");
    mvwprintw(win, help_y++, 2, "h/l / ←/→ : Seek -5s / +5s");
    mvwprintw(win, help_y++, 2, "[ / ]     : Nudge tag -50ms / +50ms");
    mvwprintw(win, help_y++, 2, "c         : Clear timestamp");
    mvwprintw(win, help_y++, 2, "s         : Save LRC file");
    mvwprintw(win, help_y++, 2, "q         : Quit");

    wrefresh(win);
}

} // namespace laya::ui