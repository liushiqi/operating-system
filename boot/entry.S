#include <asm/asm.h>
#include <context.h>
#include <csr.h>

ENTRY(init_exception)
  /* Save exception_handler_entry into STVEC */
  la t0, exception_handler_entry
  csrw stvec, t0

  /* Enable general exceptions */
  li t0, SIE_SSIE | SIE_STIE | SIE_SEIE
  csrc sie, t0

  jr ra
END_PROC(init_exception)

ENTRY(exception_handler_entry)
  csrw sscratch, x0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  /* Call interrupt_helper
   * note: don't forget to pass parameters for it.
   */
  csrr a0, stval
  csrr a1, scause
  csrr a2, sepc
  j interrupt_helper_here
END_PROC(exception_handler_entry)
