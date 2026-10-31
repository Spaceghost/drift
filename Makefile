CC ?= cc
AR ?= ar
NELUA ?= nelua
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include/drift
CFLAGS ?= -Os
CPPFLAGS += -Iinclude -DDRIFT_MODULE_DIR='"$(DATADIR)/drift"'
WARN = -std=c11 -Wall -Wextra -Werror
CORE = -ffreestanding -fno-builtin -fno-stack-protector
NATIVE = pci virtio nvme ahci ethernet usb usb_classes usb_hub io
NATIVE_OBJECTS = $(addprefix build/native-,$(addsuffix .o,$(NATIVE)))
NATIVE_TESTS = $(addprefix build/test_,$(NATIVE))
PUBLIC_HEADERS = include/drift.h include/drift_hw.h $(addprefix include/drift_,$(addsuffix .h,$(NATIVE)))

.PHONY: all check example demo install sanitize freestanding freestanding32 regen verify-generated clean size FORCE native-check native-sanitize native-regen qemu-check kernel-check linux-check linux-sanitize linux-regen linux-verify-generated windows-check windows-sanitize windows-regen windows-verify-generated
all: build/drift build/drift-keyboard build/libdrift.a build/libdrift-native.a

build:
	mkdir -p build

# Rebuild the hosts if an install prefix changes after an earlier build.
build/module-path: FORCE | build
	@printf '%s\n' '$(DATADIR)/drift' > build/module-path.new
	@if ! cmp -s build/module-path.new $@; then mv build/module-path.new $@; fi
	@rm -f build/module-path.new

FORCE:

build/drift.o: generated/drift.c include/drift.h | build
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@

build/libdrift.a: build/drift.o
	$(AR) rcs $@ $<

build/native-%.o: generated/%.c $(wildcard include/drift_*.h) | build
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@

build/libdrift-native.a: $(NATIVE_OBJECTS)
	$(AR) rcs $@ $^

$(NATIVE_TESTS): build/test_%: tests/test_%.c build/native-%.o
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $^ -o $@

native-check: $(NATIVE_TESTS)
	@set -e; for driver in $(NATIVE); do ./build/test_$$driver; done
	$(CC) -nostdlib -r $(NATIVE_OBJECTS) -o build/native-linked.o
	test -z "$$($(NM) -u build/native-linked.o)"

native-sanitize: | build
	@set -e; for driver in $(NATIVE); do \
	  $(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/$$driver.c tests/test_$$driver.c -o build/san-$$driver; \
	  ASAN_OPTIONS=detect_leaks=0 ./build/san-$$driver; done

build/test_qemu: tests/test_qemu.c hosts/qtest.c include/drift_qtest.h build/libdrift-native.a
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) -DDRIFT_TEST_USB tests/test_qemu.c hosts/qtest.c build/libdrift-native.a -o $@

qemu-check: all build/test_qemu
	sh tools/check-qemu.sh

kernel-check: all
	sh tools/check-kernel.sh

# Opt-in Linux-source compatibility proof. Never added to the MIT-only native
# archive or default install: the upstream driver retains its GPL license.
linux-check:
	sh tests/check-linux-import.sh
	$(MAKE) -C imports/linux/pcspkr check CC="$(CC)" AR="$(AR)" NM="$(NM)" CFLAGS="$(CFLAGS)"

linux-sanitize:
	$(MAKE) -C imports/linux/pcspkr sanitize CC="$(CC)"

linux-regen:
	$(MAKE) -C imports/linux/pcspkr regen NELUA="$(NELUA)"

linux-verify-generated:
	$(MAKE) -C imports/linux/pcspkr verify-generated NELUA="$(NELUA)"

# A separate closed KMDF source profile, not an NT kernel or .sys loader.
windows-check:
	$(MAKE) -C imports/windows/moufiltr check lto-check CC="$(CC)" AR="$(AR)" NM="$(NM)" CFLAGS="$(CFLAGS)"

windows-sanitize:
	$(MAKE) -C imports/windows/moufiltr sanitize CC="$(CC)"

windows-regen:
	$(MAKE) -C imports/windows/moufiltr regen NELUA="$(NELUA)"

windows-verify-generated:
	$(MAKE) -C imports/windows/moufiltr verify-generated NELUA="$(NELUA)"

build/drift: hosts/posix.c build/drift.o include/drift.h build/module-path
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

build/drift-keyboard: hosts/keyboard.c hosts/hid_descriptor.c hosts/hid_descriptor.h build/drift.o include/drift.h build/module-path
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) hosts/keyboard.c hosts/hid_descriptor.c build/drift.o -o $@

build/test_flash: tests/test_flash.c build/drift.o include/drift.h
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

build/test_hid_descriptor: tests/test_hid_descriptor.c hosts/hid_descriptor.c hosts/hid_descriptor.h | build
	$(CC) $(CPPFLAGS) -Ihosts $(WARN) $(CFLAGS) tests/test_hid_descriptor.c hosts/hid_descriptor.c -o $@

build/test_vm: tests/test_vm.c build/drift.o include/drift.h
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

build/test_drivers: tests/test_drivers.c build/drift.o include/drift.h
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

build/identify: examples/identify.c build/drift.o include/drift.h
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

example: build/identify
	./build/identify modules/jedec.drv

demo: all
	@set -e; directory=$$(mktemp -d build/demo.XXXXXX); ./build/drift flash-demo "$$directory/flash.bin"
	./build/drift-keyboard --replay fixtures/keyboard.trace

install: all
	install -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(DATADIR)/drift" "$(DESTDIR)$(LIBDIR)" "$(DESTDIR)$(INCLUDEDIR)"
	install -m 755 build/drift build/drift-keyboard "$(DESTDIR)$(BINDIR)/"
	install -m 644 modules/*.drv "$(DESTDIR)$(DATADIR)/drift/"
	install -m 644 build/libdrift.a build/libdrift-native.a "$(DESTDIR)$(LIBDIR)/"
	install -m 644 $(PUBLIC_HEADERS) "$(DESTDIR)$(INCLUDEDIR)/"

check: all native-check build/test_vm build/test_drivers build/test_flash build/test_hid_descriptor build/identify
	./build/test_vm
	./build/test_drivers modules/jedec.drv modules/keyboard.drv
	./build/test_flash modules/flash.drv
	./build/test_hid_descriptor
	sh tests/check-host.sh
	sh tests/check-keyboard.sh
	sh tests/check-flash.sh
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c examples/embed.c -o build/embed.o
	test -z "$$($(NM) -u build/drift.o)"

NM ?= nm
sanitize: all
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c tests/test_vm.c -o build/test_vm_san
	ASAN_OPTIONS=detect_leaks=0 ./build/test_vm_san
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c tests/test_drivers.c -o build/test_drivers_san
	ASAN_OPTIONS=detect_leaks=0 ./build/test_drivers_san modules/jedec.drv modules/keyboard.drv
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c examples/identify.c -o build/identify_san
	ASAN_OPTIONS=detect_leaks=0 ./build/identify_san modules/jedec.drv
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c tests/test_flash.c -o build/test_flash_san
	ASAN_OPTIONS=detect_leaks=0 ./build/test_flash_san modules/flash.drv
	$(CC) $(CPPFLAGS) -Ihosts $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined hosts/hid_descriptor.c tests/test_hid_descriptor.c -o build/test_hid_san
	ASAN_OPTIONS=detect_leaks=0 ./build/test_hid_san
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c hosts/keyboard.c hosts/hid_descriptor.c -o build/keyboard_san
	ASAN_OPTIONS=detect_leaks=0 DRIFT_KEYBOARD=./build/keyboard_san sh tests/check-keyboard.sh
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c hosts/posix.c -o build/drift_san
	ASAN_OPTIONS=detect_leaks=0 DRIFT=./build/drift_san sh tests/check-flash.sh

# No libc, startup object, allocator or kernel driver in this executable.
# x86 Linux is used only to exit and report the test result.
freestanding: | build
	$(CC) $(CPPFLAGS) $(WARN) -Os $(CORE) -fno-pie -no-pie -nostdlib -Wl,-e,_start generated/drift.c tests/freestanding.c -o build/freestanding64
	./build/freestanding64
	$(CC) -m32 $(CPPFLAGS) $(WARN) -Os $(CORE) -fno-pie -no-pie -nostdlib -Wl,-e,_start generated/drift.c tests/freestanding.c -o build/freestanding32
	@echo '32-bit freestanding link passed; make freestanding32 to execute on a compatible kernel.'

freestanding32: freestanding
	./build/freestanding32

# Nelua is needed to change the machine or assemble drivers, not to embed it.
regen: native-regen | build
	$(NELUA) -r --print-code --cflags=-Iinclude src/drift.nelua > build/drift.generated.c
	mv build/drift.generated.c generated/drift.c
	$(NELUA) -r -b -L lib -o build/jedec-asm drivers/jedec.nelua
	./build/jedec-asm modules/jedec.drv
	$(NELUA) -r -b -L lib -o build/keyboard-asm drivers/keyboard.nelua
	./build/keyboard-asm modules/keyboard.drv
	$(NELUA) -r -b -L lib -o build/flash-asm drivers/flash.nelua
	./build/flash-asm modules/flash.drv

native-regen: | build
	@set -e; for driver in $(NATIVE); do \
	  $(NELUA) -r --print-code -L lib --cflags=-Iinclude native/$$driver.nelua > build/native-$$driver.c; \
	  mv build/native-$$driver.c generated/$$driver.c; done

verify-generated: | build
	$(NELUA) -r --print-code --cflags=-Iinclude src/drift.nelua > build/verified.c
	cmp generated/drift.c build/verified.c
	$(NELUA) -r -b -L lib -o build/jedec-asm drivers/jedec.nelua
	./build/jedec-asm build/verified-jedec.drv
	cmp modules/jedec.drv build/verified-jedec.drv
	$(NELUA) -r -b -L lib -o build/keyboard-asm drivers/keyboard.nelua
	./build/keyboard-asm build/verified-keyboard.drv
	cmp modules/keyboard.drv build/verified-keyboard.drv
	$(NELUA) -r -b -L lib -o build/flash-asm drivers/flash.nelua
	./build/flash-asm build/verified-flash.drv
	cmp modules/flash.drv build/verified-flash.drv
	@set -e; for driver in $(NATIVE); do \
	  $(NELUA) -r --print-code -L lib --cflags=-Iinclude native/$$driver.nelua > build/verify-$$driver.c; \
	  cmp generated/$$driver.c build/verify-$$driver.c; done

size: all
	size build/drift.o build/drift
	size $(NATIVE_OBJECTS)
	@wc -c modules/*.drv
	$(CC) $(CPPFLAGS) $(WARN) tools/size.c -o build/footprint
	./build/footprint

clean:
	rm -rf build
