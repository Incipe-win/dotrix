# dotrix

> Single-binary dotfiles manager. Modern C++20, automatic secret redaction,
> smart sync with live-secret preservation.

## Quick start

```bash
git clone https://github.com/Incipe-win/dotrix.git && cd dotrix
xmake                              # one-shot build

# Start tracking
./dotrix ~/.zshrc                  # single file
./dotrix ~/.config/nvim/           # entire directory
```

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
├── tui/         select          Terminal checkbox UI
├── commands/    add, remove, capture, sync, list, status, scan,
│               check, setup, config
└── vendor/      json.hpp        nlohmann/json (header-only)
```

## Build

```bash
xmake                          # release build  (binary: ./dotrix, ~300KB)
xmake f -m debug && xmake      # debug build
```

Requires: C++20 compiler, xmake.

## CI / Release

Push a tag to trigger multi-arch build + GitHub Release:

```bash
git tag v0.1.0 && git push origin v0.1.0
# → linux/amd64 + linux/arm64 tarballs published
```
