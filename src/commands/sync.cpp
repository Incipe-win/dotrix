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
        auto repo = store_.repo_path(entry.original_path);

        if (!fs::exists(repo)) {
            Reporter::warn("not in repo: " + entry.original_path);
            continue;
        }

        // If repo contains redacted placeholders and live file exists, skip
        if (!force && SecretsGuard::is_redacted(repo)) {
            auto live = store_.live_path(entry.original_path);
            if (fs::exists(live)) {
                Reporter::warn("redacted, skipping: " + entry.original_path
                               + " (use --force to overwrite)");
                ++skipped;
                continue;
            }
            // Live file doesn't exist — deploy redacted as template
            Reporter::info("deploying template: " + entry.original_path
                           + " (fill in redacted values)");
        }

        if (!store_.dirty(entry.original_path) && !SecretsGuard::is_redacted(repo)) {
            Reporter::ok("unchanged: " + entry.original_path);
            continue;
        }

        Reporter::info("sync: " + entry.original_path);
        strategy_->deploy(entry.original_path);
        ++copied;
    }

    if (copied) Reporter::ok(std::to_string(copied) + " file(s) synced");
    if (skipped) Reporter::warn(std::to_string(skipped) + " redacted file(s) skipped (use --force)");
    if (copied == 0 && skipped == 0) Reporter::ok("everything up to date");
    return 0;
}

} // namespace dotrix
