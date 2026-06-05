#include "config.hpp"
#include <cstdlib>

namespace dotrix {

Config Config::load() {
    Config c;

    auto* h = std::getenv("HOME");
    c.home = h ? fs::path(h) : fs::path("/home") / std::getenv("USER");

    if (auto* e = std::getenv("DOTRIX_ROOT")) c.repo = e;
    else c.repo = c.home / ".dotfiles";

    fs::create_directories(c.repo);
    c.manifest = c.repo / ".dotrix" / "manifest";
    c.msg_file = c.repo / ".dotrix" / ".msg";
    return c;
}

} // namespace dotrix
