CC ?= cc
AR ?= ar
NM ?= nm
BUILD ?= ../../../build/linux-virtio-rng
NELUA ?= ../../../.toolchain/nelua/nelua
CFLAGS ?= -Os
ROOT = ../../..
WARN = -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter
INCLUDES = -I. -Icompat -I$(ROOT)/include
COMPAT_HEADERS = $(wildcard compat/*.h compat/linux/*.h compat/asm/*.h) drift_linux_virtio_rng.h
CORE = -ffreestanding -fno-builtin -fno-stack-protector -fno-strict-aliasing
.PHONY: all library check sanitize regen verify-generated source-check closed-api-check
all: library
$(BUILD):
	mkdir -p $@
$(BUILD)/compat.o: compat.generated.c $(COMPAT_HEADERS) Makefile | $(BUILD)
	$(CC) $(WARN) $(CFLAGS) $(CORE) $(INCLUDES) -c $< -o $@
$(BUILD)/driver.o: driver.c upstream/virtio-rng.c $(COMPAT_HEADERS) Makefile | $(BUILD)
	$(CC) $(WARN) $(CFLAGS) $(CORE) $(INCLUDES) -c $< -o $@
$(BUILD)/virtio.o: $(ROOT)/generated/virtio.c $(ROOT)/include/drift_virtio.h $(ROOT)/include/drift_hw.h $(ROOT)/include/drift_pci.h Makefile | $(BUILD)
	$(CC) $(WARN) $(CFLAGS) $(CORE) $(INCLUDES) -c $< -o $@
$(BUILD)/libdrift-linux-virtio-rng.a: $(BUILD)/driver.o $(BUILD)/compat.o $(BUILD)/virtio.o
	$(AR) rcs $@ $^
library: source-check $(BUILD)/libdrift-linux-virtio-rng.a
source-check:
	sha256sum -c SHA256SUMS
regen: | $(BUILD)
	$(NELUA) -r --print-code -L $(ROOT)/lib --cflags='-Icompat -I. -I$(ROOT)/include' compat.nelua > $(BUILD)/compat.generated.c
	mv $(BUILD)/compat.generated.c compat.generated.c
verify-generated: | $(BUILD)
	$(NELUA) -r --print-code -L $(ROOT)/lib --cflags='-Icompat -I. -I$(ROOT)/include' compat.nelua > $(BUILD)/compat.verified.c
	cmp compat.generated.c $(BUILD)/compat.verified.c
WRAP = -Wl,--wrap=drift_lx_vrng_alloc,--wrap=drift_lx_vrng_ida_alloc,--wrap=drift_lx_vrng_find_vq,--wrap=drift_lx_vrng_register,--wrap=drift_virtio_raw_device_ready,--wrap=drift_virtio_raw_add,--wrap=drift_virtio_raw_kick
$(BUILD)/test: test.c $(BUILD)/libdrift-linux-virtio-rng.a $(ROOT)/tests/test_virtio.c
	$(CC) $(WARN) $(CFLAGS) -fno-strict-aliasing $(INCLUDES) test.c $(BUILD)/libdrift-linux-virtio-rng.a -pthread $(WRAP) -o $@
closed-api-check: unsupported-api.c $(COMPAT_HEADERS) | $(BUILD)
	@if $(CC) $(WARN) $(CORE) $(INCLUDES) -Werror=implicit-function-declaration -c unsupported-api.c -o $(BUILD)/unsupported-api.o > $(BUILD)/unsupported-api.log 2>&1; then cat $(BUILD)/unsupported-api.log; echo "unsupported Linux API unexpectedly compiled"; exit 1; fi
	@cat $(BUILD)/unsupported-api.log
	@grep -q kmalloc $(BUILD)/unsupported-api.log
check: library closed-api-check $(BUILD)/test
	$(BUILD)/test
	$(CC) -nostdlib -r $(BUILD)/driver.o $(BUILD)/compat.o $(BUILD)/virtio.o -o $(BUILD)/linked.o
	$(NM) -u $(BUILD)/linked.o > $(BUILD)/undefined-symbols.txt
	cat $(BUILD)/undefined-symbols.txt
	test ! -s $(BUILD)/undefined-symbols.txt
sanitize: source-check | $(BUILD)
	$(CC) $(WARN) -O1 -g -fno-omit-frame-pointer -fno-strict-aliasing -fsanitize=address,undefined $(INCLUDES) compat.generated.c driver.c $(ROOT)/generated/virtio.c test.c -pthread $(WRAP) -o $(BUILD)/test-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-sanitize
