#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace dotrix::util {

/// Recursively copy file or directory.
void copy_tree(const fs::path& src, const fs::path& dst);

/// True if two files (or directories) differ in content.
bool files_differ(const fs::path& a, const fs::path& b);

/// List all regular files under a path (recursive).
std::vector<fs::path> list_files(const fs::path& root);

/// Compute relative path; throws if `p` is not under `base`.
std::string relative_to(const fs::path& p, const fs::path& base);

} // namespace dotrix::util
