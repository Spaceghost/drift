CC ?= cc
AR ?= ar
NM ?= nm
NELUA ?= nelua
CFLAGS ?= -Os
ROOT := $(abspath ../../..)
BUILD ?= $(ROOT)/build/windows-viorng
CPPFLAGS := -I. -Icompat -Iupstream -I$(ROOT)/include
WARN := -std=c11 -Wall -Wextra -Werror -Wno-unknown-pragmas -Wno-multichar
CORE := -ffreestanding -fno-builtin -fno-stack-protector -fno-strict-aliasing
AUDIT_LDFLAGS := -nostdlib -static -no-pie -Wl,-e,0
AUDIT_IMAGE := $(BUILD)/audit.elf
ifneq ($(findstring mingw,$(shell $(CC) -dumpmachine)),)
AUDIT_LDFLAGS := -nostdlib -r -Wl,--disable-runtime-pseudo-reloc,--disable-auto-import
AUDIT_IMAGE := $(BUILD)/linked.o
endif
SOURCE_NAMES := viorng read isrdpc power virtio
SOURCE_OBJECTS := $(addprefix $(BUILD)/source-,$(addsuffix .o,$(SOURCE_NAMES)))
OBJECTS := $(BUILD)/compat.o $(BUILD)/bridge.o $(SOURCE_OBJECTS) $(BUILD)/native-virtio.o $(BUILD)/native-pci.o
HEADERS := profile.h drift_windows_viorng.h $(wildcard compat/*.h) upstream/viorng.h upstream/trace.h $(ROOT)/include/drift_virtio.h
.PHONY: verify-upstream library regen verify-generated check sanitize clean closed-api lto-check
verify-upstream:
	sha256sum -c SHA256SUMS
$(BUILD):
	mkdir -p "$@"
$(BUILD)/compat.o: compat.generated.c $(HEADERS) | $(BUILD) verify-upstream
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@
$(BUILD)/bridge.o: bridge.c $(HEADERS) | $(BUILD) verify-upstream
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@
$(BUILD)/source-%.o: upstream/%.c $(HEADERS) | $(BUILD) verify-upstream
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@
$(BUILD)/native-%.o: $(ROOT)/generated/%.c $(wildcard $(ROOT)/include/drift_*.h) | $(BUILD)
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@
$(BUILD)/libdrift-windows-viorng.a: $(OBJECTS)
	$(AR) rcs $@ $^
library: verify-upstream $(BUILD)/libdrift-windows-viorng.a
regen: | $(BUILD)
	$(NELUA) -r --print-code -L . --cflags=-Icompat compat.nelua > $(BUILD)/compat.generated.c
	mv $(BUILD)/compat.generated.c compat.generated.c
verify-generated: | $(BUILD)
	$(NELUA) -r --print-code -L . --cflags=-Icompat compat.nelua > $(BUILD)/verified.c
	cmp compat.generated.c $(BUILD)/verified.c
check: library $(BUILD)/test closed-api
	$(BUILD)/test
	$(CC) $(AUDIT_LDFLAGS) $(OBJECTS) -o $(AUDIT_IMAGE)
	$(NM) -u $(AUDIT_IMAGE) > $(BUILD)/undefined-symbols.txt
	cat $(BUILD)/undefined-symbols.txt
	test ! -s $(BUILD)/undefined-symbols.txt
closed-api: | $(BUILD)
	printf '%s\n' '#include "compat/types.h"' 'int main(void){return WdfTimerCreate(0,0,0);}' > $(BUILD)/unsupported.c
	@if $(CC) $(CPPFLAGS) $(WARN) -c $(BUILD)/unsupported.c -o $(BUILD)/unsupported.o > $(BUILD)/unsupported.log 2>&1; then echo 'unsupported WDF API unexpectedly compiled' >&2; exit 1; fi
lto-check: verify-upstream | $(BUILD)
	$(CC) $(CPPFLAGS) $(WARN) $(CORE) -O3 -flto compat.generated.c bridge.c $(addprefix upstream/,$(addsuffix .c,$(SOURCE_NAMES))) $(ROOT)/generated/virtio.c $(ROOT)/generated/pci.c tests.c -o $(BUILD)/lto-test
	$(BUILD)/lto-test
$(BUILD)/test: tests.c $(ROOT)/tests/test_virtio.c $(BUILD)/libdrift-windows-viorng.a $(HEADERS)
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) -fno-strict-aliasing tests.c $(BUILD)/libdrift-windows-viorng.a -o $@
sanitize: verify-upstream | $(BUILD)
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fno-strict-aliasing -fsanitize=address,undefined compat.generated.c bridge.c $(addprefix upstream/,$(addsuffix .c,$(SOURCE_NAMES))) $(ROOT)/generated/virtio.c $(ROOT)/generated/pci.c tests.c -o $(BUILD)/sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/sanitize
clean:
	rm -rf $(BUILD)
