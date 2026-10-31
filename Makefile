CC ?= cc
AR ?= ar
NELUA ?= nelua
CFLAGS ?= -Os
CPPFLAGS += -Iinclude
WARN = -std=c11 -Wall -Wextra -Werror
CORE = -ffreestanding -fno-builtin -fno-stack-protector

.PHONY: all check example sanitize freestanding freestanding32 regen verify-generated clean size
all: build/drift build/libdrift.a

build:
	mkdir -p build

build/drift.o: generated/drift.c include/drift.h | build
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@

build/libdrift.a: build/drift.o
	$(AR) rcs $@ $<

build/drift: hosts/posix.c build/drift.o include/drift.h
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

build/test_vm: tests/test_vm.c build/drift.o include/drift.h
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

build/test_drivers: tests/test_drivers.c build/drift.o include/drift.h
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

build/identify: examples/identify.c build/drift.o include/drift.h
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $< build/drift.o -o $@

example: build/identify
	./build/identify modules/jedec.drv

check: all build/test_vm build/test_drivers build/identify
	./build/test_vm
	./build/test_drivers modules/jedec.drv modules/keyboard.drv
	sh tests/check-host.sh
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c examples/embed.c -o build/embed.o
	test -z "$$($(NM) -u build/drift.o)"

NM ?= nm
sanitize: | build
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c tests/test_vm.c -o build/test_vm_san
	ASAN_OPTIONS=detect_leaks=0 ./build/test_vm_san
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c tests/test_drivers.c -o build/test_drivers_san
	ASAN_OPTIONS=detect_leaks=0 ./build/test_drivers_san modules/jedec.drv modules/keyboard.drv
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined generated/drift.c examples/identify.c -o build/identify_san
	ASAN_OPTIONS=detect_leaks=0 ./build/identify_san modules/jedec.drv

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
regen: | build
	$(NELUA) -r --print-code --cflags=-Iinclude src/drift.nelua > build/drift.generated.c
	mv build/drift.generated.c generated/drift.c
	$(NELUA) -r -b -L lib -o build/jedec-asm drivers/jedec.nelua
	./build/jedec-asm modules/jedec.drv
	$(NELUA) -r -b -L lib -o build/keyboard-asm drivers/keyboard.nelua
	./build/keyboard-asm modules/keyboard.drv

verify-generated: | build
	$(NELUA) -r --print-code --cflags=-Iinclude src/drift.nelua > build/verified.c
	cmp generated/drift.c build/verified.c
	$(NELUA) -r -b -L lib -o build/jedec-asm drivers/jedec.nelua
	./build/jedec-asm build/verified-jedec.drv
	cmp modules/jedec.drv build/verified-jedec.drv
	$(NELUA) -r -b -L lib -o build/keyboard-asm drivers/keyboard.nelua
	./build/keyboard-asm build/verified-keyboard.drv
	cmp modules/keyboard.drv build/verified-keyboard.drv

size: all
	size build/drift.o build/drift
	@wc -c modules/*.drv
	$(CC) $(CPPFLAGS) $(WARN) tools/size.c -o build/footprint
	./build/footprint

clean:
	rm -rf build
