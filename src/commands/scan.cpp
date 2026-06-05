#include "scan.hpp"
#include "ui/reporter.hpp"
#include "guard/secrets.hpp"

namespace dotrix {

int ScanCommand::execute(const std::vector<std::string>& /*args*/) {
    auto manifest = manifest_.read();
    if (manifest.empty()) {
        Reporter::info("nothing to scan");
        return 0;
    }

    int total = 0, files = 0;
    for (auto& entry : manifest) {
        auto repo = store_.repo_path(entry.relative_path);
        if (!fs::exists(repo)) continue;

        auto findings = SecretsGuard::scan_dir(repo);
        if (!findings.empty()) {
            SecretsGuard::report(findings, entry.relative_path);
            total += findings.size();
            ++files;
        }
    }

    if (total == 0) Reporter::ok("no secrets detected");
    else {
        Reporter::warn(std::to_string(total) + " potential secret(s) in "
                       + std::to_string(files) + " file(s)");
    }
    return total > 0 ? 1 : 0;
}

} // namespace dotrix
