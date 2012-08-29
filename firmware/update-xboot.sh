#!/bin/bash

# Git subtree manager
# Alex Forencich <alex@alexforencich.com>
# This script facilitates easy management of subtrees
# included in larger repositories as this script can
# be included in the repository itself.

# Note:
# Requires git-subtree
# https://github.com/apenwarr/git-subtree

# Settings
# set to use --squash
squash="yes"
# Remote repository
repo="git@github.com:alexforencich/xboot.git"
# Remote name
remote="xboot"
# Subdirectory to store code in
subdir="firmware/${remote}"
# Remote branch
branch="master"
# Backport branch name (only used for pushing)
backportbranch="${remote}backport"
# Add commit message
addmsg="added ${remote} as a subproject"
# Merge commit message
mergemsg="merged changes in ${remote}"

# Usage
# add - adds subtree
# pull - default, pulls from remote
# push - pushes to remote

squashflag=""

if [ $squash ]; then
  squashflag="--squash"
fi

action="pull"

cd $(git rev-parse --show-toplevel)

if [ ! -d "$subdir" ]; then
    action="add"
fi

if [ -n "$1" ]; then
    action="$1"
fi

# array contains value
# usage: contains array value
function contains() {
    local n=$#
    local value=${!n}
    for ((i=1;i < $n;i++)) {
        if [ "${!i}" == "${value}" ]; then
            echo "y"
            return 0
        fi
    }
    echo "n"
    return 1
}

case "$action" in
  add)
    if [ $(contains $(git remote) "$remote") != "y" ]; then
      git remote add "$remote" "$repo"
    fi
    git fetch "$remote"
    git subtree add -P "$subdir" $squashflag -m "$addmsg" "$remote/$branch"
    ;;
  pull)
    if [ $(contains $(git remote) "$remote") != "y" ]; then
      git remote add "$remote" "$repo"
    fi
    git fetch "$remote"
    git subtree merge -P "$subdir" $squashflag -m "$mergemsg" "$remote/$branch"
    ;;
  push)
    if [ $(contains $(git remote) "$remote") != "y" ]; then
      git remote add "$remote" "$repo"
    fi
    git subtree split -P "$subdir" -b "$backportbranch"
    git push "$remote" "$backportbranch:$branch"
    ;;
  *)
    echo "Error: unknown action!"
    exit 1
esac

exit 0
