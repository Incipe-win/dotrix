# dotfiles

> Managed by `dotrix` — one binary, no manifest editing, no dependencies.

## Quick start

```bash
git clone https://github.com/huachaowu/dotfiles.git ~/dotfiles
cd ~/dotfiles

# Build
xmake

# Add config files
./dotrix ~/.zshrc
./dotrix ~/.tmux.conf
./dotrix ~/.config/nvim/

# On a new machine: install symlinks
./dotrix
```

## Commands

```bash
dotrix <file...>       # Add files/dirs to management
dotrix                 # Install symlinks (backs up existing files)
dotrix install         # Same as above
dotrix install --dry   # Preview, don't touch anything
dotrix sync            # Copy changes from live files → repo + commit
dotrix list            # Show managed files
dotrix status          # Show files with uncommitted changes
dotrix help
```

## How it works

```
dotrix ~/.zshrc
  → Copies ~/.zshrc → ~/dotfiles/.zshrc         (mirrors $HOME structure)
  → Adds ".zshrc" to .dotrix/manifest
  → git commit "dotrix: add .zshrc"

dotrix                    (new machine)
  → Reads .dotrix/manifest
  → For each file: creates symlink ~/.zshrc → ~/dotfiles/.zshrc
  → If ~/.zshrc already exists → backs up to ~/.zshrc.dotrix.bak first
```

## Structure

```
~/dotfiles/                  # git repo
├── .dotrix/
│   └── manifest             # one path per line (home-relative)
├── .zshrc                   # managed file (mirrors ~/.zshrc)
├── .tmux.conf               # managed file (mirrors ~/.tmux.conf)
├── .config/
│   └── nvim/                # managed dir (mirrors ~/.config/nvim/)
├── dotrix                   # the binary
├── src/                     # C++ source
├── xmake.lua
└── .gitignore
```
