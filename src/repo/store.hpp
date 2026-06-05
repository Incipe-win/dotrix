#pragma once
#include "core/config.hpp"
#include "core/types.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// Map between original (live) paths and repo (storage) paths.
class Store {
public:
    explicit Store(const Config& cfg);

    /// Live path from an original_path, e.g. "/home/user/.zshrc"
    fs::path live_path(const std::string& original) const;

    /// Repo path from an original_path, e.g. "/home/user/.zshrc" -> REPO/.zshrc
    fs::path repo_path(const std::string& original) const;

    /// Copy from live -> repo (store a file/dir).
    void store(const std::string& original) const;

    /// Copy from repo -> live (deploy a file/dir).
    void deploy(const std::string& original) const;

    /// True if live and repo versions differ.
    bool dirty(const std::string& original) const;

    const Config& config() const { return cfg_; }

private:
    Config cfg_;
};

} // namespace dotrix
