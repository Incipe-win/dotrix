#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

inline constexpr const char* REDACTED_PLACEHOLDER = "__DOTRIX_REDACTED__";

/// One detected secret.
struct Finding {
    int line;
    std::string reason;
    std::string snippet;
};

/// Scan + redact API keys / tokens from config files.
class SecretsGuard {
public:
    /// Scan a file. Returns findings (empty = clean).
    static std::vector<Finding> scan_file(const fs::path& path);

    /// Scan a directory recursively.
    static std::vector<Finding> scan_dir(const fs::path& path);

    /// Print findings to stderr.
    static void report(const std::vector<Finding>& findings, const fs::path& path);

    /// True if any finding is high-confidence (known key prefix).
    static bool has_high_confidence(const std::vector<Finding>& findings);

    /// Check if file content contains redacted placeholders.
    static bool is_redacted(const fs::path& path);

    /// Read `src`, replace secret values with __DOTRIX_REDACTED__,
    /// write to `dst`.  Returns number of lines redacted.
    static int redact_file(const fs::path& src, const fs::path& dst);

private:
    static void check_line(const std::string& line, int lineno,
                           std::vector<Finding>& out);
    static bool looks_like_key(const std::string& line);
    static bool looks_like_token(const std::string& line);
    static bool looks_like_password(const std::string& line);

    /// Redact a single line. Returns redacted version, or empty if no change.
    static std::string redact_line(const std::string& line);
};

} // namespace dotrix
