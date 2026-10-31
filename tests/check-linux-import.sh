#!/bin/sh
# Offline source integrity and no-overwrite checks. SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
root=$PWD
mkdir -p build
temporary=$(mktemp -d "$root/build/check-linux-import.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
mkdir -p "$temporary/tools" "$temporary/bin" "$temporary/imports/linux/pcspkr"
cp tools/import-linux.sh "$temporary/tools/"
cp imports/linux/pcspkr/SOURCE imports/linux/pcspkr/SHA256SUMS "$temporary/imports/linux/pcspkr/"
cat > "$temporary/bin/curl" <<'EOF'
#!/bin/sh
set -eu
url= output=
while [ "$#" -gt 0 ]; do
    case "$1" in
        -o) output=$2; shift ;;
        https:*) url=$1 ;;
    esac
    shift
done
if [ "${DRIFT_IMPORT_CORRUPT-0}" = 1 ]; then
    printf 'corrupt download\n' > "$output"
else
    case "$url" in
        */pcspkr.c) cp "$DRIFT_IMPORT_FIXTURES/pcspkr.c" "$output" ;;
        */GPL-2.0) cp "$DRIFT_IMPORT_FIXTURES/COPYING.GPL-2.0" "$output" ;;
        *) exit 2 ;;
    esac
fi
EOF
chmod +x "$temporary/bin/curl"
export DRIFT_IMPORT_FIXTURES="$root/imports/linux/pcspkr/upstream"
PATH="$temporary/bin:$PATH"; export PATH
target="$temporary/imports/linux/pcspkr/upstream/pcspkr.c"
# Missing files are restored only from verified bytes, without network access.
sh "$temporary/tools/import-linux.sh" pcspkr > "$temporary/log" 2>&1
cmp "$target" "$DRIFT_IMPORT_FIXTURES/pcspkr.c"
sh "$temporary/tools/import-linux.sh" pcspkr >> "$temporary/log" 2>&1
# A local edit must survive a rejected import.
printf '\nlocal edit\n' >> "$target"
cp "$target" "$temporary/edited.c"
if sh "$temporary/tools/import-linux.sh" pcspkr >> "$temporary/log" 2>&1; then exit 1; fi
cmp "$target" "$temporary/edited.c"
cp "$DRIFT_IMPORT_FIXTURES/pcspkr.c" "$target"
# A bad remote payload must never replace an otherwise valid source.
if DRIFT_IMPORT_CORRUPT=1 sh "$temporary/tools/import-linux.sh" pcspkr >> "$temporary/log" 2>&1; then exit 1; fi
cmp "$target" "$DRIFT_IMPORT_FIXTURES/pcspkr.c"
# Linked destinations are refused even when their target has the right bytes.
rm "$target"
ln -s "$DRIFT_IMPORT_FIXTURES/pcspkr.c" "$target"
if sh "$temporary/tools/import-linux.sh" pcspkr >> "$temporary/log" 2>&1; then exit 1; fi
[ -L "$target" ]
printf '%s\n' 'Linux import: pinned restoration, edits, bad downloads and links checked'
