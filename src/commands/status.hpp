#pragma once
#include "command.hpp"
#include "repo/store.hpp"
#include "repo/manifest.hpp"

namespace dotrix {

class StatusCommand : public ICommand {
public:
    StatusCommand(Store& store, ManifestManager& manifest)
        : store_(store), manifest_(manifest) {}

    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "status — show changed files"; }

private:
    Store& store_;
    ManifestManager& manifest_;
};

} // namespace dotrix
