#!/bin/bash

REPO_DIR=$GITHUB_WORKSPACE
export CCACHE_DIR=$REPO_DIR/.ccache
export DESTDIR=$REPO_DIR/AppDir
BUILD_DIR=build
HERE_DIR=$(readlink -f ${BASH_SOURCE[0]} | xargs dirname)

if [[ $(awk -F= '/^ID=/ {print $2}' /etc/os-release) == "alpine" ]]; then
  CMAKE_OPTS+=( "-DMUSL=ON" )
fi
if [[ "$STANDALONE" == "true" ]]; then
  CMAKE_OPTS+=( "-DUSEWX=no" )
fi

mkdir -p $BUILD_DIR && \
( cd $BUILD_DIR && \
  cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DCMAKE_C_COMPILER_LAUNCHER=/usr/bin/ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=/usr/bin/ccache \
    ${CMAKE_OPTS[@]} .. && \
    ninja && ninja install/strip ) && \

tar cJvf $REPO_DIR/far2l.tar.xz -C $REPO_DIR/AppDir .

if [[ "$STANDALONE" == "true" ]]; then
  ( cd $BUILD_DIR/install && ./far2l --help >/dev/null && bash -x $HERE_DIR/make_standalone.sh ) && \
  ( cd $REPO_DIR && makeself --keep-umask $BUILD_DIR/install $PKG_NAME.run "FAR2L File Manager" ./far2l && \
    tar cvf ${PKG_NAME/_${VERSION}-${GH_NAME}}.run.tar $PKG_NAME.run )
fi

ccache --max-size=50M --show-stats
