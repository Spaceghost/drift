#!/bin/sh
# Reproduce a pinned, unmodified source import. SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
case "${1-}" in
    pcspkr) profile=imports/linux/pcspkr ;;
    *) echo 'usage: sh tools/import-linux.sh pcspkr' >&2; exit 2 ;;
esac
[ "$#" -eq 1 ] || { echo 'unexpected arguments' >&2; exit 2; }
root=$PWD
revision=$(sed -n 's/^commit=//p' "$profile/SOURCE")
case "$revision" in
    *[!0-9a-f]*|'') echo 'invalid upstream revision' >&2; exit 1 ;;
esac
[ "${#revision}" -eq 40 ] || { echo 'expected full upstream revision' >&2; exit 1; }
mkdir -p build
temporary=$(mktemp -d "$root/build/linux-import.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
mkdir "$temporary/upstream"
base="https://raw.githubusercontent.com/torvalds/linux/$revision"
curl --fail --silent --show-error --location --proto '=https' --tlsv1.2 --connect-timeout 10 --max-time 60 \
    "$base/drivers/input/misc/pcspkr.c" -o "$temporary/upstream/pcspkr.c"
curl --fail --silent --show-error --location --proto '=https' --tlsv1.2 --connect-timeout 10 --max-time 60 \
    "$base/LICENSES/preferred/GPL-2.0" -o "$temporary/upstream/COPYING.GPL-2.0"
(cd "$temporary" && sha256sum -c "$root/$profile/SHA256SUMS")
# Never overwrite a locally edited driver or license. Review a new profile/pin
# explicitly rather than quietly patching upstream during a build.
for file in pcspkr.c COPYING.GPL-2.0; do
    destination="$profile/upstream/$file"
    if [ -e "$destination" ] || [ -L "$destination" ]; then
        if [ -L "$destination" ] || ! cmp -s "$destination" "$temporary/upstream/$file"; then
            echo "refusing to overwrite modified or linked $destination" >&2
            exit 1
        fi
    fi
done
mkdir -p "$profile/upstream"
for file in pcspkr.c COPYING.GPL-2.0; do
    if [ ! -e "$profile/upstream/$file" ]; then
        mv "$temporary/upstream/$file" "$profile/upstream/$file"
    fi
done
printf 'Verified unchanged Linux source and license at %s\n' "$revision"
