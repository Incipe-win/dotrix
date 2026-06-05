#pragma once
#include "command.hpp"
#include "repo/store.hpp"
#include "repo/manifest.hpp"

namespace dotrix {

class ScanCommand : public ICommand {
public:
    ScanCommand(Store& store, ManifestManager& manifest)
        : store_(store), manifest_(manifest) {}

    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "scan — check managed files for secrets"; }

private:
    Store& store_;
    ManifestManager& manifest_;
};

} // namespace dotrix
