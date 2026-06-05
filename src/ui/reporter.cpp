#include "reporter.hpp"

#define C_RST "\033[0m"
#define C_G   "\033[0;32m"
#define C_B   "\033[0;34m"
#define C_Y   "\033[1;33m"
#define C_R   "\033[0;31m"

namespace dotrix {

void Reporter::info(const std::string& msg)  { std::cerr << C_B "[info] " C_RST << msg << "\n"; }
void Reporter::ok(const std::string& msg)    { std::cerr << C_G "[ ok ] " C_RST << msg << "\n"; }
void Reporter::warn(const std::string& msg)  { std::cerr << C_Y "[warn] " C_RST << msg << "\n"; }
void Reporter::error(const std::string& msg) { std::cerr << C_R "[err] " C_RST << msg << "\n"; }

} // namespace dotrix
