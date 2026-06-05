#pragma once
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// Global configuration resolved at startup.
struct Config {
    fs::path home;       // $HOME
    fs::path repo;       // $DOTRIX_ROOT or ~/.dotfiles
    fs::path manifest;   // $REPO/.dotrix/manifest
    fs::path msg_file;   // $REPO/.dotrix/.msg (temp commit message)

    static Config load();
};

} // namespace dotrix
