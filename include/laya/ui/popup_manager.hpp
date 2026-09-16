#pragma once

#include "laya/core/lrc_document.hpp"
#include <ncurses.h>
#include <string>

namespace laya::ui {

class PopupManager {
public:
    PopupManager() = default;

    // Shows interactive modal
    // Returns true if user saved changes, false if cancelled (Esc)
    bool show_metadata_dialog(core::LrcDocument& doc);
};

} // namespace laya::ui