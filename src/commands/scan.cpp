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

    int total_findings = 0;
    int files_with_secrets = 0;

    for (auto& entry : manifest) {
        auto repo = store_.repo_path(entry.original_path);
        if (!fs::exists(repo)) continue;

        auto findings = SecretsGuard::scan_dir(repo);
        if (!findings.empty()) {
            SecretsGuard::report(findings, entry.original_path);
            total_findings += findings.size();
            ++files_with_secrets;
        }
    }

    if (total_findings == 0) {
        Reporter::ok("no secrets detected");
    } else {
        Reporter::warn(std::to_string(total_findings) + " potential secret(s) in "
                       + std::to_string(files_with_secrets) + " file(s)");
        Reporter::warn("ensure the repo is PRIVATE before pushing");
    }

    return total_findings > 0 ? 1 : 0;
}

} // namespace dotrix
