#include "setup.hpp"
#include "ui/reporter.hpp"
#include "util/process.hpp"
#include "tui/select.hpp"
#include <iostream>
#include <set>
#include <iomanip>
#include <fstream>
#include <cstdlib>

namespace dotrix {

#define GH_API(repo) \
    "ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" " \
    "https://api.github.com/repos/" repo "/releases/latest | " \
    "grep tag_name | head -1 | cut -d'\"' -f4); " \
    "[ -n \"$ver\" ] || { echo '  (GitHub API failed — check: export GITHUB_TOKEN=ghp_xxx)'; false; }"

// ============================================================
// Built-in recipes
// ============================================================
std::vector<SetupCommand::Recipe> SetupCommand::builtin_recipes() {
    return {
    // Shell
    {"fzf",      "fuzzy finder",           "fzf",
     "[ -d $HOME/.fzf ] || { git clone --depth=1 https://github.com/junegunn/fzf.git $HOME/.fzf && "
     "$HOME/.fzf/install --all --no-update-rc; }"},

    {"ohmyposh", "prompt theme engine",    "oh-my-posh",
     "mkdir -p $HOME/.local/bin && curl -fsSL https://github.com/JanDeDobbeleer/oh-my-posh/releases/latest/download/posh-linux-amd64 -o $HOME/.local/bin/oh-my-posh && chmod +x $HOME/.local/bin/oh-my-posh"},

    {"zoxide",   "smarter cd (z command)", "zoxide",
     "curl -fsSL https://raw.githubusercontent.com/ajeetdsouza/zoxide/master/install.sh | bash 2>&1 | tail -1"},

    {"starship", "cross-shell prompt",     "starship",
     "curl -fsSL https://starship.rs/install.sh | bash 2>&1 | tail -1"},

    // Editor
    {"nvim",     "neovim editor",          "nvim",
     GH_API("neovim/neovim") " && curl -fsSL https://github.com/neovim/neovim/releases/download/$ver/nvim-linux64.tar.gz "
     "-o /tmp/nvim.tar.gz && rm -rf $HOME/.local/nvim-linux64 && tar -xzf /tmp/nvim.tar.gz -C $HOME/.local && "
     "ln -sf $HOME/.local/nvim-linux64/bin/nvim $HOME/.local/bin/nvim"},

    // File tools
    {"rg",       "ripgrep — fast grep",    "rg",
     GH_API("BurntSushi/ripgrep") " && v=${ver#v} && curl -fsSL https://github.com/BurntSushi/ripgrep/releases/download/$ver/ripgrep-$v-x86_64-unknown-linux-musl.tar.gz "
     "-o /tmp/rg.tar.gz && tar -xzf /tmp/rg.tar.gz -C /tmp && cp /tmp/ripgrep-$v-x86_64-unknown-linux-musl/rg $HOME/.local/bin/rg"},

    {"fd",       "fd — fast find",         "fd",
     GH_API("sharkdp/fd") " && curl -fsSL https://github.com/sharkdp/fd/releases/download/$ver/fd-$ver-x86_64-unknown-linux-musl.tar.gz "
     "-o /tmp/fd.tar.gz && tar -xzf /tmp/fd.tar.gz -C /tmp && cp /tmp/fd-$ver-x86_64-unknown-linux-musl/fd $HOME/.local/bin/fd"},

    {"bat",      "bat — cat with highlighting","bat",
     GH_API("sharkdp/bat") " && curl -fsSL https://github.com/sharkdp/bat/releases/download/$ver/bat-$ver-x86_64-unknown-linux-musl.tar.gz "
     "-o /tmp/bat.tar.gz && tar -xzf /tmp/bat.tar.gz -C /tmp && cp /tmp/bat-$ver-x86_64-unknown-linux-musl/bat $HOME/.local/bin/bat"},

    {"delta",    "delta — better git diff", "delta",
     GH_API("dandavison/delta") " && v=${ver#v} && curl -fsSL https://github.com/dandavison/delta/releases/download/$ver/delta-$v-x86_64-unknown-linux-gnu.tar.gz "
     "-o /tmp/delta.tar.gz && tar -xzf /tmp/delta.tar.gz -C /tmp && cp /tmp/delta-$v-x86_64-unknown-linux-gnu/delta $HOME/.local/bin/delta"},

    {"eza",      "eza — modern ls",        "eza",
     GH_API("eza-community/eza") " && curl -fsSL https://github.com/eza-community/eza/releases/download/$ver/eza_x86_64-unknown-linux-musl.tar.gz "
     "-o /tmp/eza.tar.gz && tar -xzf /tmp/eza.tar.gz -C /tmp && cp /tmp/eza $HOME/.local/bin/eza"},

    {"lazygit",  "terminal git UI",        "lazygit",
     GH_API("jesseduffield/lazygit") " && v=${ver#v} && curl -fsSL https://github.com/jesseduffield/lazygit/releases/download/$ver/lazygit_${v}_Linux_x86_64.tar.gz "
     "-o /tmp/lazygit.tar.gz && tar -xzf /tmp/lazygit.tar.gz -C /tmp lazygit && cp /tmp/lazygit $HOME/.local/bin/lazygit"},

    // C / C++
    {"gcc",      "GNU C compiler",         "gcc",      ""},
    {"g++",      "GNU C++ compiler",       "g++",      ""},
    {"clang",    "LLVM C compiler",        "clang",    ""},
    {"clangd",   "C/C++ LSP server",       "clangd",
     GH_API("clangd/clangd") " && curl -fsSL https://github.com/clangd/clangd/releases/download/$ver/clangd-linux-$ver.zip "
     "-o /tmp/clangd.zip && unzip -o /tmp/clangd.zip -d /tmp/clangd && cp /tmp/clangd/clangd_*/bin/clangd $HOME/.local/bin/clangd"},
    {"cmake",    "CMake build system",     "cmake",
     GH_API("Kitware/CMake") " && v=${ver#v} && curl -fsSL https://github.com/Kitware/CMake/releases/download/$ver/cmake-$v-linux-x86_64.tar.gz "
     "-o /tmp/cmake.tar.gz && tar -xzf /tmp/cmake.tar.gz -C /tmp && cp /tmp/cmake-$v-linux-x86_64/bin/cmake $HOME/.local/bin/cmake"},
    {"make",     "GNU Make",               "make",     ""},
    {"gdb",      "GNU debugger",           "gdb",      ""},

    // Go
    {"go",       "go compiler",            "go",
     "curl -fsSL https://go.dev/dl/go1.26.2.linux-amd64.tar.gz -o /tmp/go.tar.gz && "
     "rm -rf $HOME/.local/go && tar -C $HOME/.local -xzf /tmp/go.tar.gz"},
    {"gopls",    "Go LSP server",          "gopls",
     "GOBIN=$HOME/.local/bin go install golang.org/x/tools/gopls@latest"},
    {"delve",    "Go debugger (dlv)",      "dlv",
     "GOBIN=$HOME/.local/bin go install github.com/go-delve/delve/cmd/dlv@latest"},

    // Python
    {"python",   "Python 3",               "python3",  ""},
    {"uv",       "uv — fast Python pkg mgr","uv",
     "curl -fsSL https://astral.sh/uv/install.sh | bash 2>&1 | tail -1"},
    {"ruff",     "ruff — Python linter",   "ruff",
     "curl -fsSL https://astral.sh/ruff/install.sh | bash 2>&1 | tail -1"},
    {"pyright",  "Python type checker",    "pyright",
     "pip install --user pyright 2>/dev/null || pip3 install pyright 2>/dev/null || npm install -g pyright 2>/dev/null || true"},

    // Rust
    {"rust",     "rust via rustup",        "rustc",
     "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y"},
    {"rust-analyzer","Rust LSP server",    "rust-analyzer",
     "curl -fsSL https://github.com/rust-lang/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu "
     "-o $HOME/.local/bin/rust-analyzer && chmod +x $HOME/.local/bin/rust-analyzer"},

    // Node / JS
    {"node",     "node.js via nvm",        "node",
     "[ -d $HOME/.nvm ] || { curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash; } && "
     "export NVM_DIR=$HOME/.nvm && [ -s $NVM_DIR/nvm.sh ] && . $NVM_DIR/nvm.sh && nvm install --lts"},
    {"bun",      "bun runtime",            "bun",
     "curl -fsSL https://bun.sh/install | bash"},

    // Nvim deps
    {"xclip",    "system clipboard (X11)", "xclip",    ""},
    {"tree-sitter","tree-sitter CLI",      "tree-sitter",
     GH_API("tree-sitter/tree-sitter") " && curl -fsSL https://github.com/tree-sitter/tree-sitter/releases/download/$ver/tree-sitter-linux-x64.gz "
     "-o /tmp/ts.gz && gunzip -f /tmp/ts.gz && mv /tmp/ts $HOME/.local/bin/tree-sitter && chmod +x $HOME/.local/bin/tree-sitter"},

    // Proxy
    {"mihomo",   "proxy (clash meta)",     "mihomo",
     GH_API("MetaCubeX/mihomo") " && curl -fsSL https://github.com/MetaCubeX/mihomo/releases/download/$ver/mihomo-linux-amd64-$ver.gz "
     "-o /tmp/mihomo.gz && gunzip -f /tmp/mihomo.gz && mv /tmp/mihomo $HOME/.local/bin/mihomo && chmod +x $HOME/.local/bin/mihomo"},

    // System / Network
    {"htop",     "process viewer",         "htop",     ""},
    {"netstat",  "network statistics",     "netstat",  ""},
    {"ping",     "network reachability",   "ping",     ""},
    {"curl",     "HTTP client",            "curl",     ""},
    {"unzip",    "unzip archives",         "unzip",    ""},

    // Containers
    {"docker",   "docker engine",          "docker",
     "curl -fsSL https://get.docker.com | sh", /*needs_sudo=*/true},

    // System
    {"tmux",     "terminal multiplexer",   "tmux",     ""},
    {"zsh",      "z shell",                "zsh",      ""},
    {"git",      "git",                    "git",      ""},
    };
}

#undef GH_API

// ============================================================
// Custom tools persistence (~/.config/dotrix/tools.json)
// ============================================================
fs::path SetupCommand::custom_tools_path() {
    auto* h = std::getenv("HOME");
    return h ? fs::path(h) / ".config/dotrix/tools.json" : fs::path("/tmp/dotrix-tools.json");
}

std::vector<SetupCommand::Recipe> SetupCommand::load_custom() {
    std::vector<Recipe> out;
    auto p = custom_tools_path();
    if (!fs::exists(p)) return out;

    std::ifstream in(p);
    std::string json, line;
    while (std::getline(in, line)) json += line;
    if (json.empty()) return out;

    // Minimal JSON parser — handles the subset we need.
    // [{ "name":"x", "desc":"y", "check":"z", "install":"...", "needs_sudo":false }]
    size_t pos = 0;
    auto skip_ws = [&]{ while (pos < json.size() && (json[pos]==' '||json[pos]=='\n'||json[pos]=='\t')) ++pos; };
    auto read_str = [&]() -> std::string {
        skip_ws(); if (pos >= json.size() || json[pos] != '"') return "";
        ++pos; // skip opening "
        std::string s;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos+1 < json.size()) { ++pos; }
            s += json[pos++];
        }
        if (pos < json.size()) ++pos; // skip closing "
        return s;
    };

    skip_ws();
    if (pos >= json.size() || json[pos] != '[') return out;
    ++pos; // skip [

    while (pos < json.size()) {
        skip_ws();
        if (pos >= json.size() || json[pos] == ']') break;
        if (json[pos] == ',') { ++pos; continue; }
        if (json[pos] != '{') break;
        ++pos; // skip {

        Recipe r;
        while (pos < json.size()) {
            skip_ws();
            if (pos >= json.size() || json[pos] == '}') break;
            if (json[pos] == ',') { ++pos; continue; }
            auto key = read_str();
            skip_ws(); ++pos; // skip :
            if (key == "name")      r.name = read_str();
            else if (key == "desc") r.desc = read_str();
            else if (key == "check")r.check = read_str();
            else if (key == "install") r.install = read_str();
            else if (key == "needs_sudo") {
                skip_ws();
                r.needs_sudo = (pos < json.size() && json[pos] == 't');
                while (pos < json.size() && json[pos] != ',' && json[pos] != '}') ++pos;
            } else {
                read_str(); // skip unknown
            }
        }
        if (pos < json.size() && json[pos] == '}') ++pos;
        if (!r.name.empty()) out.push_back(std::move(r));
    }
    return out;
}

void SetupCommand::save_custom(const std::vector<Recipe>& recipes) {
    auto p = custom_tools_path();
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::trunc);
    out << "[\n";
    for (size_t i = 0; i < recipes.size(); ++i) {
        auto& r = recipes[i];
        out << "  {\n"
            << "    \"name\": \"" << r.name << "\",\n"
            << "    \"desc\": \"" << r.desc << "\",\n"
            << "    \"check\": \"" << r.check << "\",\n"
            << "    \"install\": \"" << r.install << "\",\n"
            << "    \"needs_sudo\": " << (r.needs_sudo ? "true" : "false") << "\n"
            << "  }" << (i + 1 < recipes.size() ? "," : "") << "\n";
    }
    out << "]\n";
}

// ============================================================
// Commands
// ============================================================

int SetupCommand::run_list(const std::vector<Recipe>& all) {
    int installed = 0, missing = 0;

    std::cout << "\033[0;34mAvailable tools:\033[0m\n\n";
    for (auto& t : all) {
        bool has = !util::capture({"which", std::string(t.check)}).empty();
        if (has) ++installed; else ++missing;

        std::cout << "  " << (has ? "\033[0;32m✓\033[0m" : "\033[0;31m✗\033[0m")
                  << "  " << std::left << std::setw(14) << t.name
                  << t.desc;
        if (t.needs_sudo && !t.install.empty())
            std::cout << "  (needs sudo)";
        std::cout << "\n";
    }

    std::cout << "\n" << installed << " installed, " << missing << " missing";
    if (missing > 0)
        std::cout << "    dotrix setup --pick  to select interactively";
    std::cout << "\n";
    return 0;
}

int SetupCommand::run_install(const std::vector<Recipe>& all,
                               const std::vector<std::string>& filter) {
    std::set<std::string> f(filter.begin(), filter.end());
    auto* home = std::getenv("HOME");
    fs::path local_bin = home ? fs::path(home) / ".local/bin" : fs::path("/tmp");
    fs::create_directories(local_bin);

    int installed_now = 0, skipped_sudo = 0;

    for (auto& t : all) {
        if (!f.empty() && !f.count(t.name)) continue;
        if (!util::capture({"which", std::string(t.check)}).empty()) continue; // already installed

        if (t.install.empty()) {
            Reporter::warn("skip: " + t.name + " — install via system package manager");
            continue;
        }

        if (t.needs_sudo) {
            Reporter::warn("skip: " + t.name + " — needs sudo.  Run manually:");
            std::cerr << "  " << t.install << "\n";
            ++skipped_sudo;
            continue;
        }

        Reporter::info("installing: " + t.name + " ...");

        // Write to temp script — avoids shell quoting issues with system()
        auto script = fs::temp_directory_path() / ("dotrix-setup-" + t.name + ".sh");
        {
            std::ofstream out(script);
            out << "#!/usr/bin/env bash\nset -e\n" << t.install << "\n";
        }
        fs::permissions(script, fs::perms::owner_exec, fs::perm_options::add);
        bool ok = util::run({script.string()});
        fs::remove(script);

        if (ok) {
            Reporter::ok(t.name + " installed");
            ++installed_now;
        } else {
            Reporter::error(t.name + " failed");
        }
    }

    if (installed_now) Reporter::ok(std::to_string(installed_now) + " tool(s) installed");
    if (skipped_sudo) Reporter::warn(std::to_string(skipped_sudo) + " tool(s) needs sudo — run commands above manually");

    auto path = std::getenv("PATH");
    if (path && std::string(path).find(local_bin.string()) == std::string::npos)
        Reporter::warn("$HOME/.local/bin is NOT in PATH — add: export PATH=$PATH:$HOME/.local/bin");

    return 0;
}

int SetupCommand::run_tui(const std::vector<Recipe>& all) {
    std::vector<tui::Item> items;
    for (auto& t : all) {
        bool has = !util::capture({"which", std::string(t.check)}).empty();
        items.push_back({t.name, t.desc, /*checked=*/false, has, t.needs_sudo && !t.install.empty()});
    }

    auto selected = tui::select(items, "dotrix setup");

    if (selected.empty()) {
        Reporter::info("nothing selected");
        return 0;
    }

    // Build filter list from selected indices
    std::vector<std::string> filter;
    for (int i : selected) filter.push_back(all[i].name);

    return run_install(all, filter);
}

int SetupCommand::run_add() {
    // Simple interactive prompt (no TUI needed for adding one tool)
    auto* home = std::getenv("HOME");

    Recipe r;
    std::cout << "Name (one word): ";       std::getline(std::cin, r.name);
    if (r.name.empty()) return 0;
    std::cout << "Description: ";            std::getline(std::cin, r.desc);
    std::cout << "Check command [" << r.name << "]: ";
    std::string s; std::getline(std::cin, s);
    r.check = s.empty() ? r.name : s;
    std::cout << "Install command (shell): ";std::getline(std::cin, r.install);
    std::cout << "Needs sudo? [y/N]: ";
    std::getline(std::cin, s);
    r.needs_sudo = (s == "y" || s == "Y");

    auto customs = load_custom();
    customs.push_back(r);
    save_custom(customs);

    Reporter::ok("added: " + r.name);
    Reporter::info("custom tools saved to " + custom_tools_path().string());
    if (home) Reporter::info("tip: dotrix add " + custom_tools_path().string() + "  to track it");
    return 0;
}

// ============================================================
// Execute
// ============================================================
int SetupCommand::execute(const std::vector<std::string>& args) {
    auto builtins = builtin_recipes();
    auto customs  = load_custom();

    std::vector<Recipe> all;
    all.insert(all.end(), builtins.begin(), builtins.end());
    all.insert(all.end(), customs.begin(), customs.end());

    bool run = false;
    std::string mode;
    std::vector<std::string> filter;

    for (auto& a : args) {
        if (a == "--run")  run = true;
        else if (a == "--pick") mode = "pick";
        else if (a == "--add")  mode = "add";
        else filter.push_back(a);
    }

    // --add: add a custom tool
    if (mode == "add") return run_add();

    // --pick: interactive TUI
    if (mode == "pick") return run_tui(all);

    // --run: install
    if (run) return run_install(all, filter);

    // default: list
    return run_list(all);
}

} // namespace dotrix
