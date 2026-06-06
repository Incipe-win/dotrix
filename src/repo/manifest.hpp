#pragma once
#include "core/config.hpp"
#include "core/types.hpp"
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// JSON-backed manifest (`.dotrix/manifest.json`).
/// Auto-migrates old line-based `.dotrix/manifest` on read.
class ManifestManager {
public:
    explicit ManifestManager(const Config& cfg);

    Manifest read() const;
    void write(const Manifest& m) const;
    bool contains(const std::string& relative_path) const;
    void append(const Manifest& entries);

private:
    fs::path path_;
    fs::path legacy_path_;   // old line-based manifest — migrated if found
    void migrate_if_needed() const;
};

} // namespace dotrix
