#include "store.hpp"
#include "util/fs.hpp"
#include "ui/reporter.hpp"
#include <filesystem>

namespace dotrix {

Store::Store(const Config& cfg) : cfg_(cfg) {}

fs::path Store::live_path(const std::string& relative) const {
    return cfg_.home / relative;
}

fs::path Store::repo_path(const std::string& relative) const {
    return cfg_.repo / relative;
}

std::string Store::to_relative(const fs::path& absolute) const {
    return fs::relative(fs::canonical(absolute), cfg_.home).string();
}

void Store::store(const std::string& relative) const {
    auto src = live_path(relative);
    auto dst = repo_path(relative);
    if (!fs::exists(src)) return;
    util::copy_tree(src, dst);
}

void Store::deploy(const std::string& relative) const {
    auto src = repo_path(relative);
    auto dst = live_path(relative);
    if (!fs::exists(src)) return;

    // Back up existing live file before overwriting
    if (fs::exists(dst) && util::files_differ(src, dst)) {
        auto bak = fs::path(dst.string() + ".dotrix.bak");
        fs::rename(dst, bak);
        Reporter::warn("backup: ~/" + relative + " -> ~/" + relative + ".dotrix.bak");
    }

    util::copy_tree(src, dst);
}

bool Store::dirty(const std::string& relative) const {
    return util::files_differ(repo_path(relative), live_path(relative));
}

} // namespace dotrix
