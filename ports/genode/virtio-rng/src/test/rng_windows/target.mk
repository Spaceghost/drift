TARGET = rng-windows
SRC_CC = main.cc
SRC_C = virtio.c pci.c compat.generated.c bridge.c upstream/viorng.c upstream/read.c upstream/isrdpc.c upstream/power.c upstream/virtio.c
LIBS = base
INC_DIR += $(REP_DIR)/include $(REP_DIR)/../../../include $(REP_DIR)/../../../imports/windows/viorng $(REP_DIR)/../../../imports/windows/viorng/compat $(REP_DIR)/../../../imports/windows/viorng/upstream
CC_OPT += -ffreestanding -fno-builtin -fno-strict-aliasing
CC_C_OPT += -std=c11 -Wno-unused-parameter -Wno-multichar -Wno-unknown-pragmas
vpath %.c $(REP_DIR)/../../../generated
vpath %.c $(REP_DIR)/../../../imports/windows/viorng
