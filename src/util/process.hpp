#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix::util {

/// Run a command; returns true on success.
bool run(const std::vector<std::string>& cmd);

/// Run and capture stdout.  Returns empty string on failure.
std::string capture(const std::vector<std::string>& cmd);

} // namespace dotrix::util
