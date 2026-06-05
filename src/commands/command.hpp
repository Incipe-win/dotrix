#pragma once
#include <string>
#include <vector>
#include <memory>

namespace dotrix {

/// Interface for a CLI subcommand.
class ICommand {
public:
    virtual ~ICommand() = default;

    /// Execute the command.  Returns exit code (0 = success).
    virtual int execute(const std::vector<std::string>& args) = 0;

    /// One-line description for help text.
    virtual std::string description() const = 0;
};

using CommandPtr = std::unique_ptr<ICommand>;

} // namespace dotrix
