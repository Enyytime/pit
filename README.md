# pit
> Reinventing git, cuz why not

A Git-inspired version control system written from scratch in C. `pit` implements core VCS functionality including object storage, staging, commits, branching, merging, and syncing with a remote server, without using any Git libraries.

---

## Table of Contents
- [Install](#install)
- [Getting started](#getting-started)
- [How it works](#how-it-works)
- [Repository structure](#repository-structure)
- [Object model](#object-model)
- [Commands](#commands)
- [Working with a team](#working-with-a-team)
- [Running your own server](#running-your-own-server)
- [Building](#building)
- [Limitations](#limitations)
- [Roadmap](#roadmap)

---

## Install

Requires a C compiler, `make`, OpenSSL and zlib headers, `curl` and `rsync`.

**Ubuntu/Debian**
```bash
sudo apt install -y build-essential libssl-dev zlib1g-dev curl rsync git
```

**Arch**
```bash
sudo pacman -S base-devel openssl zlib curl rsync git
```

Then:
```bash
git clone https://github.com/YOU/pit.git
cd pit
make
sudo make install
```

Windows users: use WSL and follow the Ubuntu steps inside it. pit uses POSIX APIs (`fork`, `dirent`) and does not build natively on Windows.

---

## Getting started

Start a new project:
```bash
mkdir myproject && cd myproject
pit init
printf "name=Your Name\nemail=you@example.com\n" > .pit/config   # commit author
echo "hello" > a.txt
pit add .
pit commit -m "first commit"
pit log
```

`pit init` fills `.pit/config` from the `GIT_AUTHOR_NAME` and `GIT_AUTHOR_EMAIL` environment variables. If they are unset it writes `(null)`, so set the config by hand as above (or export those variables in your shell profile).

Join an existing shared repo:
```bash
mkdir myproject && cd myproject
pit init
printf "name=Your Name\nemail=you@example.com\n" > .pit/config
pit remote add origin http://USER:TOKEN@HOST:8080/myproject
pit pull
```

---

## How it works

`pit` stores all data as **objects** in `.pit/objects/`. Every file, directory snapshot, and commit is hashed with SHA-1 and stored as a zlib-compressed object. This means:

- Identical files are stored only once (content-addressed)
- History is immutable, so you can always retrieve any past state
- Branches are just lightweight pointers to commit hashes

The workflow mirrors Git: `add` stages files, `commit` snapshots the index, `log` walks the commit chain, `checkout` restores a snapshot, and `push` / `pull` sync objects with a remote.

---

## Repository structure

Running `pit init` creates the following inside your project:

```
.pit/
├── HEAD              # points to current branch e.g. "ref: refs/heads/main"
├── config            # author name/email and remotes
├── index             # staging area: list of mode/hash/filename entries
├── objects/          # all stored objects (blobs, trees, commits)
│   ├── ab/
│   │   └── cd1234...   # object file, named by first 2 chars of hash
│   └── ...
└── refs/
    ├── heads/
    │   ├── main      # commit hash of latest commit on main
    │   └── feature   # commit hash of latest commit on feature
    ├── remotes/
    │   └── origin/
    │       └── main  # last known commit of origin's main (set by fetch)
    └── tags/
```

`.pit/config` looks like:
```
name=Your Name
email=you@example.com
remote.origin=http://USER:TOKEN@HOST:8080/myproject
```

---

## Object model

Every object is stored as:
```
<type> <size>\0<content>
```
Compressed with zlib, named by its SHA-1 hash, stored at `.pit/objects/<first 2 chars>/<remaining 38 chars>`.

There are three object types.

### blob
Raw file content. Created when you `pit add` a file.

### tree
A directory snapshot. Stores a list of entries:
```
<mode> <filename>\0<20-byte binary SHA-1>
```
- `100644` = regular file
- `40000`  = subdirectory (points to another tree)

### commit
A snapshot of the entire project at a point in time:
```
tree <tree-hash>
parent <parent-commit-hash>
author Name <email> <timestamp> +0000
committer Name <email> <timestamp> +0000

<commit message>
```
The first commit has no `parent` line.

---

## Commands

### `pit init`
Initializes a new pit repository in the current directory.

Creates the `.pit/` directory structure and writes an initial `HEAD` pointing to `refs/heads/main`. Reads `GIT_AUTHOR_NAME` and `GIT_AUTHOR_EMAIL` to populate `.pit/config`.

```bash
pit init
```

---

### `pit add <file|.>`
Stages a file (or all files) for the next commit.

Hashes the file content into a blob object and writes an entry to `.pit/index`. If the file is already staged with the same hash, it is skipped. Passing `.` recursively stages everything in the current directory, skipping `.pit` and `.git`.

```bash
pit add main.c
pit add .
```

**Index format:**
```
100644 <sha1-hash> <filename>
```

---

### `pit commit -m "<message>"`
Creates a commit from the current index.

1. Calls `write-tree` internally to build a tree object from the index
2. Reads the current branch from `HEAD` to find the parent commit hash
3. Writes a commit object with tree, parent, author, and message
4. Updates the current branch ref to point to the new commit hash

```bash
pit commit -m "initial commit"
```

---

### `pit write-tree`
Builds a tree object from the current index and prints its hash.

Handles nested directories by recursively creating subtree objects bottom-up (deepest directories first), then assembling the root tree. Not usually called directly; `commit` calls this internally.

```bash
pit write-tree
```

---

### `pit commit-tree <tree-hash> "<message>"`
Low-level command. Creates a commit object from a given tree hash directly.

```bash
pit commit-tree abc123... "my message"
```

---

### `pit log`
Walks the commit history of the current branch and prints each commit.

Reads `HEAD` to find the current branch, reads that branch's ref to get the latest commit hash, then follows the `parent` chain until it reaches the first commit.

```bash
pit log
```

**Output format:**
```
Commit <hash>
Author: Name <email>
Date:   Mon Jan 01 12:00:00 2025

    commit message
```

---

### `pit status`
Shows staged changes and unstaged modifications.

- **Changes to be committed**: compares index entries against the last commit's tree. Files not in the tree are "New file", files with a different hash are "Modified".
- **Changes not staged for commit**: compares index entries against the current file on disk by rehashing.

```bash
pit status
```

---

### `pit checkout <branch>`
Switches to a different branch.

1. Checks that `.pit/refs/heads/<branch>` exists
2. Reads the commit hash from that ref
3. Recursively restores all files and directories from that commit's tree to the working directory
4. Updates `.pit/HEAD` to point to the new branch

```bash
pit checkout feature
```

---

### `pit branch create <name>`
Creates a new branch pointing to the current commit.

Reads the current branch's commit hash and writes it to `.pit/refs/heads/<name>`. No files are changed; it is just a new pointer.

```bash
pit branch create feature
```

### `pit branch list`
Lists all branches.

```bash
pit branch list
```

### `pit branch delete <name>`
Deletes a branch. Refuses to delete the currently checked-out branch. Only the pointer is removed; the commits stay in `.pit/objects/`.

```bash
pit branch delete feature
```

---

### `pit merge <branch>`
Fast-forward merges `<branch>` into the current branch.

1. Reads the commit hash of the current branch and of the target (a local branch, or a remote-tracking ref like `origin/main`)
2. Walks the target's parent chain looking for the current commit
3. If found, moves the current branch pointer to the target commit and restores the working tree
4. If not found, the histories have diverged and the merge is refused

```bash
pit merge feature
```

---

### `pit remote add <name> <url>` / `pit remote list`
Saves a remote to `.pit/config` as `remote.<name>=<url>`.

```bash
pit remote add origin http://USER:TOKEN@HOST:8080/myproject
pit remote add backup user@host:/home/user/repos/myproject
pit remote list
```

---

### `pit push [remote]`
Uploads objects the remote doesn't have, then moves the remote's branch pointer to your commit. Defaults to `origin`.

Push is rejected if the remote branch has commits you don't have (it must be a fast-forward). Run `pit pull` first in that case.

```bash
pit push
```

---

### `pit fetch [remote]`
Downloads missing objects and records the remote branch tip in `.pit/refs/remotes/<remote>/<branch>`. Does not touch your working files.

```bash
pit fetch
```

---

### `pit pull [remote]`
`fetch` plus a fast-forward merge of `<remote>/<branch>` into the current branch. In an empty repo with no local commits, it creates the branch from the fetched commit and checks out the files.

```bash
pit pull
```

---

### `pit hash-object <file>`
Low-level command. Hashes a file and stores it as a blob object, printing its hash.

```bash
pit hash-object main.c
```

---

### `pit cat-file <hash>`
Low-level command. Reads and prints the decompressed content of any stored object by its hash.

```bash
pit cat-file abc123...
```

---

## Working with a team

pit has two transports, chosen by the remote URL:

| URL | Transport | Needs |
|---|---|---|
| `user@host:/path` | SSH | `ssh`, `rsync`, `scp`; a login on the server |
| `http://user:token@host:8080/repo` | HTTP | `curl`; a token |

Objects are content-addressed, so push and fetch only transfer files the other side doesn't already have. A branch is a file holding a commit hash, so pushing a branch means uploading missing objects and then writing one small file.

A typical loop for a team sharing `main`:
```bash
pit pull                       # get the latest
# ...edit files...
pit add .
pit commit -m "what I changed"
pit push                       # rejected? someone pushed first, so pit pull and retry
```

The HTTP transport needs no accounts on the server: give each teammate a token and they can use pit straight away.

---

## Running your own server

`pit-server.py` is a small Python HTTP server (standard library only). It stores objects and refs under `~/pit-data/<repo>/` and creates a repo the first time someone pushes to it.

**1. Copy the server and create tokens** (on the server, in your home directory):
```bash
mkdir -p ~/pit-data
for n in alice bob; do echo "$n:$(openssl rand -hex 16)"; done > ~/pit-tokens.txt
cat ~/pit-tokens.txt      # one line per user: name:token
```
Give each user their own token. Delete a line (and restart the server) to revoke them.

**2. Run it as a service** so it survives reboots:
```bash
sudo tee /etc/systemd/system/pit-server.service <<'EOF'
[Unit]
Description=pit server
After=network.target

[Service]
User=YOUR_USER
ExecStart=/usr/bin/python3 /home/YOUR_USER/pit-server.py
Restart=always

[Install]
WantedBy=multi-user.target
EOF
sudo systemctl daemon-reload
sudo systemctl enable --now pit-server
```

**3. Open port 8080** in your firewall or cloud provider's network rules.

**4. Test it:**
```bash
curl -i -u x:TOKEN http://HOST:8080/myrepo/objects
```
`200 OK` with an empty body means the server works and the token is accepted.

**Security notes**
- Plain HTTP sends tokens unencrypted. Put a reverse proxy such as Caddy in front of the server and use `https://` URLs before exposing it beyond a trusted network.
- Any valid token can read and write every repo on the server.
- A token in a remote URL is stored in plain text in `.pit/config`. Don't share your project folder with `.pit/` inside it.

---

## Building

```bash
make              # build ./pit
make debug        # build with AddressSanitizer and UBSan
sudo make install # copy to /usr/local/bin
make clean
```

The Makefile picks up every `.c` file in the root, `include/`, `pit_commands/` and `data_structures/`, so new commands only need to be dropped into `pit_commands/`.

---

## Limitations

- Fast-forward merges only; there is no 3-way merge or conflict resolution
- `push` and `fetch` handle the current branch only
- No `diff` command yet
- Any valid server token can read and write every repo on that server
- No locking: two pushes at the same moment can race
- No `pit clone`; use `init`, `remote add`, `pit pull` instead
- Linux and macOS only (WSL on Windows)

---

## Roadmap

- [x] init, add, commit, log, status, checkout
- [x] branch (create, list, delete)
- [x] merge (fast-forward)
- [x] remote, fetch, push, pull (SSH and HTTP)
- [x] HTTP server
- [ ] diff
- [ ] 3-way merge
- [ ] push/fetch all branches
- [ ] `pit clone`
- [ ] web viewer
- [ ] HTTPS
