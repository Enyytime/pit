#!/bin/bash
set -e

WORK=/tmp/pittest
rm -rf "$WORK"
mkdir -p "$WORK/src/deep"
cd "$WORK"

export GIT_AUTHOR_NAME="Kenny"
export GIT_AUTHOR_EMAIL="kenny@example.com"

echo "== building test repo =="
echo "hello" > a.txt
echo "int main(void){return 0;}" > src/main.c
echo "nested" > src/deep/note.txt

pit init
pit add .
pit commit -m "first commit"

echo "modified" >> a.txt
pit add .
pit commit -m "second commit"

echo "== converting .pit -> .git =="
cp -r .pit .git
rm -f .git/index .git/config
cat > .git/config <<'EOF'
[core]
	repositoryformatversion = 0
	filemode = true
	bare = false
EOF

echo; echo "== git fsck =="
git fsck --full --strict || echo "FSCK FAILED"

echo; echo "== git log =="
git log --oneline || echo "LOG FAILED"

echo;