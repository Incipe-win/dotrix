#include "setup.hpp"
#include "ui/reporter.hpp"
#include "util/process.hpp"
#include <iostream>
#include <set>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <unistd.h>

namespace dotrix {

// ============================================================
// Recipe database — one entry per tool.
// install = "" means "no automatic install, system tool".
// ============================================================
// Helper macro for GitHub latest-release downloads
#define GH_API(repo) \
    "ver=$(curl -sL ${GITHUB_TOKEN:+-H \"Authorization: Bearer $GITHUB_TOKEN\"} " \
    "https://api.github.com/repos/" repo "/releases/latest | " \
    "grep tag_name | head -1 | cut -d'\"' -f4) && [ -n \"$ver\" ]"

const std::vector<SetupCommand::Recipe>& SetupCommand::recipes() {
    static const std::vector<Recipe> r = {

    // ========== Shell tools ==========
    {"fzf",      "fuzzy finder",
     "fzf",
     "[ -d $HOME/.fzf ] || { git clone --depth=1 https://github.com/junegunn/fzf.git $HOME/.fzf && "
     "$HOME/.fzf/install --all --no-update-rc; }"},

    {"ohmyposh", "prompt theme engine",
     "oh-my-posh",
     "mkdir -p $HOME/.local/bin && "
     "curl -fsSL https://github.com/JanDeDobbeleer/oh-my-posh/releases/latest/download/"
     "posh-linux-amd64 -o $HOME/.local/bin/oh-my-posh && chmod +x $HOME/.local/bin/oh-my-posh"},

    {"zoxide",   "smarter cd (z command)",
     "zoxide",
     "curl -fsSL https://raw.githubusercontent.com/ajeetdsouza/zoxide/master/install.sh | bash"},

    {"starship", "cross-shell prompt",
     "starship",
     "curl -fsSL https://starship.rs/install.sh | bash"},

    // ========== Editors ==========
    {"nvim",     "neovim editor",
     "nvim",
     GH_API("neovim/neovim") " && "
     "curl -fsSL https://github.com/neovim/neovim/releases/download/$ver/nvim-linux64.tar.gz "
     "-o /tmp/nvim.tar.gz && rm -rf $HOME/.local/nvim-linux64 && "
     "tar -xzf /tmp/nvim.tar.gz -C $HOME/.local && "
     "ln -sf $HOME/.local/nvim-linux64/bin/nvim $HOME/.local/bin/nvim"},

    // ========== File tools (search / view / find) ==========
    {"rg",       "ripgrep — fast grep",
     "rg",
     GH_API("BurntSushi/ripgrep") " && v=${ver#v} && "
     "curl -fsSL https://github.com/BurntSushi/ripgrep/releases/download/$ver/ripgrep-$v-x86_64-unknown-linux-musl.tar.gz "
     "-o /tmp/rg.tar.gz && tar -xzf /tmp/rg.tar.gz -C /tmp && "
     "cp /tmp/ripgrep-$v-x86_64-unknown-linux-musl/rg $HOME/.local/bin/rg"},

    {"fd",       "fd — fast find",
     "fd",
     GH_API("sharkdp/fd") " && "
     "curl -fsSL https://github.com/sharkdp/fd/releases/download/$ver/fd-$ver-x86_64-unknown-linux-musl.tar.gz "
     "-o /tmp/fd.tar.gz && tar -xzf /tmp/fd.tar.gz -C /tmp && "
     "cp /tmp/fd-$ver-x86_64-unknown-linux-musl/fd $HOME/.local/bin/fd"},

    {"bat",      "bat — cat with syntax highlighting",
     "bat",
     GH_API("sharkdp/bat") " && "
     "curl -fsSL https://github.com/sharkdp/bat/releases/download/$ver/bat-$ver-x86_64-unknown-linux-musl.tar.gz "
     "-o /tmp/bat.tar.gz && tar -xzf /tmp/bat.tar.gz -C /tmp && "
     "cp /tmp/bat-$ver-x86_64-unknown-linux-musl/bat $HOME/.local/bin/bat"},

    {"delta",    "delta — better git diff pager",
     "delta",
     GH_API("dandavison/delta") " && v=${ver#v} && "
     "curl -fsSL https://github.com/dandavison/delta/releases/download/$ver/delta-$v-x86_64-unknown-linux-gnu.tar.gz "
     "-o /tmp/delta.tar.gz && tar -xzf /tmp/delta.tar.gz -C /tmp && "
     "cp /tmp/delta-$v-x86_64-unknown-linux-gnu/delta $HOME/.local/bin/delta"},

    {"eza",      "eza — modern ls replacement",
     "eza",
     GH_API("eza-community/eza") " && "
     "curl -fsSL https://github.com/eza-community/eza/releases/download/$ver/eza_x86_64-unknown-linux-musl.tar.gz "
     "-o /tmp/eza.tar.gz && tar -xzf /tmp/eza.tar.gz -C /tmp && "
     "cp /tmp/eza $HOME/.local/bin/eza"},

    {"lazygit",  "terminal git UI",
     "lazygit",
     GH_API("jesseduffield/lazygit") " && v=${ver#v} && "
     "curl -fsSL https://github.com/jesseduffield/lazygit/releases/download/$ver/lazygit_${v}_Linux_x86_64.tar.gz "
     "-o /tmp/lazygit.tar.gz && tar -xzf /tmp/lazygit.tar.gz -C /tmp lazygit && "
     "cp /tmp/lazygit $HOME/.local/bin/lazygit"},

    // ========== C / C++ ==========
    {"gcc",      "GNU C compiler",
     "gcc",      "", /*needs_sudo=*/true},

    {"g++",      "GNU C++ compiler",
     "g++",      "", /*needs_sudo=*/true},

    {"clang",    "LLVM C compiler",
     "clang",    "", /*needs_sudo=*/true},

    {"clangd",   "C/C++ LSP server",
     "clangd",
     GH_API("clangd/clangd") " && "
     "curl -fsSL https://github.com/clangd/clangd/releases/download/$ver/clangd-linux-$ver.zip "
     "-o /tmp/clangd.zip && unzip -o /tmp/clangd.zip -d /tmp/clangd && "
     "cp /tmp/clangd/clangd_*/bin/clangd $HOME/.local/bin/clangd"},

    {"cmake",    "CMake build system",
     "cmake",
     GH_API("Kitware/CMake") " && v=${ver#v} && "
     "curl -fsSL https://github.com/Kitware/CMake/releases/download/$ver/cmake-$v-linux-x86_64.tar.gz "
     "-o /tmp/cmake.tar.gz && tar -xzf /tmp/cmake.tar.gz -C /tmp && "
     "cp /tmp/cmake-$v-linux-x86_64/bin/cmake $HOME/.local/bin/cmake"},

    {"make",     "GNU Make",
     "make",     "", /*needs_sudo=*/true},

    {"gdb",      "GNU debugger",
     "gdb",      "", /*needs_sudo=*/true},

    // ========== Go ==========
    {"go",       "go compiler",
     "go",
     "curl -fsSL https://go.dev/dl/go1.26.2.linux-amd64.tar.gz -o /tmp/go.tar.gz && "
     "rm -rf $HOME/.local/go && tar -C $HOME/.local -xzf /tmp/go.tar.gz"},

    {"gopls",    "Go LSP server",
     "gopls",
     "GOBIN=$HOME/.local/bin go install golang.org/x/tools/gopls@latest"},

    {"delve",    "Go debugger (dlv)",
     "dlv",
     "GOBIN=$HOME/.local/bin go install github.com/go-delve/delve/cmd/dlv@latest"},

    // ========== Python ==========
    {"python",   "Python 3",
     "python3",  "", /*needs_sudo=*/true},

    {"uv",       "uv — fast Python package manager",
     "uv",
     "curl -fsSL https://astral.sh/uv/install.sh | bash"},

    {"ruff",     "ruff — fast Python linter",
     "ruff",
     "curl -fsSL https://astral.sh/ruff/install.sh | bash"},

    {"pyright",  "Python type checker (LSP)",
     "pyright",
     "npm install -g pyright 2>/dev/null || pip3 install pyright 2>/dev/null || "
     "echo 'install manually: npm i -g pyright'"},

    // ========== Rust ==========
    {"rust",     "rust via rustup",
     "rustc",
     "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y"},

    {"rust-analyzer", "Rust LSP server",
     "rust-analyzer",
     GH_API("rust-lang/rust-analyzer") " && "
     "curl -fsSL https://github.com/rust-lang/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu "
     "-o $HOME/.local/bin/rust-analyzer && chmod +x $HOME/.local/bin/rust-analyzer"},

    // ========== Node / JS ==========
    {"node",     "node.js via nvm",
     "node",
     "[ -d $HOME/.nvm ] || { curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash; } && "
     "export NVM_DIR=$HOME/.nvm && [ -s $NVM_DIR/nvm.sh ] && . $NVM_DIR/nvm.sh && nvm install --lts"},

    {"bun",      "bun runtime",
     "bun",
     "curl -fsSL https://bun.sh/install | bash"},

    // ========== Nvim checkhealth dependencies ==========
    {"xclip",    "system clipboard (X11)",
     "xclip",    "", /*needs_sudo=*/true},

    {"wl-clipboard","system clipboard (Wayland)",
     "wl-copy",  "", /*needs_sudo=*/true},

    {"tree-sitter","tree-sitter CLI",
     "tree-sitter",
     GH_API("tree-sitter/tree-sitter") " && "
     "curl -fsSL https://github.com/tree-sitter/tree-sitter/releases/download/$ver/tree-sitter-linux-x64.gz "
     "-o /tmp/ts.gz && gunzip -f /tmp/ts.gz && mv /tmp/ts $HOME/.local/bin/tree-sitter && chmod +x $HOME/.local/bin/tree-sitter"},

    // ========== Proxy ==========
    {"mihomo",   "proxy (clash meta)",
     "mihomo",
     GH_API("MetaCubeX/mihomo") " && "
     "curl -fsSL https://github.com/MetaCubeX/mihomo/releases/download/$ver/mihomo-linux-amd64-$ver.gz "
     "-o /tmp/mihomo.gz && gunzip -f /tmp/mihomo.gz && "
     "mv /tmp/mihomo $HOME/.local/bin/mihomo && chmod +x $HOME/.local/bin/mihomo"},

    // ========== Network / System ==========
    {"htop",     "interactive process viewer",
     "htop",     "", /*needs_sudo=*/true},

    {"netstat",  "network statistics",
     "netstat",  "", /*needs_sudo=*/true},

    {"ping",     "network reachability",
     "ping",     "", /*needs_sudo=*/true},

    {"curl",     "HTTP client",
     "curl",     "", /*needs_sudo=*/true},

    {"unzip",    "unzip archives",
     "unzip",    "", /*needs_sudo=*/true},

    // ========== Containers ==========
    {"docker",   "docker engine",
     "docker",
     "curl -fsSL https://get.docker.com | sh",
     /*needs_sudo=*/true},

    // ========== System (no user-local install) ==========
    {"tmux",     "terminal multiplexer",
     "tmux",     ""},

    {"zsh",      "z shell",
     "zsh",      ""},

    {"git",      "git",
     "git",      ""},
    };
    return r;
}

#undef GH_API

bool SetupCommand::is_root() {
    return geteuid() == 0;
}

int SetupCommand::execute(const std::vector<std::string>& args) {
    bool run = false;
    std::set<std::string> filter;

    for (auto& a : args) {
        if (a == "--run") run = true;
        else filter.insert(a);
    }

    // Ensure ~/.local/bin exists
    auto* home = std::getenv("HOME");
    fs::path local_bin = home ? fs::path(home) / ".local/bin" : fs::path("/tmp");
    fs::create_directories(local_bin);

    // Check PATH
    auto path = std::getenv("PATH");
    bool bin_in_path = path && std::string(path).find(local_bin.string()) != std::string::npos;

    auto& tools = recipes();

    int installed = 0, missing = 0, skipped_sudo = 0, installed_now = 0;

    // ---- List ----
    if (!run) {
        std::cout << "\033[0;34mAvailable tools:\033[0m\n\n";
    }

    for (auto& t : tools) {
        if (!filter.empty() && !filter.count(t.name)) continue;

        bool has = !util::capture({"which", std::string(t.check)}).empty();

        if (!run) {
            std::cout << "  " << (has ? "\033[0;32m✓\033[0m" : "\033[0;31m✗\033[0m")
                      << "  " << std::left << std::setw(14) << t.name
                      << t.desc;
            if (t.needs_sudo && t.install && *t.install)
                std::cout << "  (needs sudo)";
            std::cout << "\n";
        }

        if (has) { ++installed; continue; }

        // Tool is missing
        if (!run) { ++missing; continue; }

        // ---- Install ----
        auto name = std::string(t.name);
        auto install_cmd = std::string(t.install);

        if (install_cmd.empty()) {
            Reporter::warn("skip: " + name + " — install via system package manager");
            ++missing;
            continue;
        }

        // Tools requiring sudo: print command, don't auto-run.
        // sudo dotrix won't find the binary; safer to let the user run it.
        if (t.needs_sudo) {
            Reporter::warn("skip: " + name + " — needs sudo.  Run manually:");
            std::cerr << "  " << install_cmd << "\n";
            ++skipped_sudo;
            continue;
        }

        Reporter::info("installing: " + name + " ...");
        bool ok = util::run({"bash", "-c", install_cmd});
        if (ok) {
            Reporter::ok(name + " installed");
            ++installed_now;
        } else {
            Reporter::error(name + " failed");
        }
    }

    // ---- Summary ----
    if (!run) {
        std::cout << "\n" << installed << " installed, " << missing << " missing\n";
        if (missing > 0)
            std::cout << "Run:  dotrix setup --run    to install all missing tools\n";
        std::cout << "      dotrix setup --run " << (filter.empty() ? "<name>" : "") << "  to install specific ones\n";
        if (!bin_in_path)
            Reporter::warn("$HOME/.local/bin is NOT in PATH — add it to your shell config");
    } else {
        std::cout << "\n";
        if (installed_now > 0) Reporter::ok(std::to_string(installed_now) + " tool(s) installed");
        if (skipped_sudo > 0) Reporter::warn(std::to_string(skipped_sudo) + " tool(s) skipped (needs sudo)");
        if (!bin_in_path)
            Reporter::warn("$HOME/.local/bin is NOT in PATH — add: export PATH=\\$PATH:\\$HOME/.local/bin");
    }

    return 0;
}

} // namespace dotrix
