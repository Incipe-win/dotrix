#pragma once
#include "core/types.hpp"
#include "core/config.hpp"

namespace dotrix {

/// Persist and query the list of managed entries.
class ManifestManager {
public:
    explicit ManifestManager(const Config& cfg);

    Manifest read() const;
    void write(const Manifest& m) const;

    /// True if `original_path` is already tracked.
    bool contains(const std::string& original_path) const;

    /// Append new entries (deduplicate by original_path).
    void append(const Manifest& entries);

private:
    fs::path path_;
};

} // namespace dotrix
