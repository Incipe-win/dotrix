#pragma once
#include <string>
#include <iostream>

namespace dotrix {

/// Consistent output formatting.
class Reporter {
public:
    static void info(const std::string& msg);
    static void ok(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);
};

} // namespace dotrix
