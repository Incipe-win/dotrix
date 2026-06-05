#include "sync.hpp"
#include "ui/reporter.hpp"
#include "guard/secrets.hpp"

namespace dotrix {

int SyncCommand::execute(const std::vector<std::string>& args) {
    bool force = false;
    for (auto& a : args)
        if (a == "--force" || a == "-f") force = true;

    auto manifest = manifest_.read();
    if (manifest.empty()) {
        Reporter::info("nothing to sync");
        return 0;
    }

    int copied = 0, skipped = 0;
    for (auto& entry : manifest) {
        auto repo = store_.repo_path(entry.relative_path);

        if (!fs::exists(repo)) {
            Reporter::warn("not in repo: ~/" + entry.relative_path);
            continue;
        }

        if (!force && SecretsGuard::is_redacted(repo)) {
            auto live = store_.live_path(entry.relative_path);
            if (fs::exists(live)) {
                Reporter::warn("redacted, skipping: ~/" + entry.relative_path
                               + " (use --force to overwrite)");
                ++skipped;
                continue;
            }
            Reporter::info("deploying template: ~/" + entry.relative_path
                           + " (fill in redacted values)");
        }

        if (!store_.dirty(entry.relative_path) && !SecretsGuard::is_redacted(repo)) {
            Reporter::ok("unchanged: ~/" + entry.relative_path);
            continue;
        }

        Reporter::info("sync: ~/" + entry.relative_path);
        strategy_->deploy(entry.relative_path);
        ++copied;
    }

    if (copied) Reporter::ok(std::to_string(copied) + " file(s) synced");
    if (skipped) Reporter::warn(std::to_string(skipped) + " redacted file(s) skipped (use --force)");
    if (copied == 0 && skipped == 0) Reporter::ok("everything up to date");
    return 0;
}

} // namespace dotrix
