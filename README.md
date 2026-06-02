# pit
> Reinventing git, cuz why not

A Git-inspired version control system written from scratch in C. `pit` implements core VCS functionality including object storage, staging, commits, branching, and working tree management — without using any Git libraries.

---

## Table of Contents
- [How it works](#how-it-works)
- [Repository structure](#repository-structure)
- [Object model](#object-model)
- [Commands](#commands)
- [Building](#building)

---

## How it works

`pit` stores all data as **objects** in `.pit/objects/`. Every file, directory snapshot, and commit is hashed with SHA-1 and stored as a zlib-compressed object. This means:

- Identical files are stored only once (content-addressed)
- History is immutable — you can always retrieve any past state
- Branches are just lightweight pointers to commit hashes

The workflow mirrors Git: `add` stages files → `commit` snapshots the index → `log` walks the commit chain → `checkout` restores a snapshot.

---

## Repository structure

Running `pit init` creates the following inside your project:

```
.pit/
├── HEAD              # points to current branch e.g. "ref: refs/heads/main"
├── config            # stores author name and email
├── index             # staging area — list of mode/hash/filename entries
├── objects/          # all stored objects (blobs, trees, commits)
│   ├── ab/
│   │   └── cd1234...   # object file, named by first 2 chars of hash
│   └── ...
└── refs/
    ├── heads/
    │   ├── main      # contains commit hash of latest commit on main
    │   └── feature   # contains commit hash of latest commit on feature
    └── tags/
```

---

## Object model

Every object is stored as:
```
<type> <size>\0<content>
```
Compressed with zlib, named by its SHA-1 hash, stored at `.pit/objects/<first 2 chars>/<remaining 38 chars>`.

There are three object types:

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
First commit has no `parent` line.

---

## Commands

### `pit init`
Initializes a new pit repository in the current directory.

Creates the `.pit/` directory structure and writes an initial `HEAD` pointing to `refs/heads/main`. Reads `GIT_AUTHOR_NAME` and `GIT_AUTHOR_EMAIL` environment variables to populate `.pit/config`.

```bash
pit init
```

---

### `pit add <file|.>`
Stages a file (or all files) for the next commit.

Hashes the file content into a blob object and writes an entry to `.pit/index`. If the file is already staged with the same hash, it skips it. Passing `.` recursively stages everything in the current directory, skipping `.pit` and `.git`.

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

Handles nested directories by recursively creating subtree objects bottom-up (deepest directories first), then assembling the root tree. Not usually called directly — `commit` calls this internally.

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

- **Changes to be committed** — compares index entries against the last commit's tree. Files not in the tree are "New file", files with a different hash are "Modified".
- **Changes not staged for commit** — compares index entries against the current file on disk by rehashing.

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

Reads the current branch's commit hash and writes it to `.pit/refs/heads/<name>`. No files are changed — it's just a new pointer.

```bash
pit branch create feature
```

### `pit branch list`
Lists all branches. Marks the current branch with `*`.

```bash
pit branch list
```

### `pit branch delete <name>`
Deletes a branch. Refuses to delete the currently checked-out branch.

```bash
pit branch delete feature
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

## Building

Requires `libssl`, `libcrypto`, and `zlib`.

```bash
make
```

Or manually:
```bash
gcc -Wextra -pedantic -I. \
  command_handler.c entry.c main.c include/file_handler.c \
  pit_commands/add.c pit_commands/branch.c pit_commands/cat_file.c \
  pit_commands/checkout.c pit_commands/commit.c pit_commands/commit_tree.c \
  pit_commands/diff.c pit_commands/hash_object.c pit_commands/init.c \
  pit_commands/log.c pit_commands/status.c pit_commands/write_tree.c \
  data_structures/dequeue.c \
  -o pit -lssl -lcrypto -lz
```

---

## Roadmap

- [x] init
- [x] add
- [x] commit
- [x] log
- [x] status
- [x] checkout
- [x] branch (create, list, delete)
- [ ] diff
- [ ] merge (fast-forward)
- [ ] remote
- [ ] fetch
- [ ] push
