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

    int copied = 0, merged = 0, skipped = 0;
    for (auto& entry : manifest) {
        auto repo = store_.repo_path(entry.relative_path);

        if (!fs::exists(repo)) {
            Reporter::warn("not in repo: ~/" + entry.relative_path);
            continue;
        }

        auto live = store_.live_path(entry.relative_path);

        // If repo has redacted placeholders, merge: new content from repo,
        // real secrets from live.
        if (SecretsGuard::is_redacted(repo)) {
            if (!fs::exists(live)) {
                // New machine — deploy template
                Reporter::info("deploying template: ~/" + entry.relative_path
                               + " (fill in __DOTRIX_REDACTED__ values)");
                strategy_->deploy(entry.relative_path);
                ++copied;
                continue;
            }

            // Live exists — merge: repo content + live secrets
            int n = SecretsGuard::merge_file(repo, live);
            if (n > 0) {
                Reporter::ok("merged: ~/" + entry.relative_path
                             + " (" + std::to_string(n) + " secrets preserved)");
                ++merged;
            } else {
                Reporter::ok("unchanged: ~/" + entry.relative_path);
            }
            continue;
        }

        // Non-redacted file — normal sync
        if (!force && !store_.dirty(entry.relative_path)) {
            Reporter::ok("unchanged: ~/" + entry.relative_path);
            continue;
        }

        Reporter::info("sync: ~/" + entry.relative_path);
        strategy_->deploy(entry.relative_path);
        ++copied;
    }

    if (copied)  Reporter::ok(std::to_string(copied) + " file(s) synced");
    if (merged)  Reporter::ok(std::to_string(merged) + " file(s) merged (secrets preserved)");
    if (skipped) Reporter::warn(std::to_string(skipped) + " file(s) skipped");
    if (copied == 0 && merged == 0 && skipped == 0)
        Reporter::ok("everything up to date");
    return 0;
}

} // namespace dotrix
