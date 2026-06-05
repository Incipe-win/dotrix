#pragma once
#include "strategy.hpp"
#include "repo/store.hpp"

namespace dotrix {

/// Copies files from repo → live (overwrites).  Simple and zsh-compatible.
class CopySyncStrategy : public ISyncStrategy {
public:
    explicit CopySyncStrategy(const Store& store) : store_(store) {}

    std::string name() const override { return "copy"; }

    void deploy(const std::string& original_path) override {
        store_.deploy(original_path);
    }

private:
    const Store& store_;
};

} // namespace dotrix
