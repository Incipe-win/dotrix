#pragma once
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// dotrix configuration — loaded from ~/.config/dotrix/config.json
/// Env vars override file values.
struct Config {
    fs::path home;           // $HOME
    fs::path repo;           // managed dotfiles repo  (default: ~/.dotfiles)
    fs::path config_dir;     // ~/.config/dotrix/
    fs::path manifest;       // $repo/.dotrix/manifest.json
    fs::path msg_file;       // $repo/.dotrix/.msg

    std::string github_token;  // for GitHub API
    // extensible: add fields here, read from JSON automatically

    static Config load();
    void save() const;         // persist to ~/.config/dotrix/config.json

private:
    static fs::path config_path();
};

} // namespace dotrix
