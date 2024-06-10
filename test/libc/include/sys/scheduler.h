#ifndef RISCV_LIBC_SYS_SCHED_H
#define RISCV_LIBC_SYS_SCHED_H

#include <stdint.h>
#include <sys/type.h>

typedef enum {
  PCB_UNUSED,
  TASK_BLOCKED,
  TASK_RUNNING,
  TASK_READY,
  TASK_ZOMBIE,
  TASK_EXITED,
} task_status_t;

typedef enum {
  ENTER_ZOMBIE_ON_EXIT,
  AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum {
  KERNEL_PROCESS,
  KERNEL_THREAD,
  USER_PROCESS,
  USER_THREAD,
} task_type_t;

/* task information, used to init PCB */
typedef struct task_info {
  uintptr_t entry_point;
  task_type_t type;
} task_info_t;

/* process information, used in ps. */
typedef struct process_status {
  pid_t pid;
  task_status_t status;
  char block_queue[32];
} process_status_t;

#endif
