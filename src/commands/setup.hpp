#pragma once
#include "command.hpp"
#include <string>
#include <vector>

namespace dotrix {

/// dotrix setup — one-shot install of common dev tools (user-local, latest).
class SetupCommand : public ICommand {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "setup — install common dev tools"; }

private:
    struct Recipe {
        const char* name;
        const char* desc;
        const char* check;      // binary to `which`
        const char* install;    // shell command (multi-line OK). empty = skip
        bool needs_sudo;        // requires root
    };

    static const std::vector<Recipe>& recipes();
    static bool is_root();
};

} // namespace dotrix
