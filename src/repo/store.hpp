#pragma once
#include "core/config.hpp"
#include "core/types.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// Map between relative paths and live/repo locations.
///
/// All paths stored in the manifest are relative to $HOME.
///   - live_path(".zshrc")  → /home/user/.zshrc
///   - repo_path(".zshrc")  → REPO/.zshrc
class Store {
public:
    explicit Store(const Config& cfg);

    /// Absolute live path from a home-relative path.
    fs::path live_path(const std::string& relative) const;

    /// Repo storage path from a home-relative path.
    fs::path repo_path(const std::string& relative) const;

    /// Convert an absolute path → home-relative.  Throws if not under $HOME.
    std::string to_relative(const fs::path& absolute) const;

    /// Copy from live → repo.
    void store(const std::string& relative) const;

    /// Copy from repo → live.
    void deploy(const std::string& relative) const;

    /// True if live and repo versions differ.
    bool dirty(const std::string& relative) const;

    const Config& config() const { return cfg_; }

private:
    Config cfg_;
};

} // namespace dotrix
