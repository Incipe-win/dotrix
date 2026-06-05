#include "add.hpp"
#include "ui/reporter.hpp"
#include "util/fs.hpp"
#include "guard/secrets.hpp"
#include <sstream>

namespace dotrix {

int AddCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        Reporter::warn("usage: dotrix add <file...>");
        return 1;
    }

    git_.ensure_initialized();

    Manifest added;
    int redacted_count = 0;

    for (auto& arg : args) {
        if (arg == "--force") continue;

        fs::path src = fs::absolute(fs::path(arg));

        if (!fs::exists(src)) {
            Reporter::warn("not found: " + arg);
            continue;
        }

        auto rel = util::relative_to(src, store_.config().home);
        if (rel.find("..") == 0) {
            Reporter::warn("outside $HOME, skipping: " + arg);
            continue;
        }

        auto orig = fs::canonical(src).string();
        if (manifest_.contains(orig)) {
            Reporter::ok("already managed: " + arg);
            continue;
        }

        // ---- Secret scan ----
        auto findings = SecretsGuard::scan_dir(src);
        if (!findings.empty()) {
            SecretsGuard::report(findings, src);

            // Store redacted version to repo, live file untouched
            auto dst = store_.repo_path(orig);
            int n = 0;
            if (fs::is_directory(src)) {
                fs::create_directories(dst);
                for (auto& e : fs::recursive_directory_iterator(src)) {
                    if (!e.is_directory()) {
                        auto rel_path = fs::relative(e.path(), src);
                        n += SecretsGuard::redact_file(e.path(), dst / rel_path);
                    }
                }
            } else {
                fs::create_directories(dst.parent_path());
                n = SecretsGuard::redact_file(src, dst);
            }
            redacted_count += n;
            Reporter::ok("redacted " + std::to_string(n) + " secret(s), added: " + arg);
        } else {
            // Clean file — store as-is
            store_.store(orig);
            Reporter::ok("added: " + arg);
        }

        added.push_back({orig});
    }

    if (added.empty()) {
        Reporter::info("nothing new to add");
        return 0;
    }

    if (redacted_count > 0) {
        Reporter::warn(std::to_string(redacted_count) + " value(s) redacted — live files untouched");
    }

    manifest_.append(added);

    std::ostringstream msg;
    msg << "dotrix: add";
    for (auto& e : added) msg << " " << e.original_path;
    git_.commit(msg.str());
    Reporter::ok("committed");

    return 0;
}

} // namespace dotrix
