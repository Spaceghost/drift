CC ?= cc
AR ?= ar
NM ?= nm
NELUA ?= nelua
CFLAGS ?= -Os
BUILD ?= $(abspath ../../../build/windows-moufiltr)
ROOT := $(abspath ../../..)
CPPFLAGS := -I. -Icompat
WARN := -std=c11 -Wall -Wextra -Werror -Wno-unknown-pragmas
# Linux/Windows source object contexts use typed views of caller-owned storage.
# Keep the import's aliasing policy local; do not change Drift core flags.
CORE := -ffreestanding -fno-builtin -fno-stack-protector -fno-strict-aliasing
HEADERS := drift_windows_moufiltr.h $(wildcard compat/*.h) upstream/moufiltr.h
WRAP := -Wl,--wrap=WdfDriverCreate,--wrap=WdfDeviceCreate,--wrap=WdfIoQueueCreate
.PHONY: check sanitize regen verify-generated library clean closed-api lto-check verify-upstream
verify-upstream:
	sha256sum -c SHA256SUMS
$(BUILD):
	mkdir -p "$@"
$(BUILD)/compat.o: compat.generated.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@
$(BUILD)/driver.o: driver.c upstream/moufiltr.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) $(CORE) -c $< -o $@
$(BUILD)/libdrift-windows-moufiltr.a: $(BUILD)/compat.o $(BUILD)/driver.o
	$(AR) rcs $@ $^
$(BUILD)/compat.o $(BUILD)/driver.o: | verify-upstream
library: verify-upstream $(BUILD)/libdrift-windows-moufiltr.a
$(BUILD)/test: tests.c $(BUILD)/libdrift-windows-moufiltr.a $(HEADERS)
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) -fno-strict-aliasing tests.c $(BUILD)/libdrift-windows-moufiltr.a $(WRAP) -o $@
check: library $(BUILD)/test closed-api
	sha256sum -c SHA256SUMS
	$(BUILD)/test
	$(CC) -nostdlib -r $(BUILD)/driver.o $(BUILD)/compat.o -o $(BUILD)/linked.o
	$(NM) -u $(BUILD)/linked.o > $(BUILD)/undefined-symbols.txt
	cat $(BUILD)/undefined-symbols.txt
	test ! -s $(BUILD)/undefined-symbols.txt
closed-api: | $(BUILD)
	printf '%s\n' '#include "wdf_compat.h"' 'int main(void) { return WdfTimerCreate(0,0,0); }' > $(BUILD)/unsupported.c
	@if $(CC) $(CPPFLAGS) $(WARN) -c $(BUILD)/unsupported.c -o $(BUILD)/unsupported.o > $(BUILD)/unsupported.log 2>&1; then echo 'unsupported WDF API unexpectedly compiled' >&2; exit 1; fi
lto-check: verify-upstream | $(BUILD)
	$(CC) $(CPPFLAGS) $(WARN) $(CORE) -O3 -flto -DDRIFT_MOU_NO_WRAP compat.generated.c driver.c tests.c -o $(BUILD)/lto-test
	$(BUILD)/lto-test
sanitize: verify-upstream | $(BUILD)
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fno-omit-frame-pointer -fno-strict-aliasing -fsanitize=address,undefined compat.generated.c driver.c tests.c $(WRAP) -o $(BUILD)/sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/sanitize
regen: | $(BUILD)
	$(NELUA) -r --print-code --cflags=-Icompat compat.nelua > $(BUILD)/compat.generated.c
	mv $(BUILD)/compat.generated.c compat.generated.c
verify-generated: | $(BUILD)
	$(NELUA) -r --print-code --cflags=-Icompat compat.nelua > $(BUILD)/verified.c
	cmp compat.generated.c $(BUILD)/verified.c
clean:
	rm -rf $(BUILD)
