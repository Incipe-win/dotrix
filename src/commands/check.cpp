#include "check.hpp"
#include "ui/reporter.hpp"
#include "util/process.hpp"
#include <iostream>
#include <set>
#include <iomanip>
#include <algorithm>

namespace dotrix {

// ---- Path → binary name(s) mapping ----
struct Mapping {
    const char* pattern;           // substring match on the relative path
    const char* binary;            // command to check
};

static const std::vector<Mapping> KNOWN = {
    // Shell
    {".zshrc",          "zsh"},
    {".zshenv",         "zsh"},
    {".zprofile",       "zsh"},
    {".bashrc",         "bash"},
    {".bash_profile",   "bash"},
    {".profile",        "bash"},

    // Git
    {".gitconfig",      "git"},
    {".config/git/",    "git"},

    // Terminal
    {".tmux.conf",      "tmux"},

    // Editors
    {".config/nvim",    "nvim"},
    {".vimrc",          "vim"},
    {".config/zed",     "zed"},
    {".vscode-server",  "code"},
    {".vscode",         "code"},

    // Shell tools
    {".fzf.zsh",        "fzf"},
    {".fzf.bash",       "fzf"},
    {".config/ohmyposh", "oh-my-posh"},

    // Proxy
    {"clash/config",    "mihomo"},
    {".config/mihomo",  "mihomo"},

    // Languages / runtimes
    {".config/go",      "go"},
    {".cargo",          "cargo"},
    {".rustup",         "rustc"},
    {".nvm",            "node"},
    {".bun",            "bun"},
    {"bun.lockb",       "bun"},

    // Containers
    {".docker",         "docker"},

    // Other
    {".ssh/config",     "ssh"},
    {".config/git",     "git"},
};

std::vector<std::string> CheckCommand::guess_binaries(const std::string& rel) {
    std::vector<std::string> out;
    for (auto& m : KNOWN) {
        if (rel.find(m.pattern) != std::string::npos) {
            if (std::find(out.begin(), out.end(), m.binary) == out.end())
                out.push_back(m.binary);
        }
    }
    return out;  // no fallback — only use explicit mapping
}

int CheckCommand::execute(const std::vector<std::string>& /*args*/) {
    auto manifest = manifest_.read();
    if (manifest.empty()) {
        Reporter::info("nothing managed — nothing to check");
        return 0;
    }

    std::set<std::string> installed, missing, seen;

    for (auto& entry : manifest) {
        auto bins = guess_binaries(entry.relative_path);
        for (auto& bin : bins) {
            if (seen.count(bin)) continue;
            seen.insert(bin);

            if (util::capture({"which", bin}).empty())
                missing.insert(bin);
            else
                installed.insert(bin);
        }
    }

    // Print
    auto print_set = [](const std::set<std::string>& s, const char* color, const char* symbol) {
        for (auto& x : s)
            std::cout << "  " << color << symbol << "\033[0m  " << x << "\n";
    };

    if (!installed.empty()) {
        std::cout << "\033[0;32mInstalled:\033[0m\n";
        print_set(installed, "\033[0;32m", "✓");
    }

    if (!missing.empty()) {
        if (!installed.empty()) std::cout << "\n";
        std::cout << "\033[0;31mMissing:\033[0m\n";
        print_set(missing, "\033[0;31m", "✗");
    }

    if (seen.empty()) {
        std::cout << "No software detected from managed files.\n";
    } else {
        std::cout << "\n" << installed.size() << " installed, "
                  << missing.size() << " missing (from " << seen.size() << " tools)\n";
    }

    return missing.empty() ? 0 : 1;
}

} // namespace dotrix
