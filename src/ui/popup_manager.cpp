#include "laya/ui/popup_manager.hpp"
#include <vector>
#include <algorithm>

namespace laya::ui {

namespace {
struct Field {
    std::string tag_key;
    std::string label;
    std::string value;
    size_t cursor{0};
};
} // namespace

bool PopupManager::show_metadata_dialog(core::LrcDocument& doc) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int height = 13;
    int width = 58;
    int start_y = (max_y - height) / 2;
    int start_x = (max_x - width) / 2;

    WINDOW* popup = newwin(height, width, start_y, start_x);
    keypad(popup, TRUE);
    nodelay(popup, FALSE); // Blocking input inside modal
    curs_set(1);           // Show cursor

    std::vector<Field> fields = {
        {"ti", "Title   :", doc.get_tag("ti").value_or(""), 0},
        {"ar", "Artist  :", doc.get_tag("ar").value_or(""), 0},
        {"al", "Album   :", doc.get_tag("al").value_or(""), 0},
        {"by", "Creator :", doc.get_tag("by").value_or(""), 0}
    };

    for (auto& f : fields) {
        f.cursor = f.value.size();
    }

    size_t active_field = 0;
    bool dialog_running = true;
    bool saved = false;

    while (dialog_running) {
        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 0, 2, " [ Edit LRC Metadata ] ");

        // Draw input fields
        int field_start_y = 2;
        int input_start_x = 12;
        int input_width = width - input_start_x - 3;

        for (size_t i = 0; i < fields.size(); ++i) {
            int y = field_start_y + static_cast<int>(i) * 2;
            mvwprintw(popup, y, 2, "%s", fields[i].label.c_str());

            bool is_active = (i == active_field);
            if (is_active) {
                wattron(popup, A_BOLD | COLOR_PAIR(1));
            }

            // Draw field background bracket
            mvwprintw(popup, y, input_start_x - 1, "[");
            mvwprintw(popup, y, input_start_x + input_width, "]");

            // Print field value
            std::string display_val = fields[i].value;
            if (static_cast<int>(display_val.size()) > input_width) {
                display_val = display_val.substr(0, static_cast<size_t>(input_width));
            }
            mvwprintw(popup, y, input_start_x, "%-*s", input_width, display_val.c_str());

            if (is_active) {
                wattroff(popup, A_BOLD | COLOR_PAIR(1));
            }
        }

        // Action hints
        mvwprintw(popup, height - 2, 2, "[Enter] Save  |  [Tab/↑/↓] Switch  |  [Esc] Cancel");

        // Move hardware cursor to active field
        int cur_y = field_start_y + static_cast<int>(active_field) * 2;
        int cur_x = input_start_x + static_cast<int>(fields[active_field].cursor);
        wmove(popup, cur_y, cur_x);
        wrefresh(popup);

        int ch = wgetch(popup);
        auto& cur_field = fields[active_field];

        switch (ch) {
            case 27: // ESC
                dialog_running = false;
                saved = false;
                break;

            case '\n':
            case KEY_ENTER:
            case 13:
                // Save and apply metadata tags
                for (const auto& f : fields) {
                    if (!f.value.empty()) {
                        doc.set_tag(f.tag_key, f.value);
                    }
                }
                saved = true;
                dialog_running = false;
                break;

            case '\t':
            case KEY_DOWN:
                active_field = (active_field + 1) % fields.size();
                break;

            case KEY_BTAB: // Shift+Tab
            case KEY_UP:
                active_field = (active_field == 0) ? (fields.size() - 1) : (active_field - 1);
                break;

            case KEY_LEFT:
                if (cur_field.cursor > 0) cur_field.cursor--;
                break;

            case KEY_RIGHT:
                if (cur_field.cursor < cur_field.value.size()) cur_field.cursor++;
                break;

            case KEY_HOME:
                cur_field.cursor = 0;
                break;

            case KEY_END:
                cur_field.cursor = cur_field.value.size();
                break;

            case KEY_BACKSPACE:
            case 127:
            case 8:
                if (cur_field.cursor > 0) {
                    cur_field.value.erase(cur_field.cursor - 1, 1);
                    cur_field.cursor--;
                }
                break;

            case KEY_DC:
                if (cur_field.cursor < cur_field.value.size()) {
                    cur_field.value.erase(cur_field.cursor, 1);
                }
                break;

            default:
                if (ch >= 32 && ch <= 126) {
                    if (static_cast<int>(cur_field.value.size()) < input_width) {
                        cur_field.value.insert(cur_field.value.begin() + static_cast<ptrdiff_t>(cur_field.cursor), static_cast<char>(ch));
                        cur_field.cursor++;
                    }
                }
                break;
        }
    }

    delwin(popup);
    curs_set(0);
    touchwin(stdscr);
    refresh();
    return saved;
}

} // namespace laya::ui