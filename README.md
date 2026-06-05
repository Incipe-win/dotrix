# dotrix

> Single-binary dotfiles manager.  Modern C++20, command-pattern architecture.

## Quick start

```bash
git clone https://github.com/huachaowu/dotrix.git ~/dotrix
cd ~/dotrix
xmake                              # build

# Start managing configs
./dotrix ~/.zshrc                  # add a file
./dotrix ~/.config/nvim/           # add a directory

# New machine: deploy everything
git clone <your-dotfiles-repo> ~/.dotfiles
./dotrix sync                      # deploys with auto-backup
```

## Commands

```
dotrix add     <file...>    Start tracking config files
dotrix remove  <file...>    Stop tracking (alias: rm)
dotrix capture              Save live changes → repo + git commit
dotrix sync                 Deploy repo → live (backs up existing files)
dotrix list                 Show managed files
dotrix status               Show files with uncommitted changes
```

## Workflow

```
dotrix add ~/.zshrc          # Start managing
vim ~/.zshrc                 # Edit as usual
dotrix status                # See what changed
dotrix capture               # Save changes → repo + commit

# On a new machine:
git clone <repo> ~/.dotfiles
dotrix sync                  # Deploy all configs (backs up existing files)
```

## How it works

```
dotrix add ~/.zshrc
  → copies to ~/.dotfiles/.zshrc
  → appends to .dotrix/manifest
  → git commit "dotrix: add /home/user/.zshrc"

dotrix sync
  → reads .dotrix/manifest
  → for each file: if live file exists & differs → backup to .dotrix.bak
  → copies repo → live

dotrix capture
  → compares live vs repo
  → copies changed files live → repo
  → git commit
```

## Build

```bash
xmake          # debug:   xmake f -m debug && xmake
```

Requires: C++20 compiler, xmake.

## Architecture

```
src/
├── main.cpp                 # Entry + Dispatcher (command registry)
├── core/    config, types   # Configuration & common types
├── ui/      reporter        # Output formatting
├── util/    fs, process     # Filesystem & subprocess helpers
├── repo/    store, manifest, git   # Data layer
├── sync/    strategy        # ISyncStrategy → CopySyncStrategy
└── commands/ add, remove, capture, sync, list, status
```

Add a command: implement `ICommand`, register in `Dispatcher` — one line.
