#include "sync.hpp"
#include "ui/reporter.hpp"

namespace dotrix {

int SyncCommand::execute(const std::vector<std::string>& /*args*/) {
    auto manifest = manifest_.read();
    if (manifest.empty()) {
        Reporter::info("nothing to sync");
        return 0;
    }

    int copied = 0;
    for (auto& entry : manifest) {
        if (!fs::exists(store_.repo_path(entry.original_path))) {
            Reporter::warn("not in repo: " + entry.original_path);
            continue;
        }

        if (!store_.dirty(entry.original_path)) {
            Reporter::ok("unchanged: " + entry.original_path);
            continue;
        }

        Reporter::info("sync: " + entry.original_path);
        strategy_->deploy(entry.original_path);
        ++copied;
    }

    if (copied) Reporter::ok(std::to_string(copied) + " file(s) synced");
    else Reporter::ok("everything up to date");
    return 0;
}

} // namespace dotrix
