#include "manifest.hpp"
#include <fstream>
#include <algorithm>

namespace dotrix {

ManifestManager::ManifestManager(const Config& cfg) : path_(cfg.manifest) {}

Manifest ManifestManager::read() const {
    Manifest m;
    if (!fs::exists(path_)) return m;
    std::ifstream in(path_);
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (!line.empty() && line[0] != '#')
            m.push_back({line});
    }
    return m;
}

void ManifestManager::write(const Manifest& m) const {
    fs::create_directories(path_.parent_path());
    std::ofstream out(path_, std::ios::trunc);
    for (auto& e : m) out << e.original_path << "\n";
}

bool ManifestManager::contains(const std::string& original_path) const {
    auto m = read();
    return std::any_of(m.begin(), m.end(), [&](auto& e) {
        return e.original_path == original_path;
    });
}

void ManifestManager::append(const Manifest& entries) {
    auto m = read();
    for (auto& e : entries) {
        if (!contains(e.original_path))
            m.push_back(e);
    }
    write(m);
}

} // namespace dotrix
