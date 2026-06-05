#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// A managed entry — path relative to $HOME, e.g. ".zshrc"
struct Entry {
    std::string relative_path;   // home-relative, e.g. ".zshrc"
};

using Manifest = std::vector<Entry>;

} // namespace dotrix
