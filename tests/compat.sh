#!/bin/bash


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
git --no-pager fsck --full --strict || echo "FSCK FAILED"

echo; echo "== git log =="
git --no-pager log --oneline || echo "LOG FAILED"

echo;
echo; echo "== HEAD commit object =="
git --no-pager cat-file -p HEAD || echo "CAT-FILE FAILED"

echo; echo "== root tree =="
git --no-pager cat-file -p HEAD^{tree} || echo "TREE FAILED"

echo; echo "== recursive tree listing =="
git --no-pager ls-tree -r HEAD || echo "LS-TREE FAILED"

echo; echo "== full graph walk =="
git --no-pager rev-list --objects --all | wc -l

echo; echo "== hash agreement on a single blob =="
echo "pit: $(pit hash-object a.txt)"
echo "git: $(git hash-object a.txt)"

echo; echo "done. repo left at $WORK for poking around"
