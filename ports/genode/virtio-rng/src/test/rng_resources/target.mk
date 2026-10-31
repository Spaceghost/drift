TARGET = rng-resources
SRC_CC = main.cc
SRC_C = virtio.c pci.c
LIBS = base
INC_DIR += $(REP_DIR)/include $(REP_DIR)/../../../include
CC_OPT += -ffreestanding -fno-builtin -fno-strict-aliasing
CC_C_OPT += -std=c11 -Wno-unused-parameter
vpath %.c $(REP_DIR)/../../../generated
