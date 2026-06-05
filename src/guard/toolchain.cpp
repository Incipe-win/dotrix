#include "toolchain.hpp"

namespace dotrix {

const std::vector<Tool>& known_tools() {
    static const std::vector<Tool> tools = {
        // ---- Shell (system tools, no install) ----
        {".zshrc",        "zsh",          ""},
        {".zshenv",       "zsh",          ""},
        {".zprofile",     "zsh",          ""},
        {".bashrc",       "bash",         ""},
        {".bash_profile", "bash",         ""},
        {".profile",      "bash",         ""},

        // ---- Git (system) ----
        {".gitconfig",    "git",          ""},
        {".config/git",   "git",          ""},

        // ---- Terminal ----
        {".tmux.conf",    "tmux",         ""},

        // ---- Editors ----
        {".config/nvim",  "nvim",
         "curl -fsSL https://github.com/neovim/neovim/releases/latest/download/nvim-linux64.tar.gz "
         "-o /tmp/nvim.tar.gz && tar -xzf /tmp/nvim.tar.gz -C $HOME/.local && "
         "ln -sf $HOME/.local/nvim-linux64/bin/nvim $HOME/.local/bin/nvim"},

        {".vimrc",        "vim",          ""},

        {".config/zed",   "zed",
         "curl -fsSL https://zed.dev/api/releases/stable/latest/zed-linux-x86_64.tar.gz "
         "-o /tmp/zed.tar.gz && tar -xzf /tmp/zed.tar.gz -C $HOME/.local && "
         "ln -sf $HOME/.local/zed/bin/zed $HOME/.local/bin/zed"},

        {".vscode-server","code",         ""},  // installed via vscode remote
        {".vscode",       "code",         ""},

        // ---- Shell tools ----
        {".fzf.zsh",      "fzf",
         "[ -d $HOME/.fzf ] || git clone --depth=1 https://github.com/junegunn/fzf.git $HOME/.fzf && "
         "$HOME/.fzf/install --all --no-update-rc"},

        {".fzf.bash",     "fzf",          ""},  // same binary as above

        {".config/ohmyposh","oh-my-posh",
         "curl -fsSL https://github.com/JanDeDobbeleer/oh-my-posh/releases/latest/download/"
         "posh-linux-amd64 -o $HOME/.local/bin/oh-my-posh && chmod +x $HOME/.local/bin/oh-my-posh"},

        // ---- Proxy ----
        {"clash/config",  "mihomo",
         "ver=$(curl -s https://api.github.com/repos/MetaCubeX/mihomo/releases/latest | "
         "grep tag_name | cut -d'\"' -f4) && "
         "curl -fsSL https://github.com/MetaCubeX/mihomo/releases/download/$ver/"
         "mihomo-linux-amd64-$ver.gz -o /tmp/mihomo.gz && "
         "gunzip -f /tmp/mihomo.gz && mv /tmp/mihomo $HOME/.local/bin/mihomo && chmod +x $HOME/.local/bin/mihomo"},

        {".config/mihomo","mihomo",       ""},  // same as above

        // ---- Languages / runtimes ----
        {".config/go",    "go",
         "curl -fsSL https://go.dev/dl/go1.26.2.linux-amd64.tar.gz -o /tmp/go.tar.gz && "
         "rm -rf $HOME/.local/go && tar -C $HOME/.local -xzf /tmp/go.tar.gz"},

        {".cargo",        "cargo",
         "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y"},

        {".rustup",       "rustc",        ""},  // same as cargo

        {".nvm",          "node",
         "curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash && "
         "export NVM_DIR=$HOME/.nvm && [ -s $NVM_DIR/nvm.sh ] && . $NVM_DIR/nvm.sh && nvm install --lts"},

        {".bun",          "bun",
         "curl -fsSL https://bun.sh/install | bash"},

        {"bun.lockb",     "bun",          ""},

        // ---- Containers ----
        {".docker",       "docker",
         "curl -fsSL https://get.docker.com | sh"},  // needs sudo actually, skip

        // ---- Other ----
        {".ssh/config",   "ssh",          ""},
    };

    return tools;
}

} // namespace dotrix
