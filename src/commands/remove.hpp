#pragma once
#include "command.hpp"
#include "repo/store.hpp"
#include "repo/manifest.hpp"
#include "repo/git.hpp"

namespace dotrix {

class RemoveCommand : public ICommand {
public:
    RemoveCommand(Store& store, ManifestManager& manifest, Git& git)
        : store_(store), manifest_(manifest), git_(git) {}

    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "remove <file...> — stop tracking"; }

private:
    Store& store_;
    ManifestManager& manifest_;
    Git& git_;
};

} // namespace dotrix
