#pragma once
#include "command.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace dotrix {

class SetupCommand : public ICommand {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "setup [--pick|--add] — install dev tools"; }

    struct Recipe {
        std::string name, desc, check, install;
        bool needs_sudo = false;
    };

private:
    static std::vector<Recipe> load_recipes();
    static fs::path recipes_path();

    int run_list(const std::vector<Recipe>& all);
    int run_install(const std::vector<Recipe>& all,
                    const std::vector<std::string>& filter);
    int run_tui(const std::vector<Recipe>& all);
    int run_add();
    int run_rm(const std::vector<std::string>& args);
};

} // namespace dotrix
