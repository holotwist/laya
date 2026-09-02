#include "laya/ui/editor_view.hpp"

namespace laya::ui {

void EditorView::render(WINDOW* win, const core::LrcDocument& doc, size_t selected_line, size_t cursor_col, bool is_edit_mode) {
    werase(win);
    box(win, 0, 0);

    if (is_edit_mode) {
        mvwprintw(win, 0, 2, " [ Lyrics Text Editor (EDIT MODE) ] ");
    } else {
        mvwprintw(win, 0, 2, " [ Lyrics / Sync Editor ] ");
    }

    int height, width;
    getmaxyx(win, height, width);
    int visible_rows = height - 2;

    const auto& lines = doc.lines();
    if (lines.empty()) {
        mvwprintw(win, height / 2, 4, "(No lyrics. Press 'e' to write or 'o' to paste)");
        wrefresh(win);
        return;
    }

    int start_idx = 0;
    if (static_cast<int>(selected_line) > visible_rows / 2) {
        start_idx = static_cast<int>(selected_line) - (visible_rows / 2);
    }
    int end_idx = std::min(start_idx + visible_rows, static_cast<int>(lines.size()));

    int cursor_screen_y = 1;
    int cursor_screen_x = 16;

    for (int i = start_idx; i < end_idx; ++i) {
        int row = 1 + (i - start_idx);
        const auto& line = lines[static_cast<size_t>(i)];
        bool is_selected = (static_cast<size_t>(i) == selected_line);

        std::string ts_str = line.timestamp.has_value() 
            ? core::format_timestamp(line.timestamp.value()) 
            : "[--:--.--]";

        if (is_selected && !is_edit_mode) {
            wattron(win, A_REVERSE | COLOR_PAIR(1));
        }

        mvwprintw(win, row, 1, "%s", is_selected ? ">" : " ");
        mvwprintw(win, row, 3, "%s", ts_str.c_str());

        int text_max_len = width - 18;
        if (text_max_len > 0) {
            if (is_edit_mode && is_selected) {
                // Render line with visual block cursor
                wmove(win, row, 16);
                size_t max_render_len = std::max(line.text.size() + 1, cursor_col + 1);
                for (size_t col = 0; col < max_render_len && col < static_cast<size_t>(text_max_len); ++col) {
                    bool is_cursor = (col == cursor_col);
                    if (is_cursor) {
                        wattron(win, A_REVERSE | A_BOLD | COLOR_PAIR(1));
                    }

                    if (col < line.text.size()) {
                        waddch(win, static_cast<unsigned char>(line.text[col]));
                    } else if (is_cursor) {
                        waddch(win, ' '); // Trailing cursor block
                    }

                    if (is_cursor) {
                        wattroff(win, A_REVERSE | A_BOLD | COLOR_PAIR(1));
                    }
                }
            } else {
                std::string display_text = line.text.substr(0, static_cast<size_t>(text_max_len));
                mvwprintw(win, row, 15, " %s", display_text.c_str());
            }
        }

        if (is_selected && !is_edit_mode) {
            wattroff(win, A_REVERSE | COLOR_PAIR(1));
        }

        if (is_selected) {
            cursor_screen_y = row;
            cursor_screen_x = 16 + static_cast<int>(cursor_col);
            if (cursor_screen_x >= width - 1) {
                cursor_screen_x = width - 2;
            }
        }
    }

    if (is_edit_mode) {
        wmove(win, cursor_screen_y, cursor_screen_x);
    }

    wrefresh(win);
}

} // namespace laya::ui