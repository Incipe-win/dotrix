#pragma once
#include "command.hpp"
#include "core/config.hpp"

namespace dotrix {

class ConfigCommand : public ICommand {
public:
    explicit ConfigCommand(Config& cfg) : cfg_(cfg) {}

    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "config [key] [value] — get/set config"; }

private:
    Config& cfg_;
};

} // namespace dotrix
