#!/bin/sh
# Black-box keyboard diagnostics: real module, synthetic reports. SPDX-License-Identifier: MIT
set -eu
keyboard=${DRIFT_KEYBOARD:-./build/drift-keyboard}
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT HUP INT TERM

reject() {
    description=$1
    shift
    if "$@" > "$temporary/rejected.out" 2> "$temporary/rejected.err"; then
        printf 'keyboard host accepted %s\n' "$description" >&2
        exit 1
    fi
}

# The normal fixture includes repeated reports and a modifier transition.
"$keyboard" --replay fixtures/keyboard.trace > "$temporary/events"
cat > "$temporary/expected" <<'END'
press usage=0x04 modifiers=0x00
release usage=0x04 modifiers=0x00
press usage=0xe1 modifiers=0x02
press usage=0x05 modifiers=0x02
release usage=0xe1 modifiers=0x00
release usage=0x05 modifiers=0x00
END
diff -u "$temporary/expected" "$temporary/events"

# Recorded transfers must be consumable by both the diagnostic and generic hosts.
"$keyboard" --replay fixtures/keyboard.trace --record "$temporary/record.trace" > "$temporary/record.events"
"$keyboard" --replay "$temporary/record.trace" > "$temporary/replayed.events"
diff -u "$temporary/events" "$temporary/record.events"
diff -u "$temporary/events" "$temporary/replayed.events"
./build/drift replay modules/keyboard.drv "$temporary/record.trace" > "$temporary/generic"
sed -n '/^event /p' "$temporary/generic" > "$temporary/generic.events"
cat > "$temporary/expected" <<'END'
event 1 4 1 0
event 1 4 0 0
event 1 225 1 2
event 1 5 1 2
event 1 225 0 0
event 1 5 0 0
END
diff -u "$temporary/expected" "$temporary/generic.events"
# Keystroke captures stay private even under a permissive caller umask.
(umask 000; "$keyboard" --replay fixtures/keyboard.trace --record "$temporary/private.trace" > /dev/null)
test "$(LC_ALL=C ls -ld "$temporary/private.trace" | cut -c 1-10)" = '-rw-------'
cp "$temporary/record.trace" "$temporary/original.trace"
reject 'an existing recording destination' "$keyboard" --replay fixtures/keyboard.trace --record "$temporary/record.trace"
cmp "$temporary/original.trace" "$temporary/record.trace"
reject 'a recording onto its own input' "$keyboard" --replay "$temporary/record.trace" --record "$temporary/record.trace"
cmp "$temporary/original.trace" "$temporary/record.trace"
reject 'an uncreatable recording' "$keyboard" --replay fixtures/keyboard.trace --record "$temporary/missing/record.trace"

# Stop while a key is held: suppress duplicate presses and visibly release state.
cat > "$temporary/held.trace" <<'END'
xfer - 8 0 0000040000000000
xfer - 8 0 0000040000000000
stop
END
"$keyboard" --replay "$temporary/held.trace" > "$temporary/held.events"
cat > "$temporary/expected" <<'END'
press usage=0x04 modifiers=0x00
release usage=0x04 modifiers=0x00 cleanup=stop
END
diff -u "$temporary/expected" "$temporary/held.events"

# A failed transport still releases previously delivered held-key state.
cat > "$temporary/failed.trace" <<'END'
xfer - 8 0 0000040000000000
xfer - 8 -5 -
END
reject 'a failed transport as a successful session' "$keyboard" --replay "$temporary/failed.trace"
diff -u "$temporary/expected" "$temporary/rejected.out"

# A device cannot alias a modifier in the key array and modifier bitmap.
printf 'xfer - 8 0 0200e10000000000\n' > "$temporary/alias.trace"
reject 'a key-array modifier alias' "$keyboard" --replay "$temporary/alias.trace"
test ! -s "$temporary/rejected.out"

# Repeated idle reports must not exhaust a lifetime instruction allowance.
awk 'BEGIN { for (i=0; i<40000; ++i) print "xfer - 8 0 0000000000000000"; print "stop"; }' > "$temporary/idle.trace"
"$keyboard" --replay "$temporary/idle.trace" > "$temporary/idle.events"
test ! -s "$temporary/idle.events"

# Invalid records cannot silently become valid reports or clean termination.
printf 'xfer - 8 0 00000000000000\n' > "$temporary/short.trace"
reject 'a short report' "$keyboard" --replay "$temporary/short.trace"
printf 'xfer - 8 0 0000040000000000\n' > "$temporary/eof.trace"
reject 'EOF without explicit stop' "$keyboard" --replay "$temporary/eof.trace"
printf 'stop\nxfer - 8 0 0000000000000000\n' > "$temporary/trailing.trace"
reject 'rows after stop' "$keyboard" --replay "$temporary/trailing.trace"
printf 'xfer - 8 1 0000000000000000\n' > "$temporary/status.trace"
reject 'a positive completion status' "$keyboard" --replay "$temporary/status.trace"
printf 'xfer - 8 -5 0000000000000000\n' > "$temporary/error-data.trace"
reject 'data accompanying an error' "$keyboard" --replay "$temporary/error-data.trace"
printf 'xfer - 8 0 000000000000000g\n' > "$temporary/hex.trace"
reject 'malformed report bytes' "$keyboard" --replay "$temporary/hex.trace"
printf 'xfer 00 8 0 0000000000000000\n' > "$temporary/write.trace"
reject 'a transmit request on a read endpoint' "$keyboard" --replay "$temporary/write.trace"
printf 'xfer - 9 0 0000000000000000\n' > "$temporary/capacity.trace"
reject 'an incompatible response capacity' "$keyboard" --replay "$temporary/capacity.trace"
reject 'a module on the wrong bus' "$keyboard" --module modules/jedec.drv --replay fixtures/keyboard.trace
reject 'a non-HID endpoint' "$keyboard" --device /dev/null

# Flush failures must be observable, including stdout connected to a full sink.
if [ -e /dev/full ]; then
    if "$keyboard" --replay fixtures/keyboard.trace > /dev/full 2> "$temporary/full.err"; then
        printf '%s\n' 'keyboard host hid an event delivery failure' >&2
        exit 1
    fi
fi
# With no connected hardware, discovery is a successful empty result on Linux.
if [ "$(uname -s)" = Linux ]; then
    "$keyboard" --list > "$temporary/devices" 2> "$temporary/discovery.err"
fi
printf '%s\n' 'keyboard host checks passed'
