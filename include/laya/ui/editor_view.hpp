#pragma once

#include "laya/core/lrc_document.hpp"
#include <ncurses.h>
#include <cstddef>

namespace laya::ui {

class EditorView {
public:
    void render(WINDOW* win, const core::LrcDocument& doc, size_t selected_line, core::Milliseconds current_time);
};

} // namespace laya::ui