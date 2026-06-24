#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix::util {

/// Run a command; returns true on success (stdout passes through).
bool run(const std::vector<std::string>& cmd);

/// Run silently — output goes to /dev/null. Returns true on success.
/// If log_path is non-empty, output is redirected there instead (for failure inspection).
bool run_silent(const std::vector<std::string>& cmd, const std::string& log_path = "");

/// Run and capture stdout.  Returns empty string on failure.
std::string capture(const std::vector<std::string>& cmd);

} // namespace dotrix::util
