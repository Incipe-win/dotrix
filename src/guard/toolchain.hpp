#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// Known tool: how to detect it and how to install it (user-local, no sudo).
struct Tool {
    const char* pattern;     // substring match on home-relative path
    const char* binary;      // command to check with `which`
    const char* install;     // shell command(s) — one-liner or multi-step;
                             // empty = system tool, skip
};

/// Shared tool database.  Add new entries here — check + bootstrap pick them up.
const std::vector<Tool>& known_tools();

} // namespace dotrix
