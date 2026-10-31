#!/bin/sh
# Fetch a pinned compiler into an isolated local directory. SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
repo=https://github.com/Spaceghost/nelua-lang.git
revision=3c29419776dfdca194fa7e9c572498264fe5c654
destination=.toolchain/nelua
if [ ! -d "$destination/.git" ]; then
    mkdir -p "$destination"
    git -C "$destination" init -q
    git -C "$destination" remote add origin "$repo"
fi
if [ "$(git -C "$destination" remote get-url origin)" != "$repo" ]; then
    echo 'compiler checkout has a different origin' >&2
    exit 1
fi
git -C "$destination" fetch --depth=1 origin "$revision"
git -C "$destination" checkout --detach FETCH_HEAD
make -C "$destination" -j2
printf '%s\n' 'Compiler ready. Run: make regen NELUA="$PWD/.toolchain/nelua/nelua"'
