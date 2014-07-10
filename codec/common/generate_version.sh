#!/bin/bash
rm -f codec/common/inc/version.h
git rev-list HEAD | sort > config.git-hash
LOCALVER=`wc -l config.git-hash | awk '{print $1}'`
if [ $LOCALVER \> 1 ] ; then
    VER=`git rev-list origin/master | sort | join config.git-hash - | wc -l | awk '{print $1}'`
    if [ $VER != $LOCALVER ] ; then
        VER="$VER+$(($LOCALVER-$VER))"
    fi
    if git status | grep -q "modified:" ; then
        VER="${VER}M"
    fi
    VER="$VER $(git rev-list HEAD -n 1 | cut -c 1-7)"
    GIT_VERSION=r$VER
else
    GIT_VERSION=
    VER="x"
fi
GIT_VERSION='"'$GIT_VERSION'"'
rm -f config.git-hash
 
cat codec/common/inc/version.h.template | sed "s/\$FULL_VERSION/$GIT_VERSION/g" > codec/common/inc/version.h
 
echo "Generated version.h"
