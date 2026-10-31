TARGET = drift-speaker
SRC_CC = main.cc
SRC_C = driver.c compat.generated.c
LIBS = base
INC_DIR += $(REP_DIR)/include $(REP_DIR)/../../imports/linux/pcspkr $(REP_DIR)/../../imports/linux/pcspkr/compat
CC_OPT += -ffreestanding -fno-builtin -fno-strict-aliasing
CC_C_OPT += -std=c11 -Wno-unused-parameter
vpath %.c $(REP_DIR)/../../imports/linux/pcspkr
