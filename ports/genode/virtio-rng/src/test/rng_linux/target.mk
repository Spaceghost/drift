TARGET = rng-linux
SRC_CC = main.cc
SRC_C = virtio.c pci.c driver.c compat.generated.c
LIBS = base
INC_DIR += $(REP_DIR)/include $(REP_DIR)/../../../include $(REP_DIR)/../../../imports/linux/virtio-rng $(REP_DIR)/../../../imports/linux/virtio-rng/compat
CC_OPT += -ffreestanding -fno-builtin -fno-strict-aliasing
CC_C_OPT += -std=c11 -Wno-unused-parameter
vpath %.c $(REP_DIR)/../../../generated
vpath %.c $(REP_DIR)/../../../imports/linux/virtio-rng
