#pragma once
#include "core/config.hpp"
#include <string>

namespace dotrix {

/// Git operations scoped to the dotfiles repo.
class Git {
public:
    explicit Git(const Config& cfg);

    /// Ensure the repo directory is a git repo (init if needed).
    void ensure_initialized();

    /// Stage all and commit with the given message.
    void commit(const std::string& message);

private:
    fs::path repo_;
    fs::path msg_file_;

    void configure_user();
};

} // namespace dotrix
