#include "core/config.hpp"
#include "repo/store.hpp"
#include "repo/manifest.hpp"
#include "repo/git.hpp"
#include "sync/copy_strategy.hpp"
#include "commands/add.hpp"
#include "commands/sync.hpp"
#include "commands/list.hpp"
#include "commands/status.hpp"
#include "commands/remove.hpp"

#include <iostream>
#include <unordered_map>
#include <functional>

namespace dotrix {

/// Build the command registry.
/// To add a new command: create a class implementing ICommand,
/// then register it here.  Everything else is automatic.
class Dispatcher {
public:
    Dispatcher(Store& store, ManifestManager& manifest, Git& git) {
        // add
        registry_["add"] = std::make_unique<AddCommand>(store, manifest, git);

        // sync: use copy strategy (symlink strategy can be added later)
        auto strategy = std::make_unique<CopySyncStrategy>(store);
        registry_["sync"] = std::make_unique<SyncCommand>(store, manifest, std::move(strategy));

        // list
        registry_["list"] = std::make_unique<ListCommand>(manifest);

        // status
        registry_["status"] = std::make_unique<StatusCommand>(store, manifest);

        // remove
        registry_["remove"] = std::make_unique<RemoveCommand>(store, manifest, git);
        registry_["rm"]    = std::make_unique<RemoveCommand>(store, manifest, git);
    }

    /// Dispatch a command name; returns nullptr if unknown.
    ICommand* get(const std::string& name) {
        auto it = registry_.find(name);
        return it != registry_.end() ? it->second.get() : nullptr;
    }

    void print_help(const char* prog) const {
        std::cout << "dotrix — dotfiles manager\n\n";
        for (auto& [name, cmd] : registry_) {
            std::cout << "  " << prog << " " << name;
            // pad to align descriptions
            auto pad = 22 - name.size();
            if (pad < 2) pad = 2;
            std::cout << std::string(pad, ' ') << cmd->description() << "\n";
        }
        std::cout << "\n  " << prog << " <file...>            add — track config files (shorthand)\n";
        std::cout << "\nRepo: ~/.dotfiles  (set DOTRIX_ROOT to override)\n";
    }

private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> registry_;
};

} // namespace dotrix

int main(int argc, char* argv[]) {
    using namespace dotrix;

    // ---- Bootstrap ----
    auto cfg = Config::load();
    Store store(cfg);
    ManifestManager manifest(cfg);
    Git git(cfg);
    Dispatcher dispatcher(store, manifest, git);

    // ---- Help ----
    if (argc < 2) {
        // No args — default to sync
        auto* cmd = dispatcher.get("sync");
        return cmd ? cmd->execute({}) : 1;
    }

    std::string name = argv[1];
    if (name == "help" || name == "-h" || name == "--help") {
        dispatcher.print_help(argv[0]);
        return 0;
    }

    // ---- Dispatch ----
    auto* cmd = dispatcher.get(name);
    if (cmd) {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) args.push_back(argv[i]);
        return cmd->execute(args);
    }

    // ---- Shorthand: treat all args as "add" ----
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.push_back(argv[i]);
    auto* addCmd = dispatcher.get("add");
    return addCmd ? addCmd->execute(args) : 1;
}
