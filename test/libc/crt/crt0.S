#include <asm/asm.h>
#include <sys/syscall.h>

ENTRY(_start)
  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  /* call main */
  call main
  /* call sys_exit*/
  li a7, SYS_exit
  ecall
END(_start)
