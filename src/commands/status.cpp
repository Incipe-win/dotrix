#include "status.hpp"
#include "ui/reporter.hpp"
#include <iostream>

namespace dotrix {

int StatusCommand::execute(const std::vector<std::string>& /*args*/) {
    auto m = manifest_.read();
    int dirty = 0;

    for (auto& e : m) {
        auto live = store_.live_path(e.relative_path);
        auto repo = store_.repo_path(e.relative_path);

        if (!fs::exists(live)) {
            std::cout << "  D  ~/" << e.relative_path << "  (missing)\n"; ++dirty;
        } else if (!fs::exists(repo)) {
            std::cout << "  A  ~/" << e.relative_path << "  (not in repo)\n"; ++dirty;
        } else if (store_.dirty(e.relative_path)) {
            std::cout << "  M  ~/" << e.relative_path << "  (modified)\n"; ++dirty;
        }
    }

    if (dirty == 0) Reporter::ok("clean");
    else Reporter::warn(std::to_string(dirty) + " file(s) differ");
    return dirty > 0 ? 1 : 0;
}

} // namespace dotrix
