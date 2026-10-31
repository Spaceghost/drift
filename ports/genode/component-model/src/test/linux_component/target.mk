TARGET = test-linux-component
SRC_CC = main.cc
SRC_C = bench_linux_import.c driver.c compat.generated.c
LIBS = base
CC_OLEVEL = -Os
CC_OPT += -ffreestanding -fno-builtin -fno-strict-aliasing
CC_C_OPT += -std=c11 -Wno-unused-parameter -DDRIFT_LINUX_IMPORT_BENCH_NO_MAIN
INC_DIR += $(REP_DIR)/../../../tests $(REP_DIR)/../../../imports/linux/pcspkr $(REP_DIR)/../../../imports/linux/pcspkr/compat
vpath bench_linux_import.c $(REP_DIR)/../../../tests
vpath driver.c $(REP_DIR)/../../../imports/linux/pcspkr
vpath compat.generated.c $(REP_DIR)/../../../imports/linux/pcspkr
