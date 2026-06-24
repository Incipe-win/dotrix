#include "process.hpp"
#include <cstdio>
#include <array>

namespace dotrix::util {

bool run(const std::vector<std::string>& cmd) {
    std::string line;
    for (auto& a : cmd) line += a + " ";
    return system(line.c_str()) == 0;
}

bool run_silent(const std::vector<std::string>& cmd) {
    std::string line;
    for (auto& a : cmd) line += a + " ";
    line += " >/dev/null 2>&1";
    return system(line.c_str()) == 0;
}

std::string capture(const std::vector<std::string>& cmd) {
    std::string line, out;
    for (auto& a : cmd) line += a + " ";
    std::array<char, 4096> buf{};
    auto* p = popen(line.c_str(), "r");
    if (!p) return "";
    while (fgets(buf.data(), buf.size(), p)) out += buf.data();
    pclose(p);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

} // namespace dotrix::util
