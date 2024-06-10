#include <csr.h>
#include <elf.h>
#include <fdt.h>
#include <futex.h>
#include <interrupt.h>
#include <memory.h>
#include <naivefs.h>
#include <scheduler.h>
#include <stdio.h>
#include <string.h>
#include <sys/scheduler.h>
#include <time.h>

static void init_pcb(uintptr_t free_memory_start, uintptr_t kernel_root_page) {
  pid0_pcb.pid = 0;
  pid0_pcb.status = TASK_RUNNING;
  pid0_pcb.type = KERNEL_PROCESS;
  pid0_pcb.priority = 1;
  pid0_pcb.should_run = 10;
  pid0_pcb.page_directory = kernel_root_page;
  pid0_pcb.kernel_sp = pid0_pcb.user_sp = free_memory_start;
  strncpy(pid0_pcb.block_queue, "0", 32);
  strcpy(pid0_pcb.pwd, "/");
  list_add(&pid0_pcb.state_list, &using_pcb);
  init_list_head(&pid0_pcb.wait_list);

  // remember to initialize `current_running`
  current_running = &pid0_pcb;
  current_pid = 1;

  char *argv[] = {"/bin/sh", NULL};
  exec("/bin/sh", 1, argv);
}

static void init_syscall(void) {
  // init system call table.
  // NONE!!!!!!!!!!!!!!
}

// jump from boot loader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
__attribute__((noreturn)) void kernel_main(uintptr_t riscv_dtb, uintptr_t usable_memory_begin, uintptr_t kernel_root_page) {
  // Close the cache, no longer refresh the cache
  // when making the exception vector entry copy

  printf("> [INIT] Entered kernel.\r\n");
  printf("> [INIT] The begin of kernel stack is: %lx\r\n", usable_memory_begin);

  // init interrupt (^_^)
  init_exception();
  printf("> [INIT] Interrupt processing initialization succeeded.\n\r");

  // init memory
  ((page_table_entry_t *)kernel_root_page)[0] = 0x0;
  init_memory(usable_memory_begin);
  printf("> [INIT] Initialized memory.\r\n");

  naive_init();
  printf("> [Init] NaiveFS initialization succeeded.\r\n");

  // init Process Control Block (-_-!)
  init_pcb(usable_memory_begin, kernel_root_page);
  printf("> [INIT] PCB initialization succeeded.\n\r");

  // setup timebase
  fdt_print(riscv_dtb);
  get_prop_u32(riscv_dtb, "/cpus/timebase-frequency", &time_base);

  // init futex mechanism
  init_system_futex();
  printf("> [INIT] Futex initialization succeeded.\n\r");

  // init system call table (0_0)
  init_syscall();
  printf("> [INIT] System call initialized successfully.\n\r");

  // init screen (QAQ)
  init_screen();
  printf("> [INIT] SCREEN initialization succeeded.\n\r");

  // Enable interrupt
  enable_interrupt();
  reset_irq_timer();

  while (1) {
    enable_interrupt();
    asm volatile("wfi" :::);
  }
}
