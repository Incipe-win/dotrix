#pragma once
#include "command.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

/// dotrix setup — one-shot install of common dev tools (user-local, latest).
class SetupCommand : public ICommand {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "setup [--pick|--add] — install dev tools"; }

    struct Recipe {
        std::string name;
        std::string desc;
        std::string check;      // binary to `which`
        std::string install;    // shell command. empty = system tool, skip
        bool needs_sudo = false;
    };

private:
    static std::vector<Recipe> builtin_recipes();
    static std::vector<Recipe> load_custom();
    static void save_custom(const std::vector<Recipe>& recipes);
    static fs::path custom_tools_path();

    int run_list(const std::vector<Recipe>& all);
    int run_install(const std::vector<Recipe>& all,
                    const std::vector<std::string>& filter);
    int run_tui(const std::vector<Recipe>& all);
    int run_add();
};

} // namespace dotrix
