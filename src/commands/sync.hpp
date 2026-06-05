#pragma once
#include "command.hpp"
#include "repo/store.hpp"
#include "repo/manifest.hpp"
#include "sync/strategy.hpp"
#include <memory>

namespace dotrix {

class SyncCommand : public ICommand {
public:
    SyncCommand(Store& store, ManifestManager& manifest,
                std::unique_ptr<ISyncStrategy> strategy)
        : store_(store), manifest_(manifest), strategy_(std::move(strategy)) {}

    int execute(const std::vector<std::string>& args) override;
    std::string description() const override {
        return "sync — deploy configs (" + strategy_->name() + ")";
    }

private:
    Store& store_;
    ManifestManager& manifest_;
    std::unique_ptr<ISyncStrategy> strategy_;
};

} // namespace dotrix
