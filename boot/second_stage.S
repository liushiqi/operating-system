#include <asm/asm.h>
#include <csr.h>
#include <sbi.h>

/* reuse bootblock's space as boot stack */
.equ boot_stack, 0x10201000

.section .text.boot
ENTRY(_start)
  /* Mask all interrupts */
  csrw sie, zero
  csrw sip, zero

  /*
   * Disable FPU to detect illegal usage of
   * floating point in kernel space
   */
  li t0, SR_FS
  csrc sstatus, t0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  la a3, __bss_start
  la a4, __BSS_END__
.loc_clear_begin:
  sw zero, (a3)
  add a3, a3, 4
  ble a3, a4, .loc_clear_begin

  la s0, riscv_dtb
  sd s1, (s0)

  /* setup C environment for boot */
  li sp, boot_stack
  li tp, boot_stack
  
  /* this should never return */
  call main

  li a7, SBI_SHUTDOWN
  ecall
END(_start)
