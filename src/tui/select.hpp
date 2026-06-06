#pragma once
#include <string>
#include <vector>
#include <functional>

namespace dotrix::tui {

/// One selectable item.
struct Item {
    std::string name;
    std::string desc;
    bool checked = true;
    bool installed = false;   // dimmed if already installed
    bool needs_sudo = false;
};

/// Simple checkbox-list TUI using ANSI escapes.  Zero dependencies.
///
/// Returns indices of selected items (empty = cancelled).
/// Navigation:  ↑↓ / jk   Space=toggle   Enter=install   q=quit
///              a=all  n=none
std::vector<int> select(std::vector<Item>& items,
                        const std::string& title = "Select tools to install");

} // namespace dotrix::tui
