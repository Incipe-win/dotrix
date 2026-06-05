#include "add.hpp"
#include "ui/reporter.hpp"
#include "util/fs.hpp"
#include <sstream>

namespace dotrix {

int AddCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        Reporter::warn("usage: dotrix add <file...>");
        return 1;
    }

    git_.ensure_initialized();

    Manifest added;
    for (auto& arg : args) {
        fs::path src = fs::absolute(fs::path(arg));

        if (!fs::exists(src)) {
            Reporter::warn("not found: " + arg);
            continue;
        }

        // Must be under $HOME
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

        store_.store(orig);
        added.push_back({orig});
        Reporter::ok("added: " + arg);
    }

    if (added.empty()) {
        Reporter::info("nothing new to add");
        return 0;
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
