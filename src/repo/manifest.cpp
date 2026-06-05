#include "manifest.hpp"
#include <fstream>
#include <algorithm>
#include <cstdlib>

namespace dotrix {

ManifestManager::ManifestManager(const Config& cfg) : path_(cfg.manifest) {}

Manifest ManifestManager::read() const {
    Manifest m;
    if (!fs::exists(path_)) return m;

    auto* home = std::getenv("HOME");
    fs::path home_dir = home ? fs::path(home) : fs::path("/home") / std::getenv("USER");

    std::ifstream in(path_);
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        // Migrate old absolute paths → relative
        if (line[0] == '/') {
            try {
                line = fs::relative(line, home_dir).string();
            } catch (...) {
                continue; // outside HOME, skip
            }
        }

        m.push_back({line});
    }
    return m;
}

void ManifestManager::write(const Manifest& m) const {
    fs::create_directories(path_.parent_path());
    std::ofstream out(path_, std::ios::trunc);
    for (auto& e : m) out << e.relative_path << "\n";
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
