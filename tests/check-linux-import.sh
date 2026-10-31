#!/bin/sh
# Offline source integrity and no-overwrite checks. SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
root=$PWD
mkdir -p build
temporary=$(mktemp -d "$root/build/check-linux-import.XXXXXX")
trap 'task_import_status=$?; if [ "$task_import_status" -ne 0 ] && [ -f "$temporary/log" ]; then cat "$temporary/log" >&2; fi; rm -rf "$temporary"; exit "$task_import_status"' EXIT
trap 'exit 130' INT
trap 'exit 143' HUP TERM
mkdir -p "$temporary/tools" "$temporary/bin"
cp tools/import-linux.sh "$temporary/tools/"
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
        */virtio-rng.c) cp "$DRIFT_IMPORT_FIXTURES/virtio-rng.c" "$output" ;;
        */GPL-2.0) cp "$DRIFT_IMPORT_FIXTURES/COPYING.GPL-2.0" "$output" ;;
        *) exit 2 ;;
    esac
fi
EOF
chmod +x "$temporary/bin/curl"
PATH="$temporary/bin:$PATH"; export PATH
for task_import_profile in pcspkr virtio-rng; do
mkdir -p "$temporary/imports/linux/$task_import_profile"
cp "imports/linux/$task_import_profile/SOURCE" "imports/linux/$task_import_profile/SHA256SUMS" "$temporary/imports/linux/$task_import_profile/"
export DRIFT_IMPORT_FIXTURES="$root/imports/linux/$task_import_profile/upstream"
target="$temporary/imports/linux/$task_import_profile/upstream/$task_import_profile.c"
# Missing files are restored only from verified bytes, without network access.
sh "$temporary/tools/import-linux.sh" "$task_import_profile" > "$temporary/log" 2>&1
cmp "$target" "$DRIFT_IMPORT_FIXTURES/$task_import_profile.c"
sh "$temporary/tools/import-linux.sh" "$task_import_profile" >> "$temporary/log" 2>&1
# A local edit must survive a rejected import.
printf '\nlocal edit\n' >> "$target"
cp "$target" "$temporary/edited.c"
if sh "$temporary/tools/import-linux.sh" "$task_import_profile" >> "$temporary/log" 2>&1; then exit 1; fi
cmp "$target" "$temporary/edited.c"
cp "$DRIFT_IMPORT_FIXTURES/$task_import_profile.c" "$target"
# A bad remote payload must never replace an otherwise valid source.
if DRIFT_IMPORT_CORRUPT=1 sh "$temporary/tools/import-linux.sh" "$task_import_profile" >> "$temporary/log" 2>&1; then exit 1; fi
cmp "$target" "$DRIFT_IMPORT_FIXTURES/$task_import_profile.c"
# Linked destinations are refused even when their target has the right bytes.
rm "$target"
ln -s "$DRIFT_IMPORT_FIXTURES/$task_import_profile.c" "$target"
if sh "$temporary/tools/import-linux.sh" "$task_import_profile" >> "$temporary/log" 2>&1; then exit 1; fi
[ -L "$target" ]
done
printf '%s\n' 'Linux import: pinned restoration, edits, bad downloads and links checked'
