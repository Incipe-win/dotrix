#include "list.hpp"
#include <iostream>

namespace dotrix {

int ListCommand::execute(const std::vector<std::string>& /*args*/) {
    auto m = manifest_.read();
    if (m.empty()) {
        std::cout << "(empty)\n";
        return 0;
    }
    for (auto& e : m) std::cout << "  ~/" << e.relative_path << "\n";
    std::cout << m.size() << " file(s)\n";
    return 0;
}

} // namespace dotrix
