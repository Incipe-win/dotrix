#pragma once
#include "command.hpp"
#include "repo/manifest.hpp"
#include <string>
#include <vector>
#include <map>

namespace dotrix {

class CheckCommand : public ICommand {
public:
    explicit CheckCommand(ManifestManager& manifest) : manifest_(manifest) {}

    int execute(const std::vector<std::string>& args) override;
    std::string description() const override { return "check — which tools are installed / missing"; }

private:
    ManifestManager& manifest_;

    /// Map a managed path → binary name(s) to check.
    /// Returns empty if unknown.
    static std::vector<std::string> guess_binaries(const std::string& relative_path);
};

} // namespace dotrix
