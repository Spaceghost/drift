#!/bin/sh
# Boot the same portable controllers in a tiny OS. SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
compiler=${CC:-cc}
linker=${LD:-ld}
mkdir -p build/kernel
flags='-m32 -std=c11 -Wall -Wextra -Werror -Os -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -Iinclude'
$compiler $flags -c examples/kernel/main.c -o build/kernel/main.o
$compiler -m32 -ffreestanding -fno-pie -c examples/kernel/start.S -o build/kernel/start.o
for source in ahci nvme pci virtio ethernet usb usb_hub usb_classes io; do
    $compiler $flags -c "generated/$source.c" -o "build/kernel/$source.o"
done
$linker -m elf_i386 -T examples/kernel/link.ld build/kernel/start.o build/kernel/main.o \
    build/kernel/ahci.o build/kernel/nvme.o build/kernel/pci.o build/kernel/virtio.o \
    build/kernel/ethernet.o build/kernel/usb.o build/kernel/usb_hub.o \
    build/kernel/usb_classes.o build/kernel/io.o -o build/kernel/drift.elf

qemu=${QEMU:-qemu-system-x86_64}
set --
if ! command -v "$qemu" >/dev/null 2>&1 && [ -x .toolchain/qemu/usr/bin/qemu-system-x86_64 ]; then
    qemu="$PWD/.toolchain/qemu/usr/bin/qemu-system-x86_64"
    export LD_LIBRARY_PATH="$PWD/.toolchain/qemu/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export QEMU_MODULE_DIR="$PWD/.toolchain/qemu/usr/lib/x86_64-linux-gnu/qemu"
    set -- -L "$PWD/.toolchain/qemu/usr/share/qemu" -bios "$PWD/.toolchain/qemu/usr/share/seabios/bios-256k.bin"
fi
if ! command -v "$qemu" >/dev/null 2>&1; then
    printf '%s\n' 'QEMU is required: install qemu-system-x86 or set QEMU to its executable.' >&2
    exit 1
fi
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
for disk in sata nvme virtio usb; do
    dd if=/dev/zero of="$temporary/$disk.raw" bs=1048576 count=16 2>/dev/null
done
status=0
timeout 45 "$qemu" "$@" -machine q35 -accel tcg -m 64M -display none -serial stdio \
    -nodefaults -no-reboot -kernel build/kernel/drift.elf \
    -device isa-debug-exit,iobase=0xf4,iosize=4 \
    -device ich9-ahci,id=drift-ahci,addr=4 \
    -drive "if=none,id=sata,format=raw,file=$temporary/sata.raw" \
    -device ide-hd,drive=sata,bus=drift-ahci.0 \
    -drive "if=none,id=nvme,format=raw,file=$temporary/nvme.raw" \
    -device nvme,drive=nvme,serial=DRIFTTEST,addr=5 \
    -drive "if=none,id=vblk,format=raw,file=$temporary/virtio.raw" \
    -device virtio-blk-pci,drive=vblk,disable-legacy=on,addr=6 \
    -netdev hubport,id=n1,hubid=0 \
    -device virtio-net-pci,netdev=n1,disable-legacy=on,addr=7,romfile= \
    -netdev hubport,id=n2,hubid=0 -device e1000,netdev=n2,addr=8,romfile= \
    -netdev hubport,id=n3,hubid=0 -device rtl8139,netdev=n3,addr=9,romfile= \
    -device qemu-xhci,id=drift-usb,p2=4,p3=0,addr=0a \
    -drive "if=none,id=usb,format=raw,file=$temporary/usb.raw" \
    -device usb-storage,drive=usb,bus=drift-usb.0 > "$temporary/serial.log" 2>&1 || status=$?
cat "$temporary/serial.log"
if [ "$status" -ne 33 ]; then
    printf 'Reference kernel failed (QEMU exit %s).\n' "$status" >&2
    exit 1
fi
for expected in \
    'AHCI read/write/flush/restore PASS' \
    'NVMe read/write/flush/restore PASS' \
    'Virtio block read/write/flush/restore PASS' \
    'Virtio/Intel/Realtek Ethernet frame exchange PASS' \
    'xHCI/USB storage read/write/flush/restore PASS' \
    'DRIFT KERNEL PASS'; do
    grep -F "$expected" "$temporary/serial.log" >/dev/null
done
