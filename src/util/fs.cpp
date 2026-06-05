#include "fs.hpp"
#include <fstream>
#include <algorithm>

namespace dotrix::util {

void copy_tree(const fs::path& src, const fs::path& dst) {
    if (!fs::exists(src)) return;
    if (fs::is_directory(src)) {
        fs::create_directories(dst);
        for (auto& e : fs::directory_iterator(src))
            copy_tree(e.path(), dst / e.path().filename());
    } else {
        fs::create_directories(dst.parent_path());
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
    }
}

bool files_differ(const fs::path& a, const fs::path& b) {
    if (!fs::exists(a) || !fs::exists(b)) return true;
    bool ad = fs::is_directory(a), bd = fs::is_directory(b);
    if (ad != bd) return true;
    if (ad) {
        auto af = list_files(a), bf = list_files(b);
        if (af.size() != bf.size()) return true;
        std::sort(af.begin(), af.end());
        std::sort(bf.begin(), bf.end());
        for (size_t i = 0; i < af.size(); ++i) {
            if (af[i] != bf[i]) return true;
            if (files_differ(a / af[i], b / bf[i])) return true;
        }
        return false;
    }
    if (fs::file_size(a) != fs::file_size(b)) return true;
    std::ifstream fa(a, std::ios::binary), fb(b, std::ios::binary);
    return !std::equal(
        std::istreambuf_iterator<char>(fa.rdbuf()),
        std::istreambuf_iterator<char>(),
        std::istreambuf_iterator<char>(fb.rdbuf()));
}

std::vector<fs::path> list_files(const fs::path& root) {
    std::vector<fs::path> v;
    if (!fs::exists(root)) return v;
    for (auto& e : fs::recursive_directory_iterator(root))
        if (!e.is_directory()) v.push_back(fs::relative(e.path(), root));
    return v;
}

std::string relative_to(const fs::path& p, const fs::path& base) {
    return fs::relative(fs::canonical(p), base).string();
}

} // namespace dotrix::util
