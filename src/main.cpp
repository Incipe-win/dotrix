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
#include "commands/capture.hpp"
#include "commands/scan.hpp"
#include "commands/check.hpp"
#include "commands/setup.hpp"
#include "commands/config_cmd.hpp"
#include "guard/secrets.hpp"
#include <csignal>

#include <iostream>
#include <unordered_map>
#include <functional>

namespace dotrix {

/// Build the command registry.
/// To add a new command: create a class implementing ICommand,
/// then register it here.  Everything else is automatic.
class Dispatcher {
public:
    Dispatcher(Store& store, ManifestManager& manifest, Git& git, Config& cfg) {
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

        // capture
        registry_["capture"] = std::make_unique<CaptureCommand>(store, manifest, git);

        // scan
        registry_["scan"] = std::make_unique<ScanCommand>(store, manifest);

        // check
        registry_["check"] = std::make_unique<CheckCommand>(manifest);

        // setup
        registry_["setup"] = std::make_unique<SetupCommand>();

        // config
        registry_["config"] = std::make_unique<ConfigCommand>(cfg);
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

/// Auto-track dotrix's own config files on first run.
static void auto_track_self(dotrix::Store& store, dotrix::ManifestManager& manifest,
                            dotrix::Git& git) {
    auto& cfg = store.config();
    std::vector<std::string> self_files;

    // config.json (~/.config/dotrix/config.json)
    auto config_json = cfg.config_dir / "config.json";
    if (fs::exists(config_json)) {
        try {
            auto rel = store.to_relative(config_json);
            if (!manifest.contains(rel)) self_files.push_back(rel);
        } catch (...) {}
    }

    // recipes.json (~/.config/dotrix/recipes.json)
    auto recipes_json = cfg.config_dir / "recipes.json";
    if (fs::exists(recipes_json)) {
        try {
            auto rel = store.to_relative(recipes_json);
            if (!manifest.contains(rel)) self_files.push_back(rel);
        } catch (...) {}
    }

    // Manifest is repo-internal — not tracked

    if (self_files.empty()) return;

    git.ensure_initialized();

    dotrix::Manifest added;
    for (auto& rel : self_files) {
        auto src = store.live_path(rel);
        auto dst = store.repo_path(rel);
        fs::create_directories(dst.parent_path());

        // Redact secrets in config files
        auto findings = dotrix::SecretsGuard::scan_dir(src);
        if (!findings.empty()) {
            dotrix::SecretsGuard::redact_file(src, dst);
        } else {
            store.store(rel);
        }
        added.push_back({rel});
    }
    manifest.append(added);

    std::string msg = "dotrix: auto-track self";
    for (auto& rel : self_files) msg += " ~/" + rel;
    git.commit(msg);
}

/// Signal handler — clean exit on Ctrl+C.
static void handle_sigint(int) {
    // Clean up any leftover temp scripts
    auto tmp = fs::temp_directory_path();
    for (auto& e : fs::directory_iterator(tmp)) {
        auto name = e.path().filename().string();
        if (name.starts_with("dotrix-setup-") && name.ends_with(".sh"))
            fs::remove(e.path());
    }
    _exit(130);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint);
    using namespace dotrix;

    // ---- Bootstrap ----
    auto cfg = Config::load();
    Store store(cfg);
    ManifestManager manifest(cfg);
    Git git(cfg);

    // Auto-track dotrix's own config files
    auto_track_self(store, manifest, git);

    Dispatcher dispatcher(store, manifest, git, cfg);

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
