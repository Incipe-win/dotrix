#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// A managed entry — can be a single file or a directory.
struct Entry {
    std::string original_path;   // absolute path, e.g. /home/user/.zshrc
};

using Manifest = std::vector<Entry>;

} // namespace dotrix
