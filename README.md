# dotrix

> Single-binary dotfiles manager. Modern C++20, automatic secret redaction,
> smart sync with live-secret preservation.

Two repos:

| Repo | Purpose |
|------|---------|
| [Incipe-win/dotrix](https://github.com/Incipe-win/dotrix) | The tool itself (this repo) |
| [Incipe-win/.dotfiles](https://github.com/Incipe-win/.dotfiles) | My personal config files managed by dotrix |

## Quick start

```bash
git clone https://github.com/Incipe-win/dotrix.git && cd dotrix
xmake                              # one-shot build

# Start tracking your own configs
./dotrix ~/.zshrc                  # single file
./dotrix ~/.config/nvim/           # entire directory
```

## New machine setup

Bare-metal server — no zsh, no nothing.

```bash
# 1. System prerequisites
sudo apt install -y zsh git curl unzip build-essential

# 2. Build dotrix
git clone https://github.com/Incipe-win/dotrix.git ~/dotrix
cd ~/dotrix && xmake

# 3. Clone your config repo
git clone https://github.com/Incipe-win/.dotfiles.git ~/.dotfiles

# 4. Set GitHub token (for downloading tool binaries)
./dotrix config github_token ghp_xxx

# 5. Install dotrix itself requires installing zsh + helpers
./dotrix setup --pick    # check: zsh, ohmyzsh, ohmyposh, fzf

# 6. Deploy all config files (auto-merge secrets)
./dotrix sync

# 7. Install dev tools
./dotrix setup --pick    # check: nvim, rg, fd, go, rust, node, etc.

# 8. Switch to zsh
chsh -s $(which zsh)
exec zsh
```

> **Step 5 & 7 are the same command.**  First pass installs shell basics so
> `.zshrc` can source them.  Second pass installs everything else.

## Commands

```
dotrix <file...>            add — start tracking (auto-redacts secrets)
dotrix add     <file...>    add — explicit form
dotrix remove  <file...>    stop tracking (alias: rm)
dotrix capture              save live changes → repo (auto-redacts)
dotrix sync                 deploy repo → live (smart-merge secrets)
dotrix list                 show managed files
dotrix status               show files with uncommitted changes
dotrix scan                 scan managed files for secrets
dotrix check                show which tools are installed / missing
dotrix setup                dev-tool installer with TUI (--pick)
dotrix setup --add          add custom install recipe
dotrix config               manage dotrix settings (token, paths)
dotrix help
```

No args = `dotrix sync`.

## Secret handling

API keys and tokens are detected and automatically replaced with
`__DOTRIX_REDACTED__` when adding or capturing files.

On sync, secrets are **never** overwritten — the repo's new config is merged
with the live file's real keys:

```
repo (new):    OCO_API_KEY= __DOTRIX_REDACTED__    live (old):    OCO_API_KEY= sk-real-key
               OCO_NEW_OPTION=true                                 OCO_MODEL=deepseek
                                                  ↓ merge ↓
               OCO_API_KEY= sk-real-key      ← preserved from live
               OCO_NEW_OPTION=true           ← new from repo
```

## Paths are portable

Manifest stores paths relative to `$HOME` (`.zshrc`, not `/home/user/.zshrc`).
Old absolute paths are auto-migrated.

## dev-tool installer

```bash
dotrix setup                              # list all 42 tools
dotrix setup --pick                       # interactive TUI
dotrix setup --run nvim go rust           # install specific
dotrix setup --add                        # add custom recipe
dotrix setup --rm mytool                  # remove custom recipe
```

All installs are user-local (`~/.local/bin`). GitHub API calls use the
token from `dotrix config`.

## Configuration

```
~/.config/dotrix/config.json      dotrix settings (token, repo path)
~/.dotfiles/.dotrix/recipes.json  tool install recipes
~/.dotfiles/.dotrix/manifest.json managed file list
```

All three are auto-tracked. `config.json` secrets are redacted in the repo.

```bash
dotrix config                         # show settings
dotrix config github_token ghp_xxx    # set GitHub token
dotrix config dotfiles_root ~/.dotfiles
```

## Architecture

```
src/
├── main.cpp                     Entry + Dispatcher + auto-track-self
├── core/        config          Config loader (JSON)
├── ui/          reporter        Output formatting
├── util/        fs, process     Filesystem & subprocess
├── repo/        store, manifest, git   Data layer (JSON manifest)
├── guard/       secrets, toolchain     Secret detection/redaction/merge
├── sync/        strategy        ISyncStrategy → CopySyncStrategy
├── commands/    add, remove, capture, sync, list, status, scan,
│               check, setup, config
└── vendor/      json.hpp        nlohmann/json (header-only)

External:  fuibase/               Modern C++20 immediate-mode TUI framework
```

## Dependencies

- [**fuibase**](https://github.com/Incipe-win/fuibase) — C++20 TUI framework (header-only)

```bash
git clone https://github.com/Incipe-win/fuibase.git ../fuibase
```

## Build

**Option 1: Using xmake** (recommended)
```bash
xmake                          # release build  (binary: ./dotrix, ~300KB)
xmake f -m debug && xmake      # debug build
```

**Option 2: Using g++ directly** (no xmake required)
```bash
g++ -std=c++20 -O2 -o dotrix $(find src -name "*.cpp") \
    -Isrc -Isrc/vendor -I../fuibase/include -lstdc++fs -s
```

Requires: C++20 compiler (g++ 10+), [fuibase](https://github.com/Incipe-win/fuibase), optional: xmake.

## CI / Release

Push a tag to trigger multi-arch build + GitHub Release:

```bash
git tag v0.1.0 && git push origin v0.1.0
# → linux/amd64 + linux/arm64 tarballs published
```
