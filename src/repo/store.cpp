#include "store.hpp"
#include "util/fs.hpp"
#include "ui/reporter.hpp"
#include <filesystem>

namespace dotrix {

Store::Store(const Config& cfg) : cfg_(cfg) {}

fs::path Store::live_path(const std::string& original) const {
    return fs::path(original);
}

fs::path Store::repo_path(const std::string& original) const {
    auto live = fs::path(original);
    auto rel = util::relative_to(live, cfg_.home);
    return cfg_.repo / rel;
}

void Store::store(const std::string& original) const {
    auto src = live_path(original);
    auto dst = repo_path(original);
    if (!fs::exists(src)) return;
    util::copy_tree(src, dst);
}

void Store::deploy(const std::string& original) const {
    auto src = repo_path(original);
    auto dst = live_path(original);
    if (!fs::exists(src)) return;

    // Back up existing live file before overwriting
    if (fs::exists(dst) && util::files_differ(src, dst)) {
        auto bak = fs::path(dst.string() + ".dotrix.bak");
        fs::rename(dst, bak);
        Reporter::warn("backup: " + original + " -> " + original + ".dotrix.bak");
    }

    util::copy_tree(src, dst);
}

bool Store::dirty(const std::string& original) const {
    return util::files_differ(repo_path(original), live_path(original));
}

} // namespace dotrix
