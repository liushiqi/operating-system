/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "page_table.h"
#include <list.h>
#include <screen.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/scheduler.h>

#define NUM_MAX_TASK 16
#define BUFFER_MAX_SIZE 128

extern atomic_int current_pid;

typedef struct {
  bool valid;
  int inode;
  char path[512];
  int read_position;
  int write_position;
} fd_t;

/* Process Control Block */
typedef struct pcb {
  /* register context */
  // this must be this order!! The order is defined in regs.h
  register_t kernel_sp;
  register_t user_sp;

  // count the number of disable_preempt
  // enable_preempt enables CSR_SIE only when preempt_count == 0
  register_t preempt_count;

  page_table_entry_t page_directory;

  char pwd[512];
  fd_t files[64];

  /* previous, next pointer */
  list_node_t list;
  list_node_t state_list;
  list_node_t wait_list;

  /* process id */
  pid_t pid;

  /* kernel/user thread/process */
  task_type_t type;

  /* BLOCK | READY | RUNNING | ZOMBIE */
  task_status_t status;
  spawn_mode_t mode;
  char block_queue[32];

  /* process priority */
  int32_t priority;
  int32_t should_run;

  char buffer[BUFFER_MAX_SIZE];
  int buffer_pos;
} pcb_t;

/* ready queue to run */
extern list_head ready_queue;
extern list_head using_pcb;
extern list_head exited_pcb;

/* current running task PCB */
extern pcb_t *volatile current_running;
extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
extern pcb_t pid0_pcb;
extern intptr_t pid0_stack;

extern pcb_t *get_next_active_thread();
extern pcb_t *find_pcb(int pid);
extern void clean_up(pcb_t *clean_pcb);

void init_pcb_stack(intptr_t kernel_stack, intptr_t user_stack, intptr_t user_stack_virtual_address, intptr_t entry_point, int argc, char **argv,
                    pcb_t *target_pcb);

extern void switch_to(pcb_t *prev, pcb_t *next);
extern void scheduler(void);

extern void sleep(uint32_t);

extern pid_t exec(char *name, int argc, char **argv);
extern void exit(void);
extern int kill(pid_t pid);
extern int wait_pid(pid_t pid);
extern int get_process_info(int pid, process_status_t *info);

extern void block(pcb_t *blocked, list_head *queue, char *block_queue);
extern void unblock(pcb_t *unblocked);

#endif
