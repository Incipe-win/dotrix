#include "secrets.hpp"
#include "ui/reporter.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <map>

namespace dotrix {

// ---- Helpers ----

static bool contains_any(const std::string& s, const std::vector<std::string>& keys) {
    auto lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (auto& k : keys)
        if (lower.find(k) != std::string::npos) return true;
    return false;
}

static std::string truncate(const std::string& s, size_t n = 60) {
    if (s.size() <= n) return s;
    return s.substr(0, n) + "...";
}

/// Extract the "key" from a line like `KEY = value` or `"key": value`
static std::string extract_key(const std::string& line) {
    auto trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    if (trimmed.empty()) return "";

    // JSON style: "key": value
    if (trimmed[0] == '"') {
        auto end = trimmed.find('"', 1);
        if (end != std::string::npos) return trimmed.substr(1, end - 1);
    }

    // KEY=VALUE style
    auto sep = trimmed.find('=');
    if (sep == std::string::npos) sep = trimmed.find(':');
    if (sep != std::string::npos) {
        auto key = trimmed.substr(0, sep);
        // Trim trailing whitespace from key
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
            key.pop_back();
        return key;
    }

    return "";
}

// ---- Pattern matchers ----

bool SecretsGuard::looks_like_key(const std::string& line) {
    static const std::vector<std::string> prefixes = {
        "sk-", "pk-",
        "AKIA",
        "ghp_", "gho_", "ghu_",
        "github_pat_",
        "xoxb-", "xoxp-",
        "sk-ant-",
        "eyJ",
    };
    for (auto& p : prefixes)
        if (line.find(p) != std::string::npos) return true;
    return false;
}

bool SecretsGuard::looks_like_token(const std::string& line) {
    static const std::vector<std::string> key_names = {
        "token", "secret", "api_key", "apikey", "auth_token",
        "access_key", "private_key",
    };
    if (!contains_any(line, key_names)) return false;

    auto eq = line.find('=');
    if (eq == std::string::npos) eq = line.find(':');
    if (eq == std::string::npos) return false;

    auto val = line.substr(eq + 1);
    val.erase(0, val.find_first_not_of(" \"'\t"));
    val.erase(val.find_last_not_of(" \"'\t") + 1);

    if (val.size() < 16) return false;
    if (val.find("://") != std::string::npos) return false;
    if (val.find('/') != std::string::npos) return false;
    if (val.find("${") != std::string::npos) return false;
    if (val.find("$(") != std::string::npos) return false;
    if (val == "localhost" || val == "127.0.0.1") return false;
    if (val.find("example") != std::string::npos) return false;
    if (val.find("xxx") != std::string::npos) return false;
    if (val.find("<") != std::string::npos) return false;

    return true;
}

bool SecretsGuard::looks_like_password(const std::string& line) {
    if (!contains_any(line, {"password", "passwd"})) return false;

    auto eq = line.find('=');
    if (eq == std::string::npos) eq = line.find(':');
    if (eq == std::string::npos) return false;

    auto val = line.substr(eq + 1);
    val.erase(0, val.find_first_not_of(" \"'\t"));
    val.erase(val.find_last_not_of(" \"'\t") + 1);

    return val.size() >= 4
        && val != "localhost"
        && val.find("example") == std::string::npos;
}

// ---- Scanning ----

void SecretsGuard::check_line(const std::string& line, int lineno,
                               std::vector<Finding>& out) {
    auto trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    if (trimmed.empty()) return;
    if (trimmed[0] == '#' || trimmed[0] == '/' || trimmed[0] == '*') return;

    if (looks_like_key(line))
        out.push_back({lineno, "API key prefix detected", truncate(line)});
    else if (looks_like_token(line))
        out.push_back({lineno, "token / secret detected", truncate(line)});
    else if (looks_like_password(line))
        out.push_back({lineno, "password detected", truncate(line)});
}

std::vector<Finding> SecretsGuard::scan_file(const fs::path& path) {
    std::vector<Finding> findings;
    if (!fs::exists(path) || fs::is_directory(path)) return findings;

    std::ifstream in(path);
    std::string line;
    int lineno = 0;
    while (std::getline(in, line))
        check_line(line, ++lineno, findings);
    return findings;
}

std::vector<Finding> SecretsGuard::scan_dir(const fs::path& path) {
    std::vector<Finding> all;
    if (!fs::exists(path)) return all;
    if (fs::is_directory(path)) {
        for (auto& e : fs::recursive_directory_iterator(path))
            if (!e.is_directory()) {
                auto f = scan_file(e.path());
                all.insert(all.end(), f.begin(), f.end());
            }
    } else {
        all = scan_file(path);
    }
    return all;
}

void SecretsGuard::report(const std::vector<Finding>& findings,
                           const fs::path& path) {
    if (findings.empty()) return;
    Reporter::warn("secrets in: " + path.string());
    for (auto& f : findings)
        std::cerr << "  line " << f.line << ": " << f.reason << "\n"
                  << "    " << f.snippet << "\n";
}

bool SecretsGuard::has_high_confidence(const std::vector<Finding>& findings) {
    return std::any_of(findings.begin(), findings.end(), [](auto& f) {
        return f.reason.find("key prefix") != std::string::npos;
    });
}

bool SecretsGuard::is_redacted(const fs::path& path) {
    if (!fs::exists(path) || fs::is_directory(path)) return false;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line))
        if (line.find(REDACTED_PLACEHOLDER) != std::string::npos) return true;
    return false;
}

// ---- Redaction ----

std::string SecretsGuard::redact_line(const std::string& line) {
    auto trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    if (trimmed.empty()) return "";
    if (trimmed[0] == '#' || trimmed[0] == '/' || trimmed[0] == '*') return "";

    if (!looks_like_key(line) && !looks_like_token(line) && !looks_like_password(line))
        return "";

    auto sep_pos = line.find('=');
    if (sep_pos == std::string::npos) sep_pos = line.find(':');
    if (sep_pos == std::string::npos) return "";

    auto after = line.find_first_of(",;#", sep_pos + 1);

    auto prefix = line.substr(0, sep_pos + 1);
    auto suffix = (after != std::string::npos) ? line.substr(after) : "";

    auto val_start = line.find_first_not_of(" \t", sep_pos + 1);
    bool quoted = (val_start != std::string::npos && line[val_start] == '"');

    if (quoted)
        return prefix + " \"__DOTRIX_REDACTED__\"" + suffix;
    else
        return prefix + " __DOTRIX_REDACTED__" + suffix;
}

int SecretsGuard::redact_file(const fs::path& src, const fs::path& dst) {
    if (!fs::exists(src)) return 0;

    std::ifstream in(src);
    std::ofstream out(dst, std::ios::trunc);
    std::string line;
    int count = 0;

    for (int lineno = 1; std::getline(in, line); ++lineno) {
        auto redacted = redact_line(line);
        if (!redacted.empty()) {
            out << redacted << "\n";
            ++count;
        } else {
            out << line << "\n";
        }
    }

    return count;
}

// ---- Merge ----

int SecretsGuard::merge_file(const fs::path& repo, const fs::path& live) {
    if (!fs::exists(repo)) return 0;

    // Build key → line map from live file
    std::map<std::string, std::string> live_by_key;
    if (fs::exists(live)) {
        std::ifstream in(live);
        std::string line;
        while (std::getline(in, line)) {
            auto key = extract_key(line);
            if (!key.empty()) live_by_key[key] = line;
        }
    }

    // Read repo, merge secrets from live
    std::ifstream repo_in(repo);
    std::vector<std::string> merged;
    std::string line;
    int preserved = 0;

    while (std::getline(repo_in, line)) {
        if (line.find(REDACTED_PLACEHOLDER) != std::string::npos) {
            auto key = extract_key(line);
            auto it = live_by_key.find(key);
            if (it != live_by_key.end()) {
                merged.push_back(it->second);
                ++preserved;
                continue;
            }
        }
        merged.push_back(line);
    }

    // Check if anything actually changed
    if (fs::exists(live)) {
        std::ifstream old_in(live);
        std::vector<std::string> old_lines;
        std::string ol;
        while (std::getline(old_in, ol)) old_lines.push_back(ol);
        if (old_lines == merged) return 0;  // no change, skip
    }

    // Back up + write
    if (fs::exists(live)) {
        auto bak = fs::path(live.string() + ".dotrix.bak");
        fs::copy_file(live, bak, fs::copy_options::overwrite_existing);
        Reporter::warn("backup: " + live.string() + " -> " + bak.string());
    }

    fs::create_directories(live.parent_path());
    std::ofstream out(live, std::ios::trunc);
    for (auto& l : merged) out << l << "\n";

    return preserved;
}

} // namespace dotrix
