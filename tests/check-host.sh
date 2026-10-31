#!/bin/sh
# Black-box CLI checks. SPDX-License-Identifier: MIT
set -eu
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
./build/drift replay modules/jedec.drv fixtures/jedec.trace > "$temporary/jedec"
cat > "$temporary/expected" <<'EOF'
event 2 239 64 24
done 0 steps=22
EOF
diff -u "$temporary/expected" "$temporary/jedec"
./build/identify modules/jedec.drv > "$temporary/identify"
cat > "$temporary/expected" <<'EOF'
transfer 9f -> ef 40 18
event 2: ef 40 18
EOF
diff -u "$temporary/expected" "$temporary/identify"
if ./build/identify modules/keyboard.drv > /dev/null 2>&1; then
    echo 'example host accepted a mismatched bus' >&2
    exit 1
fi
if [ -e /dev/full ] && ./build/identify modules/jedec.drv > /dev/full 2>/dev/null; then
    echo 'example host hid an output failure' >&2
    exit 1
fi
./build/drift replay modules/keyboard.drv fixtures/keyboard.trace > "$temporary/keyboard"
sed -n '/^event /p' "$temporary/keyboard" > "$temporary/events"
cat > "$temporary/expected" <<'EOF'
event 1 4 1 0
event 1 4 0 0
event 1 225 1 2
event 1 5 1 2
event 1 225 0 0
event 1 5 0 0
EOF
diff -u "$temporary/expected" "$temporary/events"
cat > "$temporary/wrong.trace" <<'EOF'
xfer 90 3 0 ef4018
EOF
if ./build/drift replay modules/jedec.drv "$temporary/wrong.trace" > /dev/null 2>&1; then
    echo 'replay accepted the wrong command' >&2
    exit 1
fi
cat > "$temporary/short.trace" <<'EOF'
xfer 9f 3 0 ef40
EOF
if ./build/drift replay modules/jedec.drv "$temporary/short.trace" > /dev/null 2>&1; then
    echo 'host hid a driver failure' >&2
    exit 1
fi
cat > "$temporary/trailing.trace" <<'EOF'
xfer 9f 3 0 ef4018
xfer 9f 3 0 ef4018
EOF
if ./build/drift replay modules/jedec.drv "$temporary/trailing.trace" > /dev/null 2>&1; then
    echo 'replay ignored a trailing transaction' >&2
    exit 1
fi
if ./build/drift stream modules/jedec.drv /dev/null > /dev/null 2>&1; then
    echo 'host accepted a mismatched bus binding' >&2
    exit 1
fi
if [ -e /dev/full ] && ./build/drift replay modules/jedec.drv fixtures/jedec.trace > /dev/full 2>/dev/null; then
    echo 'host acknowledged an undelivered event' >&2
    exit 1
fi
printf '%s\n' 'host checks passed'
