#include <asm/asm.h>
#include <context.h>
#include <csr.h>

.macro SAVE_CONTEXT
  .local _restore_kernel_tpsp
  .local _save_context
  /*
   * If coming from userspace, preserve the user thread pointer and load
   * the kernel thread pointer.  If we came from the kernel, sscratch
   * will contain 0, and we should continue on the current TP.
   */
  csrrw tp, sscratch, tp
  bnez tp, _save_context

_restore_kernel_tpsp:
  csrr tp, sscratch
  sd sp, PCB_KERNEL_SP(tp)
_save_context:
  sd sp, PCB_USER_SP(tp)
  ld sp, PCB_KERNEL_SP(tp)
  addi sp, sp, -(OFFSET_SIZE)

  /* save all general purpose registers here! */
  sd ra, OFFSET_REG_RA(sp)
  sd gp, OFFSET_REG_GP(sp)
  sd tp, OFFSET_REG_TP(sp)
  sd t0, OFFSET_REG_T0(sp)
  sd t1, OFFSET_REG_T1(sp)
  sd t2, OFFSET_REG_T2(sp)
  sd s0, OFFSET_REG_S0(sp)
  sd s1, OFFSET_REG_S1(sp)
  sd a0, OFFSET_REG_A0(sp)
  sd a1, OFFSET_REG_A1(sp)
  sd a2, OFFSET_REG_A2(sp)
  sd a3, OFFSET_REG_A3(sp)
  sd a4, OFFSET_REG_A4(sp)
  sd a5, OFFSET_REG_A5(sp)
  sd a6, OFFSET_REG_A6(sp)
  sd a7, OFFSET_REG_A7(sp)
  sd s2, OFFSET_REG_S2(sp)
  sd s3, OFFSET_REG_S3(sp)
  sd s4, OFFSET_REG_S4(sp)
  sd s5, OFFSET_REG_S5(sp)
  sd s6, OFFSET_REG_S6(sp)
  sd s7, OFFSET_REG_S7(sp)
  sd s8, OFFSET_REG_S8(sp)
  sd s9, OFFSET_REG_S9(sp)
  sd s10, OFFSET_REG_S10(sp)
  sd s11, OFFSET_REG_S11(sp)
  sd t3, OFFSET_REG_T3(sp)
  sd t4, OFFSET_REG_T4(sp)
  sd t5, OFFSET_REG_T5(sp)
  sd t6, OFFSET_REG_T6(sp)

  /*
   * Disable user-mode memory access as it should only be set in the
   * actual user copy routines.
   *
   * Disable the FPU to detect illegal usage of floating point in kernel
   * space.
   */
  li t0, SR_SUM | SR_FS

  ld s0, PCB_USER_SP(tp)
  csrrc s1, sstatus, t0
  csrr s2, sepc
  csrr s3, stval
  csrr s4, scause
  csrr s5, sscratch
  sd s0, OFFSET_REG_SP(sp)
  sd s1, OFFSET_REG_SSTATUS(sp)
  sd s2, OFFSET_REG_SEPC(sp)
  sd s3, OFFSET_REG_SBADADDR(sp)
  sd s4, OFFSET_REG_SCAUSE(sp)
  sd s5, OFFSET_REG_TP(sp)
.endm

.macro RESTORE_CONTEXT
  /* Restore all registers and sepc, sstatus */
  ld a0, OFFSET_REG_SSTATUS(sp)
  ld a2, OFFSET_REG_SEPC(sp)
  csrw sstatus, a0
  csrw sepc, a2
  ld ra, OFFSET_REG_RA(sp)
  ld gp, OFFSET_REG_GP(sp)
  ld tp, OFFSET_REG_TP(sp)
  ld t0, OFFSET_REG_T0(sp)
  ld t1, OFFSET_REG_T1(sp)
  ld t2, OFFSET_REG_T2(sp)
  ld s0, OFFSET_REG_S0(sp)
  ld s1, OFFSET_REG_S1(sp)
  ld a0, OFFSET_REG_A0(sp)
  ld a1, OFFSET_REG_A1(sp)
  ld a2, OFFSET_REG_A2(sp)
  ld a3, OFFSET_REG_A3(sp)
  ld a4, OFFSET_REG_A4(sp)
  ld a5, OFFSET_REG_A5(sp)
  ld a6, OFFSET_REG_A6(sp)
  ld a7, OFFSET_REG_A7(sp)
  ld s2, OFFSET_REG_S2(sp)
  ld s3, OFFSET_REG_S3(sp)
  ld s4, OFFSET_REG_S4(sp)
  ld s5, OFFSET_REG_S5(sp)
  ld s6, OFFSET_REG_S6(sp)
  ld s7, OFFSET_REG_S7(sp)
  ld s8, OFFSET_REG_S8(sp)
  ld s9, OFFSET_REG_S9(sp)
  ld s10, OFFSET_REG_S10(sp)
  ld s11, OFFSET_REG_S11(sp)
  ld t3, OFFSET_REG_T3(sp)
  ld t4, OFFSET_REG_T4(sp)
  ld t5, OFFSET_REG_T5(sp)
  ld t6, OFFSET_REG_T6(sp)

  ld sp, OFFSET_REG_SP(sp)
.endm

ENTRY(setup_exception)
  /* save exception_handler_entry into STVEC */
  la t0, exception_handler_entry
  csrw stvec, t0

  /* enable global exceptions */
  li t0, SIE_SSIE | SIE_STIE | SIE_SEIE
  csrc sie, t0

  jr ra
END_PROC(setup_exception)

ENTRY(enable_preempt)
  ld t1, current_running
  ld t0, PCB_PREEMPT_COUNT(t1)
  beq t0, zero, do_enable
  addi t0, t0, -1
  sd t0, PCB_PREEMPT_COUNT(t1)
  beq t0, zero, do_enable
  jr ra
do_enable:
  not t0, x0
  csrs sie, t0
  jr ra
END_PROC(enable_preempt)

ENTRY(disable_preempt)
  csrw sie, zero
  ld t1, current_running
  ld t0, PCB_PREEMPT_COUNT(t1)
  addi t0, t0, 1
  sd t0, PCB_PREEMPT_COUNT(t1)
  jr ra
END_PROC(disable_preempt)

ENTRY(enable_interrupt)
  li t0, SR_SIE
  csrs sstatus, t0
  jr ra
END_PROC(enable_interrupt)

ENTRY(disable_interrupt)
  li t0, SR_SIE
  csrc sstatus, t0
  jr ra
END_PROC(disable_interrupt)

// the address of previous pcb in a0
// the address of next pcb in a1
ENTRY(__switch_to)
  // save all callee save registers on kernel stack
  addi sp, sp, -(SWITCH_TO_SIZE)
  /* store all callee save registers,
   * see the definition of `struct switchto_context` in sched.h */
  sd ra, SWITCH_TO_RA(sp)
  sd sp, SWITCH_TO_SP(sp)
  sd s0, SWITCH_TO_S0(sp)
  sd s1, SWITCH_TO_S1(sp)
  sd s2, SWITCH_TO_S2(sp)
  sd s3, SWITCH_TO_S3(sp)
  sd s4, SWITCH_TO_S4(sp)
  sd s5, SWITCH_TO_S5(sp)
  sd s6, SWITCH_TO_S6(sp)
  sd s7, SWITCH_TO_S7(sp)
  sd s8, SWITCH_TO_S8(sp)
  sd s9, SWITCH_TO_S9(sp)
  sd s10, SWITCH_TO_S10(sp)
  sd s11, SWITCH_TO_S11(sp)
  sd sp, PCB_KERNEL_SP(a0)

  // restore next
  mv tp, a1
  /* restore all callee save registers,
   * see the definition of `struct switchto_context` in sched.h */
  ld sp, PCB_KERNEL_SP(a1)
  ld ra, SWITCH_TO_RA(sp)
  ld sp, SWITCH_TO_SP(sp)
  ld s0, SWITCH_TO_S0(sp)
  ld s1, SWITCH_TO_S1(sp)
  ld s2, SWITCH_TO_S2(sp)
  ld s3, SWITCH_TO_S3(sp)
  ld s4, SWITCH_TO_S4(sp)
  ld s5, SWITCH_TO_S5(sp)
  ld s6, SWITCH_TO_S6(sp)
  ld s7, SWITCH_TO_S7(sp)
  ld s8, SWITCH_TO_S8(sp)
  ld s9, SWITCH_TO_S9(sp)
  ld s10, SWITCH_TO_S10(sp)
  ld s11, SWITCH_TO_S11(sp)

  addi sp, sp, SWITCH_TO_SIZE
  jr ra
END_PROC(__switch_to)

ENTRY(ret_from_exception)
  /* check if we return to kernel or a normal thread */
  la t0, pid0_pcb
  beq tp, t0, restore

  /* return to a normal thread(not the pid0 thread) now */
  addi t0, sp, OFFSET_SIZE
  sd t0, PCB_KERNEL_SP(tp)
  csrw sscratch, tp

restore:
  RESTORE_CONTEXT
  sret
END_PROC(ret_from_exception)

ENTRY(exception_handler_entry)
  SAVE_CONTEXT

  csrw sscratch, x0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  /* load ret_from_exception into ra
   * so that we can return to ret_from_exception
   * when interrupt_help complete.
   */
  la ra, ret_from_exception

  /* call interrupt_helper
   * note: don't forget to pass parameters for it.
   */
  mv a0, sp
  csrr a1, stval
  csrr a2, scause
  j interrupt_helper
END_PROC(exception_handler_entry)
