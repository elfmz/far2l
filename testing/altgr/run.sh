#!/bin/sh
# Runs translation/state regressions without starting the GUI or changing layouts.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/far2l-altgr.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
cd "$root"
wx_config=${WX_CONFIG:-wx-config}
run_check() {
    # wx-config emits compiler/linker argument lists; intentional word splitting.
    ${CXX:-c++} -std=c++17 "$@" $($wx_config --cxxflags) \
        -IWinPort -IWinPort/src -IWinPort/src/Backend \
        -IWinPort/src/Backend/WX -Iutils/include \
        testing/altgr/check.cpp $($wx_config --libs) -o "$build_dir/check"
    "$build_dir/check"
}
if [ "$(uname -s)" = Darwin ]; then
    run_check
fi
run_check -DTEST_X11
