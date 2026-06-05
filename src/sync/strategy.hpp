#pragma once
#include <string>

namespace dotrix {

/// Abstract sync strategy: how to deploy a file from repo → live.
class ISyncStrategy {
public:
    virtual ~ISyncStrategy() = default;

    /// Return human-readable name, e.g. "copy".
    virtual std::string name() const = 0;

    /// Deploy one entry.  Called only when repo and live differ.
    virtual void deploy(const std::string& original_path) = 0;
};

} // namespace dotrix
