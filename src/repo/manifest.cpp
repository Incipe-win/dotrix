#include "manifest.hpp"
#include "vendor/json.hpp"
#include <fstream>
#include <algorithm>
#include <cstdlib>

namespace dotrix {

ManifestManager::ManifestManager(const Config& cfg)
    : path_(cfg.manifest.parent_path() / "manifest.json")
    , legacy_path_(cfg.manifest)   // old .dotrix/manifest (line-based)
{
    migrate_if_needed();
}

void ManifestManager::migrate_if_needed() const {
    // If new JSON manifest exists, nothing to do
    if (fs::exists(path_)) return;

    // If old line-based manifest exists, migrate it
    if (!fs::exists(legacy_path_)) return;

    auto m = Manifest{};
    auto* home = std::getenv("HOME");
    fs::path home_dir = home ? fs::path(home) : fs::path("/home") / std::getenv("USER");

    std::ifstream in(legacy_path_);
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        // Convert old absolute path → relative
        if (line[0] == '/') {
            try { line = fs::relative(line, home_dir).string(); }
            catch (...) { continue; }
        }
        m.push_back({line});
    }

    if (!m.empty()) {
        fs::create_directories(path_.parent_path());
        nlohmann::json j = nlohmann::json::array();
        for (auto& e : m) j.push_back(e.relative_path);
        std::ofstream out(path_);
        out << j.dump(2) << "\n";
    }

    // Remove old manifest
    fs::remove(legacy_path_);
}

Manifest ManifestManager::read() const {
    migrate_if_needed();

    Manifest m;
    if (!fs::exists(path_)) return m;

    std::ifstream in(path_);
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (j.is_array()) {
        for (auto& item : j) {
            if (item.is_string())
                m.push_back({item.get<std::string>()});
        }
    }
    return m;
}

void ManifestManager::write(const Manifest& m) const {
    fs::create_directories(path_.parent_path());
    nlohmann::json j = nlohmann::json::array();
    for (auto& e : m) j.push_back(e.relative_path);
    std::ofstream out(path_);
    out << j.dump(2) << "\n";
}

bool ManifestManager::contains(const std::string& relative_path) const {
    auto m = read();
    return std::any_of(m.begin(), m.end(), [&](auto& e) {
        return e.relative_path == relative_path;
    });
}

void ManifestManager::append(const Manifest& entries) {
    auto m = read();
    for (auto& e : entries) {
        if (!contains(e.relative_path))
            m.push_back(e);
    }
    write(m);
}

} // namespace dotrix
