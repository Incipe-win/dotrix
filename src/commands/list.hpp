#pragma once
#include "command.hpp"
#include "repo/manifest.hpp"

namespace dotrix {

class ListCommand : public ICommand {
public:
    explicit ListCommand(ManifestManager& manifest) : manifest_(manifest) {}

    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "list — show managed files"; }

private:
    ManifestManager& manifest_;
};

} // namespace dotrix
