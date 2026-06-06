#include "config.hpp"
#include "vendor/json.hpp"
#include <fstream>
#include <cstdlib>

namespace dotrix {

static std::string env_or(const char* name, const std::string& fallback) {
    auto* v = std::getenv(name);
    return v ? std::string(v) : fallback;
}

fs::path Config::config_path() {
    auto* xdg = std::getenv("XDG_CONFIG_HOME");
    auto* home = std::getenv("HOME");
    auto base = xdg ? fs::path(xdg) : (home ? fs::path(home) / ".config" : fs::path("/tmp"));
    return base / "dotrix" / "config.json";
}

Config Config::load() {
    Config c;

    auto* h = std::getenv("HOME");
    c.home = h ? fs::path(h) : fs::path("/home") / std::getenv("USER");

    c.config_dir = c.home / ".config/dotrix";

    // Read JSON config if exists
    auto path = config_path();
    nlohmann::json j;
    if (fs::exists(path)) {
        std::ifstream in(path);
        j = nlohmann::json::parse(in, nullptr, false);
    }
    if (!j.is_object()) j = nlohmann::json::object();

    // Env var overrides file value
    c.github_token = env_or("GITHUB_TOKEN", j.value("github_token", ""));

    auto default_repo = (c.home / ".dotfiles").string();
    auto repo_val     = j.value("dotfiles_root", default_repo);
    // Expand ~/ → $HOME
    if (repo_val.starts_with("~/"))
        repo_val = c.home.string() + repo_val.substr(1);
    else if (!repo_val.starts_with("/"))
        repo_val = (c.home / repo_val).string();   // relative → absolute
    c.repo = env_or("DOTRIX_ROOT", repo_val);

    fs::create_directories(c.repo);
    c.manifest = c.repo / ".dotrix" / "manifest.json";
    c.msg_file = c.repo / ".dotrix" / ".msg";

    return c;
}

void Config::save() const {
    auto path = config_path();
    fs::create_directories(path.parent_path());

    // Store home-relative paths with ~/ prefix for portability
    auto repo_str = repo.string();
    auto home_str = home.string();
    if (repo_str.starts_with(home_str))
        repo_str = "~" + repo_str.substr(home_str.size());

    nlohmann::json j;
    j["github_token"]   = github_token;
    j["dotfiles_root"]  = repo_str;

    std::ofstream out(path);
    out << j.dump(2) << "\n";
}

} // namespace dotrix
