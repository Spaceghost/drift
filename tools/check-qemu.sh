#!/bin/sh
# Real controller integration under QEMU TCG; no KVM/root needed.
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
qemu=${QEMU:-qemu-system-x86_64}
if ! command -v "$qemu" >/dev/null 2>&1; then
    if [ -x .toolchain/qemu/usr/bin/qemu-system-x86_64 ]; then
        qemu=$PWD/.toolchain/qemu/usr/bin/qemu-system-x86_64
        export QEMU_MODULE_DIR=$PWD/.toolchain/qemu/usr/lib/x86_64-linux-gnu/qemu
        export QEMU_DATA=$PWD/.toolchain/qemu/usr/share/qemu
        export LD_LIBRARY_PATH=$PWD/.toolchain/qemu/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
    else
        echo 'QEMU is required: install qemu-system-x86 or set QEMU.' >&2
        exit 1
    fi
fi
temporary=$(mktemp -d)
qemu_pid=
cleanup() {
    if [ -n "$qemu_pid" ]; then kill "$qemu_pid" 2>/dev/null || :; wait "$qemu_pid" 2>/dev/null || :; fi
    rm -rf "$temporary"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
# No firmware sets up the controllers: reset CPU runs CLI/HLT, device timers run.
dd if=/dev/zero of="$temporary/halt.bin" bs=65536 count=1 2>/dev/null
printf '\372\364\353\375' | dd of="$temporary/halt.bin" bs=1 seek=65520 conv=notrunc 2>/dev/null
for disk in nvme sata virtio usb hub; do dd if=/dev/zero of="$temporary/$disk.img" bs=1048576 count=8 2>/dev/null; done
port=$((20000 + $$ % 20000))
net0=$((port+1)); net1=$((port+2)); net2=$((port+3)); qmp=$((port+4)); console=$((port+5)); net3=$((port+6))
set --
if [ -n "${QEMU_DATA:-}" ]; then set -- -L "$QEMU_DATA"; fi
"$qemu" "$@" -machine q35,accel=tcg -m 128 -nodefaults -display none -no-reboot \
    -qmp "tcp:127.0.0.1:$qmp,server=on,wait=off" -bios "$temporary/halt.bin" -qtest-log /dev/null -qtest "tcp:127.0.0.1:$port,server=on,wait=off" \
    -drive "if=none,id=nvme,file=$temporary/nvme.img,format=raw" -device nvme,drive=nvme,serial=drift,addr=5 \
    -drive "if=none,id=sata,file=$temporary/sata.img,format=raw" -device ich9-ahci,id=drift-ahci,addr=4 -device ide-hd,drive=sata,bus=drift-ahci.0 \
    -drive "if=none,id=virtio,file=$temporary/virtio.img,format=raw" -device virtio-blk-pci,disable-legacy=on,drive=virtio,addr=6 \
    -netdev "socket,id=net0,listen=127.0.0.1:$net0" -device e1000,netdev=net0,addr=b,romfile= \
    -netdev "socket,id=net1,listen=127.0.0.1:$net1" -device rtl8139,netdev=net1,addr=c,romfile= \
    -netdev "socket,id=net2,listen=127.0.0.1:$net2" -device virtio-net-pci,disable-legacy=on,netdev=net2,addr=7,romfile= \
    -chardev "socket,id=console,host=127.0.0.1,port=$console,server=on,wait=off" -device virtio-serial-pci,disable-legacy=on,max_ports=1,addr=9 -device virtconsole,chardev=console \
    -device virtio-keyboard-pci,addr=e \
    -device qemu-xhci,id=xkbd,addr=d,p3=0 -device qemu-xhci,id=xmouse,addr=f,p3=0 \
    -device qemu-xhci,id=xhub,addr=10,p3=0 -device usb-hub,id=hub,bus=xhub.0,port=1,ports=8 \
    -drive "if=none,id=hubdisk,file=$temporary/hub.img,format=raw" -device usb-storage,drive=hubdisk,bus=xhub.0,port=1.1 \
    -netdev "socket,id=net3,listen=127.0.0.1:$net3" -device qemu-xhci,id=xnet,addr=11,p3=0 -device usb-net,netdev=net3,bus=xnet.0 \
    -object rng-random,id=rng,filename=/dev/urandom -device virtio-rng-pci,disable-legacy=on,rng=rng,addr=8 \
    -device qemu-xhci,id=xhci,addr=a,p3=0 -drive "if=none,id=usb,file=$temporary/usb.img,format=raw" -device usb-storage,drive=usb,bus=xhci.0 \
    > "$temporary/qemu.log" 2>&1 &
qemu_pid=$!
if ! timeout 180 ./build/test_qemu "$port" "$net0"; then
    cat "$temporary/qemu.log" >&2
    exit 1
fi
