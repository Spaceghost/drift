#!/bin/sh
# Complete backups publish once; failed backups publish nothing.
# SPDX-License-Identifier: MIT
set -eu
temporary=$(mktemp -d)
writer_pid=
reader_pid=
cleanup()
{
    if [ -n "$writer_pid" ]; then kill "$writer_pid" 2>/dev/null || :; fi
    if [ -n "$reader_pid" ]; then kill "$reader_pid" 2>/dev/null || :; fi
    if [ -n "$writer_pid" ]; then wait "$writer_pid" 2>/dev/null || :; fi
    if [ -n "$reader_pid" ]; then wait "$reader_pid" 2>/dev/null || :; fi
    rm -rf "$temporary"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
${CC:-cc} -std=c11 -Wall -Wextra -Werror tests/flash_fixture.c -o "$temporary/fixture"
drift=${DRIFT:-./build/drift}

no_partial()
{
    for partial in "$temporary"/*.part-*; do
        if [ -e "$partial" ] || [ -L "$partial" ]; then
            echo "backup leaked temporary file: $partial" >&2
            exit 1
        fi
    done
}

"$temporary/fixture" expected > "$temporary/expected.bin"
"$drift" flash-demo "$temporary/demo.bin" > "$temporary/stdout" 2> "$temporary/stderr"
test ! -s "$temporary/stdout"
cmp "$temporary/expected.bin" "$temporary/demo.bin"
no_partial

"$temporary/fixture" good > "$temporary/good.trace"
"$drift" flash-replay "$temporary/good.trace" "$temporary/replay.bin" > "$temporary/stdout" 2> "$temporary/stderr"
test ! -s "$temporary/stdout"
cmp "$temporary/expected.bin" "$temporary/replay.bin"
no_partial

for mode in truncated invalid-id unsupported-capacity short-id short mismatch transport stop extra; do
    "$temporary/fixture" "$mode" > "$temporary/$mode.trace"
    if "$drift" flash-replay "$temporary/$mode.trace" "$temporary/$mode.bin" > "$temporary/stdout" 2> "$temporary/stderr"; then
        echo "flash replay accepted $mode" >&2
        exit 1
    fi
    test ! -e "$temporary/$mode.bin"
    no_partial
done

# Interrupt while waiting for a live endpoint, with a private partial open.
mkfifo "$temporary/held.trace"
"$temporary/fixture" hold > "$temporary/held.trace" &
writer_pid=$!
"$drift" flash-replay "$temporary/held.trace" "$temporary/interrupted.bin" > "$temporary/stdout" 2> "$temporary/stderr" &
reader_pid=$!
attempt=0
while :; do
    set -- "$temporary"/interrupted.bin.part-*
    if [ -f "$1" ]; then break; fi
    attempt=$((attempt + 1))
    if [ "$attempt" -ge 200 ] || ! kill -0 "$reader_pid" 2>/dev/null; then
        echo 'backup did not open its private partial file' >&2
        exit 1
    fi
    sleep 0.01
done
kill -TERM "$reader_pid"
if wait "$reader_pid"; then
    reader_pid=
    echo 'interrupted backup reported success' >&2
    exit 1
fi
reader_pid=
kill "$writer_pid" 2>/dev/null || :
wait "$writer_pid" 2>/dev/null || :
writer_pid=
test ! -e "$temporary/interrupted.bin"
no_partial

# A pre-existing file, directory or dangling symlink is never replaced.
printf 'keep this file\n' > "$temporary/existing.bin"
cp "$temporary/existing.bin" "$temporary/original.bin"
if "$drift" flash-demo "$temporary/existing.bin" > /dev/null 2>&1; then
    echo 'backup overwrote an existing file' >&2
    exit 1
fi
cmp "$temporary/original.bin" "$temporary/existing.bin"
mkdir "$temporary/directory.bin"
if "$drift" flash-demo "$temporary/directory.bin" > /dev/null 2>&1; then
    echo 'backup replaced a directory' >&2
    exit 1
fi
test -d "$temporary/directory.bin"
ln -s missing.bin "$temporary/link.bin"
if "$drift" flash-demo "$temporary/link.bin" > /dev/null 2>&1; then
    echo 'backup replaced a symlink' >&2
    exit 1
fi
test -L "$temporary/link.bin"
test ! -e "$temporary/missing.bin"
no_partial

if "$drift" flash-demo "$temporary/missing/dump.bin" > /dev/null 2>&1; then
    echo 'backup accepted a nonexistent output directory' >&2
    exit 1
fi
test ! -e "$temporary/missing"
no_partial

# /dev/full is an existing endpoint; publication must not replace it.
if [ -c /dev/full ]; then
    if "$drift" flash-demo /dev/full > /dev/null 2>&1; then
        echo 'backup replaced /dev/full' >&2
        exit 1
    fi
    test -c /dev/full
    for partial in /dev/full.part-*; do
        if [ -e "$partial" ]; then
            echo "backup leaked temporary file: $partial" >&2
            exit 1
        fi
    done
fi
printf '%s\n' 'flash host checks passed'
