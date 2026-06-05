#include "remove.hpp"
#include "ui/reporter.hpp"
#include <sstream>

namespace dotrix {

int RemoveCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        Reporter::warn("usage: dotrix remove <file...>");
        return 1;
    }

    auto manifest = manifest_.read();
    std::vector<std::string> removed;
    Manifest new_manifest;

    for (auto& entry : manifest) {
        bool should_remove = false;
        for (auto& target : args) {
            // Normalize target to home-relative
            std::string rel;
            try {
                rel = store_.to_relative(fs::absolute(fs::path(target)));
            } catch (...) { continue; }

            if (entry.relative_path == rel) {
                should_remove = true;
                break;
            }
        }

        if (should_remove) {
            removed.push_back(entry.relative_path);
            auto repo = store_.repo_path(entry.relative_path);
            if (fs::exists(repo)) fs::remove_all(repo);

            // Clean empty parent dirs
            auto dir = repo.parent_path();
            while (dir != store_.config().repo && dir != store_.config().repo.parent_path()) {
                if (fs::exists(dir) && fs::is_directory(dir) && fs::is_empty(dir)) {
                    fs::remove(dir);
                    dir = dir.parent_path();
                } else break;
            }
        } else {
            new_manifest.push_back(entry);
        }
    }

    if (removed.empty()) {
        Reporter::warn("no matching files in manifest");
        return 1;
    }

    manifest_.write(new_manifest);

    for (auto& r : removed) Reporter::ok("removed: ~/" + r);

    std::ostringstream msg;
    msg << "dotrix: remove";
    for (auto& r : removed) msg << " ~/" << r;
    git_.commit(msg.str());
    Reporter::ok("committed");

    return 0;
}

} // namespace dotrix
