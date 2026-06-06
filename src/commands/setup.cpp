#include "setup.hpp"
#include "ui/reporter.hpp"
#include "util/process.hpp"
#include "tui/select.hpp"
#include "vendor/json.hpp"
#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>
#include <cstdlib>

namespace dotrix {

// ============================================================
// Recipe loading — builtin from embedded JSON, custom from disk
// ============================================================

static std::vector<SetupCommand::Recipe> parse_recipes_json(const fs::path& path) {
    std::vector<SetupCommand::Recipe> out;
    if (!fs::exists(path)) return out;

    std::ifstream in(path);
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_array()) return out;

    for (auto& item : j) {
        SetupCommand::Recipe r;
        r.name       = item.value("name", "");
        r.desc       = item.value("desc", "");
        r.check      = item.value("check", r.name);
        r.install    = item.value("install", "");
        r.needs_sudo = item.value("needs_sudo", false);
        if (!r.name.empty()) out.push_back(std::move(r));
    }
    return out;
}

/// Find builtin recipes.json — shipped next to the binary.
static fs::path builtin_recipes_path() {
    try {
        auto exe = fs::read_symlink("/proc/self/exe").parent_path();
        auto p = exe / "recipes.json";
        if (fs::exists(p)) return p;
        // Fallback: source tree during development
        p = exe / "src" / "recipes.json";
        if (fs::exists(p)) return p;
    } catch (...) {}
    return {};
}

std::vector<SetupCommand::Recipe> SetupCommand::load_recipes() {
    auto out = parse_recipes_json(builtin_recipes_path());
    auto custom = parse_recipes_json(custom_recipes_path());
    out.insert(out.end(),
               std::make_move_iterator(custom.begin()),
               std::make_move_iterator(custom.end()));
    return out;
}

fs::path SetupCommand::custom_recipes_path() {
    auto* h = std::getenv("HOME");
    return h ? fs::path(h) / ".config/dotrix/recipes.json" : fs::path("/tmp/dotrix-recipes.json");
}

// ============================================================
// List / Install / TUI / Add / Remove
// ============================================================

int SetupCommand::run_list(const std::vector<Recipe>& all) {
    int installed = 0, missing = 0;

    std::cout << "\033[0;34mAvailable tools:\033[0m\n\n";
    for (auto& t : all) {
        bool has = !util::capture({"which", t.check}).empty();
        if (has) ++installed; else ++missing;

        std::cout << "  " << (has ? "\033[0;32m✓\033[0m" : "\033[0;31m✗\033[0m")
                  << "  " << std::left << std::setw(14) << t.name
                  << t.desc;
        if (t.needs_sudo && !t.install.empty())
            std::cout << "  (needs sudo)";
        std::cout << "\n";
    }

    std::cout << "\n" << installed << " installed, " << missing << " missing";
    if (missing > 0)
        std::cout << "    dotrix setup --pick  to select interactively";
    std::cout << "\n";
    return 0;
}

int SetupCommand::run_install(const std::vector<Recipe>& all,
                               const std::vector<std::string>& filter) {
    std::set<std::string> f(filter.begin(), filter.end());
    auto* home = std::getenv("HOME");
    fs::path local_bin = home ? fs::path(home) / ".local/bin" : fs::path("/tmp");
    fs::create_directories(local_bin);

    int installed_now = 0, skipped_sudo = 0;

    for (auto& t : all) {
        if (!f.empty() && !f.count(t.name)) continue;
        if (!util::capture({"which", t.check}).empty()) continue;

        if (t.install.empty()) {
            Reporter::warn("skip: " + t.name + " — system tool, no install script");
            continue;
        }

        if (t.needs_sudo) {
            Reporter::warn("skip: " + t.name + " — needs sudo. Run manually:");
            std::cerr << "  " << t.install << "\n";
            ++skipped_sudo;
            continue;
        }

        Reporter::info("installing: " + t.name + " ...");

        // Write to temp script — avoids shell quoting issues with system()
        auto script = fs::temp_directory_path() / ("dotrix-setup-" + t.name + ".sh");
        {
            std::ofstream out(script);
            out << "#!/usr/bin/env bash\nset -e\n" << t.install << "\n";
        }
        fs::permissions(script, fs::perms::owner_exec, fs::perm_options::add);
        bool ok = util::run({script.string()});
        fs::remove(script);

        if (ok) {
            Reporter::ok(t.name + " installed");
            ++installed_now;
        } else {
            Reporter::error(t.name + " failed");
        }
    }

    if (installed_now) Reporter::ok(std::to_string(installed_now) + " tool(s) installed");
    if (skipped_sudo) Reporter::warn(std::to_string(skipped_sudo) + " tool(s) needs sudo — run commands above manually");

    auto path = std::getenv("PATH");
    if (path && std::string(path).find(local_bin.string()) == std::string::npos)
        Reporter::warn("$HOME/.local/bin is NOT in PATH — add: export PATH=$PATH:$HOME/.local/bin");

    return 0;
}

int SetupCommand::run_tui(const std::vector<Recipe>& all) {
    std::vector<tui::Item> items;
    for (auto& t : all) {
        bool has = !util::capture({"which", t.check}).empty();
        items.push_back({t.name, t.desc, /*checked=*/false, has, t.needs_sudo && !t.install.empty()});
    }

    auto selected = tui::select(items, "dotrix setup");

    if (selected.empty()) {
        Reporter::info("nothing selected");
        return 0;
    }

    std::vector<std::string> filter;
    for (int i : selected) filter.push_back(all[i].name);
    return run_install(all, filter);
}

int SetupCommand::run_add() {
    Recipe r;
    std::cout << "Name (one word): ";       std::getline(std::cin, r.name);
    if (r.name.empty()) return 0;
    std::cout << "Description: ";            std::getline(std::cin, r.desc);
    std::cout << "Check command [" << r.name << "]: ";
    std::string s; std::getline(std::cin, s);
    r.check = s.empty() ? r.name : s;
    std::cout << "Install command (shell): ";std::getline(std::cin, r.install);
    std::cout << "Needs sudo? [y/N]: ";
    std::getline(std::cin, s);
    r.needs_sudo = (s == "y" || s == "Y");

    // Load existing custom recipes
    nlohmann::json j = nlohmann::json::array();
    auto path = custom_recipes_path();
    if (fs::exists(path)) {
        std::ifstream in(path);
        j = nlohmann::json::parse(in, nullptr, false);
        if (!j.is_array()) j = nlohmann::json::array();
    }

    j.push_back({
        {"name", r.name},
        {"desc", r.desc},
        {"check", r.check},
        {"install", r.install},
        {"needs_sudo", r.needs_sudo}
    });

    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    out << j.dump(2) << "\n";

    Reporter::ok("added: " + r.name);
    Reporter::info("custom recipes: " + path.string());
    auto* home = std::getenv("HOME");
    if (home) Reporter::info("tip: dotrix add " + path.string() + "  to track it");
    return 0;
}

int SetupCommand::run_rm(const std::vector<std::string>& names) {
    auto path = custom_recipes_path();
    if (!fs::exists(path) || names.empty()) {
        Reporter::warn("usage: dotrix setup --rm <name>");
        return 1;
    }

    std::ifstream in(path);
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_array()) return 1;

    nlohmann::json out_json = nlohmann::json::array();
    std::set<std::string> targets(names.begin(), names.end());
    int removed = 0;

    for (auto& item : j) {
        if (targets.count(item.value("name", ""))) {
            Reporter::ok("removed: " + item.value("name", ""));
            ++removed;
        } else {
            out_json.push_back(item);
        }
    }

    if (removed == 0) {
        Reporter::warn("no matching custom recipes");
        return 1;
    }

    std::ofstream ofs(path);
    ofs << out_json.dump(2) << "\n";
    Reporter::ok("removed " + std::to_string(removed) + " custom recipe(s)");
    return 0;
}

// ============================================================
// Execute
// ============================================================
int SetupCommand::execute(const std::vector<std::string>& args) {
    auto all = load_recipes();

    bool run = false;
    std::string mode;
    std::vector<std::string> filter;

    for (auto& a : args) {
        if (a == "--run")  run = true;
        else if (a == "--pick") mode = "pick";
        else if (a == "--add")  mode = "add";
        else if (a == "--rm")   mode = "rm";
        else filter.push_back(a);
    }

    if (mode == "add") return run_add();
    if (mode == "rm")  return run_rm(filter);
    if (mode == "pick") return run_tui(all);
    if (run) return run_install(all, filter);
    return run_list(all);
}

} // namespace dotrix
