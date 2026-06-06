#include "config_cmd.hpp"
#include <iostream>

namespace dotrix {

int ConfigCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        // Show all
        std::cout << "dotrix config\n"
                  << "  dotfiles_root: " << cfg_.repo << "\n"
                  << "  github_token:  ";
        if (cfg_.github_token.empty()) std::cout << "(not set)\n";
        else std::cout << cfg_.github_token.substr(0, 4) << "*** (use 'dotrix config token')\n";
        std::cout << "  config_dir:    " << cfg_.config_dir << "\n";
        return 0;
    }

    auto key = args[0];
    auto val = args.size() > 1 ? args[1] : "";

    if (key == "github_token" || key == "token") {
        if (val.empty()) {
            std::cout << cfg_.github_token << "\n";
        } else {
            cfg_.github_token = val;
            cfg_.save();
            std::cout << "github_token saved\n";
        }
    } else if (key == "dotfiles_root" || key == "root") {
        if (val.empty()) {
            std::cout << cfg_.repo << "\n";
        } else {
            cfg_.repo = val;
            cfg_.save();
            std::cout << "dotfiles_root saved\n";
        }
    } else {
        std::cerr << "unknown key: " << key << "\n";
        std::cerr << "valid keys: github_token, dotfiles_root\n";
        return 1;
    }

    return 0;
}

} // namespace dotrix
