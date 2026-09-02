#pragma once

#include "laya/core/lrc_document.hpp"
#include <ncurses.h>
#include <cstddef>

namespace laya::ui {

class EditorView {
public:
    void render(WINDOW* win, const core::LrcDocument& doc, size_t selected_line, size_t cursor_col, bool is_edit_mode);
};

} // namespace laya::ui