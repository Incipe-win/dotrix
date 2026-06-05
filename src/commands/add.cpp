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

        std::string rel;
        try {
            rel = store_.to_relative(src);
        } catch (...) {
            Reporter::warn("outside $HOME, skipping: " + arg);
            continue;
        }

        if (manifest_.contains(rel)) {
            Reporter::ok("already managed: ~/" + rel);
            continue;
        }

        // ---- Secret scan ----
        auto findings = SecretsGuard::scan_dir(src);
        if (!findings.empty()) {
            SecretsGuard::report(findings, src);

            auto dst = store_.repo_path(rel);
            int n = 0;
            if (fs::is_directory(src)) {
                fs::create_directories(dst);
                for (auto& e : fs::recursive_directory_iterator(src)) {
                    if (!e.is_directory()) {
                        auto rp = fs::relative(e.path(), src);
                        n += SecretsGuard::redact_file(e.path(), dst / rp);
                    }
                }
            } else {
                fs::create_directories(dst.parent_path());
                n = SecretsGuard::redact_file(src, dst);
            }
            redacted_count += n;
            Reporter::ok("redacted " + std::to_string(n) + " secret(s), added: ~/" + rel);
        } else {
            store_.store(rel);
            Reporter::ok("added: ~/" + rel);
        }

        added.push_back({rel});
    }

    if (added.empty()) {
        Reporter::info("nothing new to add");
        return 0;
    }

    if (redacted_count > 0)
        Reporter::warn(std::to_string(redacted_count) + " value(s) redacted — live files untouched");

    manifest_.append(added);

    std::ostringstream msg;
    msg << "dotrix: add";
    for (auto& e : added) msg << " ~/" << e.relative_path;
    git_.commit(msg.str());
    Reporter::ok("committed");

    return 0;
}

} // namespace dotrix
