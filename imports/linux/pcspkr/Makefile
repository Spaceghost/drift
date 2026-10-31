CC ?= cc
AR ?= ar
NM ?= nm
BUILD ?= ../../../build/linux-pcspkr
NELUA ?= ../../../.toolchain/nelua/nelua
CFLAGS ?= -Os
WARN = -std=c11 -Wall -Wextra -Werror
INCLUDES = -I. -Icompat
# The imported kernel source uses a fixed caller-owned arena for Linux objects.
# Match Linux's aliasing model; these flags do not affect Drift's MIT libraries.
CORE = -ffreestanding -fno-builtin -fno-stack-protector -fno-strict-aliasing
# MinGW's PE linker otherwise synthesizes a CRT pseudo-relocation helper even
# for this relocatable audit object. No DLL data imports are permitted here.
ifneq ($(findstring mingw,$(shell $(CC) -dumpmachine)),)
PARTIAL_LINK_FLAGS = -Wl,--disable-runtime-pseudo-reloc
endif

.PHONY: all library check sanitize regen verify-generated source-check
all: library
$(BUILD):
	mkdir -p $@
$(BUILD)/compat.o: compat.generated.c compat/pcspkr_compat.h drift_linux_pcspkr.h Makefile | $(BUILD)
	$(CC) $(WARN) $(CFLAGS) $(CORE) $(INCLUDES) -c $< -o $@
$(BUILD)/driver.o: driver.c upstream/pcspkr.c compat/pcspkr_compat.h drift_linux_pcspkr.h $(wildcard compat/linux/*.h) Makefile | $(BUILD)
	$(CC) $(WARN) -Wno-unused-parameter $(CFLAGS) $(CORE) $(INCLUDES) -c $< -o $@
$(BUILD)/libdrift-linux-pcspkr.a: $(BUILD)/compat.o $(BUILD)/driver.o
	$(AR) rcs $@ $^
library: source-check $(BUILD)/libdrift-linux-pcspkr.a
source-check:
	sha256sum -c SHA256SUMS
regen: | $(BUILD)
	$(NELUA) -r --print-code --cflags=-Icompat --cflags=-I. compat.nelua > $(BUILD)/compat.generated.c
	mv $(BUILD)/compat.generated.c compat.generated.c
verify-generated: | $(BUILD)
	$(NELUA) -r --print-code --cflags=-Icompat --cflags=-I. compat.nelua > $(BUILD)/compat.verified.c
	cmp compat.generated.c $(BUILD)/compat.verified.c
$(BUILD)/test: test.c $(BUILD)/compat.o $(BUILD)/driver.o
	$(CC) $(WARN) $(CFLAGS) -fno-strict-aliasing $(INCLUDES) $^ -pthread -Wl,--wrap=drift_lx_input_allocate_device -Wl,--wrap=drift_lx_input_register_device -o $@
check: library $(BUILD)/test
	$(BUILD)/test
	$(CC) -nostdlib -r $(PARTIAL_LINK_FLAGS) $(BUILD)/compat.o $(BUILD)/driver.o -o $(BUILD)/linked.o
	$(NM) -u $(BUILD)/linked.o > $(BUILD)/undefined-symbols.txt
	cat $(BUILD)/undefined-symbols.txt
	test ! -s $(BUILD)/undefined-symbols.txt
	@if $(CC) $(WARN) $(INCLUDES) -c unsupported.c -o $(BUILD)/unsupported.o >$(BUILD)/unsupported.log 2>&1; then \
	  echo 'unsupported Linux API unexpectedly compiled'; exit 1; \
	fi
	grep -q 'request_irq' $(BUILD)/unsupported.log
sanitize: source-check | $(BUILD)
	$(CC) $(WARN) -Wno-unused-parameter -fno-strict-aliasing -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined $(INCLUDES) compat.generated.c driver.c test.c -pthread -Wl,--wrap=drift_lx_input_allocate_device -Wl,--wrap=drift_lx_input_register_device -o $(BUILD)/test-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-sanitize
