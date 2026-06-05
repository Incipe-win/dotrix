#include "capture.hpp"
#include "ui/reporter.hpp"
#include <sstream>

namespace dotrix {

int CaptureCommand::execute(const std::vector<std::string>& /*args*/) {
    auto manifest = manifest_.read();
    if (manifest.empty()) {
        Reporter::info("nothing to capture");
        return 0;
    }

    std::vector<std::string> updated;
    for (auto& entry : manifest) {
        auto live = store_.live_path(entry.relative_path);

        if (!fs::exists(live)) {
            Reporter::warn("missing on disk, skipping: ~/" + entry.relative_path);
            continue;
        }

        if (!store_.dirty(entry.relative_path))
            continue;

        Reporter::info("capture: ~/" + entry.relative_path);
        store_.store(entry.relative_path);
        updated.push_back(entry.relative_path);
    }

    if (updated.empty()) {
        Reporter::ok("everything up to date");
        return 0;
    }

    std::ostringstream msg;
    msg << "dotrix: capture";
    for (auto& u : updated) msg << " ~/" << u;
    git_.commit(msg.str());
    Reporter::ok("captured " + std::to_string(updated.size()) + " file(s)");

    return 0;
}

} // namespace dotrix
