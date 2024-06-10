#include <asm/asm.h>
#include <csr.h>
#include <sbi.h>

// reuse bootblock's space as boot stack
.equ boot_stack, 0x1020000

.section ".text.entry","ax"
FUNC_EXPORT(_start)
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

  /* setup C environment */
  la tp, pid0_pcb
  mv sp, a1
  csrw sscratch, x0

  j kernel_main
loop:
  wfi
  j loop
END(_start)
