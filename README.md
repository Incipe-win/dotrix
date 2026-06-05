# dotrix

> Single-binary dotfiles manager.  Modern C++20.  Config-only, no bloat.

## Quick start

```bash
git clone https://github.com/huachaowu/dotrix.git && cd dotrix
xmake                              # one-shot build

# Start tracking
./dotrix ~/.zshrc                  # single file
./dotrix ~/.config/nvim/           # entire directory (recursive)
./dotrix ~/.zshrc ~/.tmux.conf    # multiple at once
```

## Commands

```
dotrix add     <file...>    Start tracking (auto-redacts secrets)
dotrix remove  <file...>    Stop tracking (alias: rm)
dotrix capture              Save live changes → repo + commit
dotrix sync                 Deploy repo → live (backs up existing files)
dotrix scan                 Scan managed files for secrets
dotrix list                 Show managed files
dotrix status               Show files with uncommitted changes
dotrix help
```

No args = `dotrix sync`.

## Workflow

```
# ---- Set up ----
dotrix ~/.zshrc ~/.gitconfig ~/.tmux.conf ~/.config/nvim/

# ---- Daily use ----
vim ~/.zshrc                         # edit configs normally
dotrix status                        # see what changed
dotrix capture                       # save → repo, git commit

# ---- Stop tracking ----
dotrix rm ~/.config/nvim

# ---- New machine ----
git clone <repo> ~/.dotfiles
dotrix sync                          # deploys all configs, backs up existing files
```

## Secret handling

`dotrix add` automatically detects API keys / tokens and redacts them before
storing:

```bash
dotrix ~/.claude/settings.json
  [warn] secrets in: ~/.claude/settings.json
  [ ok ] redacted 1 secret(s), added  # repo: __DOTRIX_REDACTED__
  [warn] 1 value(s) redacted — live files untouched
```

`dotrix sync` skips redacted files (protects your real keys):

```bash
dotrix sync
  [warn] redacted, skipping: ~/.claude/settings.json (use --force to overwrite)
```

Detected patterns: `sk-*`, `ghp_*`, `xoxb-*`, `AKIA*`, JWT headers,
`token =`, `api_key =`, `secret =`, `password =` with non-trivial values.

## Paths are portable

Manifest stores paths relative to `$HOME` (`.zshrc`, not `/home/user/.zshrc`).
Old absolute paths are auto-migrated on read.

```
Machine A (huachaowu):   .zshrc  →  /home/huachaowu/.zshrc
Machine B (bob):         .zshrc  →  /home/bob/.zshrc
```

## Build

```bash
xmake                          # release build  (binary: ./dotrix)
xmake f -m debug && xmake      # debug build
```

Requires: C++20 compiler, xmake.  Binary ~180KB stripped.

## Architecture

```
src/
├── main.cpp                     # Entry + Dispatcher (command registry)
├── core/     config, types      # Config resolver, common types
├── ui/       reporter           # Output formatting
├── util/     fs, process        # Filesystem & subprocess
├── repo/     store, manifest, git   # Data layer
├── guard/    secrets            # API-key detection & redaction
├── sync/     strategy           # ISyncStrategy → CopySyncStrategy
└── commands/ add, remove, capture, sync, list, status, scan
```

Add a command: implement `ICommand`, register in `Dispatcher` — one line.

## CI / Release

Push a tag to trigger multi-arch build + GitHub Release:

```bash
git tag v0.1.0 && git push origin v0.1.0
# → linux/amd64 + linux/arm64 tarballs published
```
