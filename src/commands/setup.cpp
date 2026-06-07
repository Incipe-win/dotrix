#include "setup.hpp"
#include "core/config.hpp"
#include "ui/reporter.hpp"
#include "util/process.hpp"
#include "tui/select.hpp"
#include "vendor/json.hpp"
#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>
#include <cstdlib>

namespace dotrix {

// ============================================================
// Recipe loading — builtin from embedded JSON, custom from disk
// ============================================================

// Builtin tool recipes — written to ~/.config/dotrix/recipes.json on first run.
static const char* BUILTIN_RECIPES = R"JSON([{"name":"fzf","desc":"fuzzy finder","check":"fzf","install":"[ -d $HOME/.fzf ] || { git clone --depth=1 https://github.com/junegunn/fzf.git $HOME/.fzf && $HOME/.fzf/install --all --no-update-rc; }"},{"name":"trzsz","desc":"trz/tsz — file transfer","check":"trz","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/trzsz/trzsz-go/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && v=${ver#v} && curl -fsSL https://github.com/trzsz/trzsz-go/releases/download/$ver/trzsz_${v}_linux_x86_64.tar.gz -o /tmp/trzsz.tar.gz && tar -xzf /tmp/trzsz.tar.gz -C /tmp && cp /tmp/trzsz_${v}_linux_x86_64/trz /tmp/trzsz_${v}_linux_x86_64/tsz $HOME/.local/bin/"},{"name":"ohmyposh","desc":"prompt theme engine","check":"oh-my-posh","install":"mkdir -p $HOME/.local/bin && curl -fsSL https://github.com/JanDeDobbeleer/oh-my-posh/releases/latest/download/posh-linux-amd64 -o $HOME/.local/bin/oh-my-posh && chmod +x $HOME/.local/bin/oh-my-posh"},{"name":"ohmyzsh","desc":"Oh My Zsh framework","check":"test -d $HOME/.oh-my-zsh","install":"ZSH_CUSTOM=${ZSH_CUSTOM:-$HOME/.oh-my-zsh/custom} && (sh -c \"$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)\" \"\" --unattended || true) && [ -d $ZSH_CUSTOM/plugins/zsh-autosuggestions ] || git clone --depth=1 https://github.com/zsh-users/zsh-autosuggestions.git $ZSH_CUSTOM/plugins/zsh-autosuggestions && [ -d $ZSH_CUSTOM/plugins/zsh-syntax-highlighting ] || git clone --depth=1 https://github.com/zsh-users/zsh-syntax-highlighting.git $ZSH_CUSTOM/plugins/zsh-syntax-highlighting && [ -d $ZSH_CUSTOM/plugins/z ] || git clone --depth=1 https://github.com/agkozak/zsh-z.git $ZSH_CUSTOM/plugins/z"},{"name":"zoxide","desc":"smarter cd (z command)","check":"zoxide","install":"curl -fsSL https://raw.githubusercontent.com/ajeetdsouza/zoxide/master/install.sh | sh"},{"name":"starship","desc":"cross-shell prompt","check":"starship","install":"curl -fsSL https://starship.rs/install.sh | sh"},{"name":"nvim","desc":"neovim editor","check":"nvim","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/neovim/neovim/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && [ -n \"$ver\" ] && curl -fsSL https://github.com/neovim/neovim/releases/download/$ver/nvim-linux-x86_64.tar.gz -o /tmp/nvim.tar.gz && rm -rf $HOME/.local/nvim-linux-x86_64 && tar -xzf /tmp/nvim.tar.gz -C $HOME/.local && ln -sf $HOME/.local/nvim-linux-x86_64/bin/nvim $HOME/.local/bin/nvim"},{"name":"rg","desc":"ripgrep","check":"rg","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/BurntSushi/ripgrep/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && v=${ver#v} && curl -fsSL https://github.com/BurntSushi/ripgrep/releases/download/$ver/ripgrep-$v-x86_64-unknown-linux-musl.tar.gz -o /tmp/rg.tar.gz && tar -xzf /tmp/rg.tar.gz -C /tmp && cp /tmp/ripgrep-$v-x86_64-unknown-linux-musl/rg $HOME/.local/bin/rg"},{"name":"fd","desc":"fd — fast find","check":"fd","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/sharkdp/fd/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && curl -fsSL https://github.com/sharkdp/fd/releases/download/$ver/fd-$ver-x86_64-unknown-linux-musl.tar.gz -o /tmp/fd.tar.gz && tar -xzf /tmp/fd.tar.gz -C /tmp && cp /tmp/fd-$ver-x86_64-unknown-linux-musl/fd $HOME/.local/bin/fd"},{"name":"bat","desc":"bat — cat with highlighting","check":"bat","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/sharkdp/bat/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && curl -fsSL https://github.com/sharkdp/bat/releases/download/$ver/bat-$ver-x86_64-unknown-linux-musl.tar.gz -o /tmp/bat.tar.gz && tar -xzf /tmp/bat.tar.gz -C /tmp && cp /tmp/bat-$ver-x86_64-unknown-linux-musl/bat $HOME/.local/bin/bat"},{"name":"delta","desc":"delta — better git diff","check":"delta","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/dandavison/delta/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && v=${ver#v} && curl -fsSL https://github.com/dandavison/delta/releases/download/$ver/delta-$v-x86_64-unknown-linux-gnu.tar.gz -o /tmp/delta.tar.gz && tar -xzf /tmp/delta.tar.gz -C /tmp && cp /tmp/delta-$v-x86_64-unknown-linux-gnu/delta $HOME/.local/bin/delta"},{"name":"eza","desc":"eza — modern ls","check":"eza","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/eza-community/eza/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && curl -fsSL https://github.com/eza-community/eza/releases/download/$ver/eza_x86_64-unknown-linux-musl.tar.gz -o /tmp/eza.tar.gz && tar -xzf /tmp/eza.tar.gz -C /tmp && cp /tmp/eza $HOME/.local/bin/eza"},{"name":"lazygit","desc":"terminal git UI","check":"lazygit","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/jesseduffield/lazygit/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && v=${ver#v} && curl -fsSL https://github.com/jesseduffield/lazygit/releases/download/$ver/lazygit_${v}_Linux_x86_64.tar.gz -o /tmp/lazygit.tar.gz && tar -xzf /tmp/lazygit.tar.gz -C /tmp lazygit && cp /tmp/lazygit $HOME/.local/bin/lazygit"},{"name":"gcc","desc":"GNU C compiler","check":"gcc"},{"name":"g++","desc":"GNU C++ compiler","check":"g++"},{"name":"clang","desc":"LLVM C compiler","check":"clang"},{"name":"clangd","desc":"C/C++ LSP server","check":"clangd","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/clangd/clangd/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && curl -fsSL https://github.com/clangd/clangd/releases/download/$ver/clangd-linux-$ver.zip -o /tmp/clangd.zip && unzip -o /tmp/clangd.zip -d /tmp/clangd && cp /tmp/clangd/clangd_*/bin/clangd $HOME/.local/bin/clangd"},{"name":"cmake","desc":"CMake build system","check":"cmake","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/Kitware/CMake/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && v=${ver#v} && curl -fsSL https://github.com/Kitware/CMake/releases/download/$ver/cmake-$v-linux-x86_64.tar.gz -o /tmp/cmake.tar.gz && tar -xzf /tmp/cmake.tar.gz -C /tmp && cp /tmp/cmake-$v-linux-x86_64/bin/cmake $HOME/.local/bin/cmake"},{"name":"make","desc":"GNU Make","check":"make"},{"name":"gdb","desc":"GNU debugger","check":"gdb"},{"name":"go","desc":"go compiler","check":"go","install":"curl -fsSL https://go.dev/dl/go1.26.2.linux-amd64.tar.gz -o /tmp/go.tar.gz && rm -rf $HOME/.local/go && tar -C $HOME/.local -xzf /tmp/go.tar.gz"},{"name":"gopls","desc":"Go LSP server","check":"gopls","install":"GOBIN=$HOME/.local/bin go install golang.org/x/tools/gopls@latest"},{"name":"delve","desc":"Go debugger (dlv)","check":"dlv","install":"GOBIN=$HOME/.local/bin go install github.com/go-delve/delve/cmd/dlv@latest"},{"name":"python","desc":"Python 3","check":"python3"},{"name":"uv","desc":"uv — fast Python pkg mgr","check":"uv","install":"curl -fsSL https://astral.sh/uv/install.sh | sh"},{"name":"ruff","desc":"ruff — Python linter","check":"ruff","install":"curl -fsSL https://astral.sh/ruff/install.sh | sh"},{"name":"pyright","desc":"Python type checker","check":"pyright","install":"pip install --user pyright 2>/dev/null || pip3 install pyright 2>/dev/null || npm install -g pyright 2>/dev/null || true"},{"name":"rust","desc":"rust via rustup","check":"rustc","install":"curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y"},{"name":"rust-analyzer","desc":"Rust LSP server","check":"rust-analyzer","install":"curl -fsSL https://github.com/rust-lang/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu.gz -o /tmp/ra.gz && gunzip -f /tmp/ra.gz && mv /tmp/ra $HOME/.local/bin/rust-analyzer && chmod +x $HOME/.local/bin/rust-analyzer"},{"name":"node","desc":"node.js via nvm","check":"node","install":"[ -d $HOME/.nvm ] || { curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash; } && export NVM_DIR=\"$HOME/.nvm\" && . \"$NVM_DIR/nvm.sh\" && nvm install --lts || true"},{"name":"bun","desc":"bun runtime","check":"bun","install":"curl -fsSL https://bun.sh/install | bash"},{"name":"xclip","desc":"system clipboard (X11)","check":"xclip"},{"name":"tree-sitter","desc":"tree-sitter CLI","check":"tree-sitter","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/tree-sitter/tree-sitter/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && curl -fsSL https://github.com/tree-sitter/tree-sitter/releases/download/$ver/tree-sitter-linux-x64.gz -o /tmp/ts.gz && gunzip -f /tmp/ts.gz && mv /tmp/ts $HOME/.local/bin/tree-sitter && chmod +x $HOME/.local/bin/tree-sitter"},{"name":"mihomo","desc":"proxy (clash meta)","check":"mihomo","install":"ver=$(curl -sL -H \"Authorization: token $GITHUB_TOKEN\" https://api.github.com/repos/MetaCubeX/mihomo/releases/latest | grep tag_name | head -1 | cut -d'\"' -f4) && curl -fsSL https://github.com/MetaCubeX/mihomo/releases/download/$ver/mihomo-linux-amd64-$ver.gz -o /tmp/mihomo.gz && gunzip -f /tmp/mihomo.gz && mv /tmp/mihomo $HOME/.local/bin/mihomo && chmod +x $HOME/.local/bin/mihomo"},{"name":"htop","desc":"process viewer","check":"htop"},{"name":"netstat","desc":"network statistics","check":"netstat"},{"name":"ping","desc":"network reachability","check":"ping"},{"name":"curl","desc":"HTTP client","check":"curl"},{"name":"unzip","desc":"unzip archives","check":"unzip"},{"name":"docker","desc":"docker engine","check":"docker","install":"curl -fsSL https://get.docker.com | sh","needs_sudo":true},{"name":"tmux","desc":"terminal multiplexer","check":"tmux"},{"name":"zsh","desc":"z shell","check":"zsh"},{"name":"git","desc":"git","check":"git"}])JSON";

static std::vector<SetupCommand::Recipe> parse_recipes_json(const fs::path& path) {
    std::vector<SetupCommand::Recipe> out;
    if (!fs::exists(path)) return out;

    std::ifstream in(path);
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_array()) return out;

    for (auto& item : j) {
        SetupCommand::Recipe r;
        r.name       = item.value("name", "");
        r.desc       = item.value("desc", "");
        r.check      = item.value("check", r.name);
        r.install    = item.value("install", "");
        r.needs_sudo = item.value("needs_sudo", false);
        if (!r.name.empty()) out.push_back(std::move(r));
    }
    return out;
}

std::vector<SetupCommand::Recipe> SetupCommand::load_recipes() {
    auto path = recipes_path();

    // First run: seed with builtin recipes
    if (!fs::exists(path)) {
        fs::create_directories(path.parent_path());

        // Migrate from old location (~/.config/dotrix/recipes.json) if exists
        auto* h = std::getenv("HOME");
        auto old_path = h ? fs::path(h) / ".config/dotrix/recipes.json" : fs::path();
        if (!old_path.empty() && fs::exists(old_path)) {
            fs::create_directories(path.parent_path());
            fs::copy_file(old_path, path);
            fs::remove(old_path);
        } else {
            std::ofstream out(path);
            out << BUILTIN_RECIPES << "\n";
        }
    }

    return parse_recipes_json(path);
}

fs::path SetupCommand::recipes_path() {
    auto cfg = Config::load();
    return cfg.repo / ".dotrix" / "recipes.json";
}

// ============================================================
// List / Install / TUI / Add / Remove
// ============================================================

int SetupCommand::run_list(const std::vector<Recipe>& all) {
    int installed = 0, missing = 0;

    std::cout << "\033[0;34mAvailable tools:\033[0m\n\n";
    for (auto& t : all) {
        auto check_cmd = std::string(t.check);
        bool has = false;
        if (check_cmd.find(' ') != std::string::npos) {
            // Shell expression (e.g. "test -d $HOME/.oh-my-zsh")
            has = util::run({"sh", "-c", "\"" + check_cmd + "\" >/dev/null 2>&1"});
        } else {
            // Simple binary name
            has = !util::capture({"which", check_cmd}).empty();
        }
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

    // Load config for GITHUB_TOKEN
    auto dotrix_cfg = Config::load();
    if (!dotrix_cfg.github_token.empty()) {
        setenv("GITHUB_TOKEN", dotrix_cfg.github_token.c_str(), 1);
    }

    int installed_now = 0, skipped_sudo = 0;

    for (auto& t : all) {
        if (!f.empty() && !f.count(t.name)) continue;

        // Re-check installation (same dual-mode logic as list/tui)
        auto check_cmd = std::string(t.check);
        bool already = false;
        if (check_cmd.find(' ') != std::string::npos)
            already = util::run({"sh", "-c", "\"" + check_cmd + "\" >/dev/null 2>&1"});
        else
            already = !util::capture({"which", check_cmd}).empty();
        if (already) continue;

        if (t.install.empty()) {
            Reporter::warn("skip: " + t.name + " — system tool, no install script");
            continue;
        }

        if (t.needs_sudo) {
            Reporter::warn("skip: " + t.name + " — needs sudo. Run manually:");
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
        auto check_cmd = std::string(t.check);
        bool has = false;
        if (check_cmd.find(' ') != std::string::npos) {
            // Shell expression (e.g. "test -d $HOME/.oh-my-zsh")
            has = util::run({"sh", "-c", "\"" + check_cmd + "\" >/dev/null 2>&1"});
        } else {
            // Simple binary name
            has = !util::capture({"which", check_cmd}).empty();
        }
        items.push_back({t.name, t.desc, /*checked=*/false, has, t.needs_sudo && !t.install.empty()});
    }

    auto selected = tui::select(items, "dotrix setup");

    if (selected.empty()) {
        Reporter::info("nothing selected");
        return 0;
    }

    std::vector<std::string> filter;
    for (int i : selected) filter.push_back(all[i].name);
    return run_install(all, filter);
}

int SetupCommand::run_add() {
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

    // Load existing custom recipes
    nlohmann::json j = nlohmann::json::array();
    auto path = recipes_path();
    if (fs::exists(path)) {
        std::ifstream in(path);
        j = nlohmann::json::parse(in, nullptr, false);
        if (!j.is_array()) j = nlohmann::json::array();
    }

    j.push_back({
        {"name", r.name},
        {"desc", r.desc},
        {"check", r.check},
        {"install", r.install},
        {"needs_sudo", r.needs_sudo}
    });

    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    out << j.dump(2) << "\n";

    Reporter::ok("added: " + r.name);
    Reporter::info("custom recipes: " + path.string());
    auto* home = std::getenv("HOME");
    if (home) Reporter::info("recipes stored in dotfiles repo — synced automatically");
    return 0;
}

int SetupCommand::run_rm(const std::vector<std::string>& names) {
    auto path = recipes_path();
    if (!fs::exists(path) || names.empty()) {
        Reporter::warn("usage: dotrix setup --rm <name>");
        return 1;
    }

    std::ifstream in(path);
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_array()) return 1;

    nlohmann::json out_json = nlohmann::json::array();
    std::set<std::string> targets(names.begin(), names.end());
    int removed = 0;

    for (auto& item : j) {
        if (targets.count(item.value("name", ""))) {
            Reporter::ok("removed: " + item.value("name", ""));
            ++removed;
        } else {
            out_json.push_back(item);
        }
    }

    if (removed == 0) {
        Reporter::warn("no matching custom recipes");
        return 1;
    }

    std::ofstream ofs(path);
    ofs << out_json.dump(2) << "\n";
    Reporter::ok("removed " + std::to_string(removed) + " custom recipe(s)");
    return 0;
}

// ============================================================
// Execute
// ============================================================
int SetupCommand::execute(const std::vector<std::string>& args) {
    auto all = load_recipes();

    bool run = false;
    std::string mode;
    std::vector<std::string> filter;

    for (auto& a : args) {
        if (a == "--run")  run = true;
        else if (a == "--pick") mode = "pick";
        else if (a == "--add")  mode = "add";
        else if (a == "--rm")   mode = "rm";
        else filter.push_back(a);
    }

    if (mode == "add") return run_add();
    if (mode == "rm")  return run_rm(filter);
    if (mode == "pick") return run_tui(all);
    if (run) return run_install(all, filter);
    return run_list(all);
}

} // namespace dotrix
