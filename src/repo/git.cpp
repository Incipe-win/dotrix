#include "git.hpp"
#include "ui/reporter.hpp"
#include "util/process.hpp"
#include <fstream>

namespace dotrix {

Git::Git(const Config& cfg) : repo_(cfg.repo), msg_file_(cfg.msg_file) {}

void Git::ensure_initialized() {
    if (fs::exists(repo_ / ".git")) return;
    Reporter::info("init: " + repo_.string());
    util::run({"git", "-C", repo_.string(), "init"});

    // Write default .gitignore for the managed repo
    std::ofstream gi(repo_ / ".gitignore", std::ios::trunc);
    gi << "# dotrix internals\n"
       << ".dotrix/.msg\n"
       << "# Sensitive — uncomment if pushing to a public repo:\n"
       << "# clash/config.yaml\n";
    gi.close();
}

void Git::configure_user() {
    auto name = util::capture({"git", "-C", repo_.string(), "config", "--local", "user.name"});
    if (!name.empty()) return;

    name = util::capture({"git", "config", "--global", "user.name"});
    auto email = util::capture({"git", "config", "--global", "user.email"});
    if (name.empty()) name = std::getenv("USER") ? std::getenv("USER") : "user";
    if (email.empty()) email = name + "@localhost";

    util::run({"git", "-C", repo_.string(), "config", "user.name", name});
    util::run({"git", "-C", repo_.string(), "config", "user.email", email});
}

void Git::commit(const std::string& message) {
    configure_user();
    util::run({"git", "-C", repo_.string(), "add", "-A"});

    fs::create_directories(msg_file_.parent_path());
    std::ofstream out(msg_file_, std::ios::trunc);
    out << message << "\n";
    out.close();

    util::run({"git", "-C", repo_.string(), "commit", "-F", msg_file_.string()});
}

} // namespace dotrix
