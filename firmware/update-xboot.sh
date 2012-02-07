#!/bin/sh

# Note:
# Requires git-subtree
# https://github.com/apenwarr/git-subtree

cd ..

git remote add xboot git@github.com:alexforencich/xboot.git
git fetch xboot
git subtree merge -P firmware/xboot --squash -m "merged changes in xboot" xboot/master


