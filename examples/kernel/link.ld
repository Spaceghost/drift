/* A flat, identity-mapped Multiboot kernel. SPDX-License-Identifier: MIT */
ENTRY(_start)
SECTIONS {
  . = 1M;
  .multiboot : { KEEP(*(.multiboot)) }
  .text : ALIGN(4096) { *(.text*) }
  .rodata : ALIGN(4096) { *(.rodata*) }
  .data : ALIGN(4096) { *(.data*) }
  .bss : ALIGN(4096) { *(COMMON) *(.bss*) }
  /DISCARD/ : { *(.comment) *(.eh_frame*) }
}
