#include "laya/ui/editor_view.hpp"

namespace laya::ui {

void EditorView::render(WINDOW* win, const core::LrcDocument& doc, size_t selected_line, core::Milliseconds current_time) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " [ Lyrics / Sync Editor ] ");

    int height, width;
    getmaxyx(win, height, width);
    int visible_rows = height - 2;

    const auto& lines = doc.lines();
    if (lines.empty()) {
        mvwprintw(win, height / 2, 4, "(No lyrics loaded. Press 'o' or paste text)");
        wrefresh(win);
        return;
    }

    // Scroll window calculation so selected_line stays centered
    int start_idx = 0;
    if (static_cast<int>(selected_line) > visible_rows / 2) {
        start_idx = static_cast<int>(selected_line) - (visible_rows / 2);
    }
    int end_idx = std::min(start_idx + visible_rows, static_cast<int>(lines.size()));

    for (int i = start_idx; i < end_idx; ++i) {
        int row = 1 + (i - start_idx);
        const auto& line = lines[static_cast<size_t>(i)];

        bool is_selected = (static_cast<size_t>(i) == selected_line);
        std::string ts_str = line.timestamp.has_value() 
            ? core::format_timestamp(line.timestamp.value()) 
            : "[--:--.--]";

        if (is_selected) {
            wattron(win, A_REVERSE | COLOR_PAIR(1));
        }

        // Draw cursor pointer
        mvwprintw(win, row, 1, "%s", is_selected ? ">" : " ");

        // Print timestamp
        mvwprintw(win, row, 3, "%s", ts_str.c_str());

        // Print text truncated to fit column
        int text_max_len = width - 18;
        if (text_max_len > 0) {
            std::string display_text = line.text.substr(0, static_cast<size_t>(text_max_len));
            mvwprintw(win, row, 15, " %s", display_text.c_str());
        }

        if (is_selected) {
            wattroff(win, A_REVERSE | COLOR_PAIR(1));
        }
    }

    wrefresh(win);
}

} // namespace laya::ui