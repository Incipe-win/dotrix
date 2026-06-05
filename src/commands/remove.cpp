#include "remove.hpp"
#include "ui/reporter.hpp"
#include <sstream>
#include <algorithm>

namespace dotrix {

int RemoveCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        Reporter::warn("usage: dotrix remove <file...>");
        return 1;
    }

    auto manifest = manifest_.read();
    std::vector<std::string> removed;
    std::vector<std::string> keep;

    for (auto& entry : manifest) {
        // Check if this entry matches any remove target
        bool should_remove = false;
        for (auto& target : args) {
            auto resolved = fs::absolute(fs::path(target)).string();
            if (entry.original_path == resolved) {
                should_remove = true;
                break;
            }
        }

        if (should_remove) {
            removed.push_back(entry.original_path);

            // Remove from repo storage
            auto repo = store_.repo_path(entry.original_path);
            if (fs::exists(repo)) {
                fs::remove_all(repo);
            }
        } else {
            keep.push_back(entry.original_path);
        }
    }

    if (removed.empty()) {
        Reporter::warn("no matching files in manifest");
        return 1;
    }

    // Rewrite manifest
    Manifest new_manifest;
    for (auto& k : keep) new_manifest.push_back({k});
    manifest_.write(new_manifest);

    // Clean empty parent dirs in repo (best-effort)
    for (auto& path : removed) {
        auto repo = store_.repo_path(path);
        // Walk up and remove empty directories
        auto dir = repo.parent_path();
        while (dir != store_.config().repo && dir != store_.config().repo.parent_path()) {
            if (fs::exists(dir) && fs::is_directory(dir) && fs::is_empty(dir)) {
                fs::remove(dir);
                dir = dir.parent_path();
            } else {
                break;
            }
        }
    }

    for (auto& r : removed) Reporter::ok("removed: " + r);

    std::ostringstream msg;
    msg << "dotrix: remove";
    for (auto& r : removed) msg << " " << r;
    git_.commit(msg.str());
    Reporter::ok("committed");

    return 0;
}

} // namespace dotrix
