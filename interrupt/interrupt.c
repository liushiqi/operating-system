#include <assert.h>
#include <interrupt.h>
#include <sbi.h>
#include <scheduler.h>
#include <time.h>

void reset_irq_timer() {
  // Clock interrupt handler.
  // Call following functions when task4
  timer_check();

  // Note: use sbi_set_timer
  // Remember to reschedule
  sbi_set_timer(get_ticks() + time_base / 1000);
  scheduler();
}

__attribute__((unused)) void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause) {
  // interrupt handler.
  // Call corresponding handler by the value of `cause`
  if (cause >= (1ul << 63ul)) {
    handle_int(regs, stval, cause & 0x7ffffffffffffffful);
  } else {
    handle_exception(regs, stval, cause);
  }
}

void handle_int(regs_context_t *regs, uint64_t stval, uint64_t cause) {
  if (cause == 5) {
    reset_irq_timer();
  } else {
    handle_other(regs, stval, cause);
  }
}

void handle_exception(regs_context_t *regs, uint64_t stval, uint64_t cause) {
  if (cause == 8) {
    handle_syscall(regs, stval, cause);
  } else {
    handle_other(regs, stval, cause);
  }
}

void init_exception() { setup_exception(); }

static const char *reg_name[] = {"zero ", " ra  ", " sp  ", " gp  ", " tp  ", " t0  ", " t1  ", " t2  ", "s0/fp", " s1  ", " a0  ",
                                 " a1  ", " a2  ", " a3  ", " a4  ", " a5  ", " a6  ", " a7  ", " s2  ", " s3  ", " s4  ", " s5  ",
                                 " s6  ", " s7  ", " s8  ", " s9  ", " s10 ", " s11 ", " t3  ", " t4  ", " t5  ", " t6  "};

void handle_other(regs_context_t *regs, __attribute__((unused)) uint64_t stval, __attribute__((unused)) uint64_t cause) {
  printf("current pid is: %d\r\n", current_running->pid);
  for (int i = 0; i < 32; i += 3) {
    for (int j = 0; j < 3 && i + j < 32; ++j) {
      printf("%s : %016lx ", reg_name[i + j], regs->regs[i + j]);
    }
    printf("\n\r");
  }
  printf("sstatus: 0x%lx stval: 0x%lx scause: %ld\n\r", regs->sstatus, regs->stval, regs->scause);
  uint64_t stval1;
  asm volatile("csrr %0, stval" : "=r"(stval1)::);
  printf("stval value: %lx\r\n", stval1);
  printf("sepc: 0x%016lx\r\n", regs->sepc);
  uint32_t *epc_pos = (uint32_t *)pa_to_ka(va_to_pa(regs->sepc, (const page_table_entry_t *)current_running->page_directory));
  if ((unsigned long)epc_pos != 0xffffffc000000000)
    printf("instruction pos: %08x\n\r", *epc_pos);
  uintptr_t current_user_stack = pa_to_ka(va_to_pa(regs->regs[2], (const page_table_entry_t *)current_running->page_directory));
  printf("current sp in memory is: %lx\r\n", current_user_stack);
  /**
   * ```
   * uint64_t *stack_begin = (uint64_t *)(current_user_stack & 0xfffffffffffff000);
   * for (int i = 0; i < 64; ++i) {
   *   printf("0x%16p: 0x%016lx 0x%016lx 0x%016lx 0x%016lx 0x%016lx 0x%016lx 0x%016lx 0x%016lx\r\n", stack_begin + 8 * i, stack_begin[8 * i],
   *          stack_begin[8 * i + 1], stack_begin[8 * i + 2], stack_begin[8 * i + 3], stack_begin[8 * i + 4], stack_begin[8 * i + 5],
   *          stack_begin[8 * i + 6], stack_begin[8 * i + 7]);
   * }
   * ```
   */
  exit();
}
