#include <csr.h>
#include <interrupt.h>
#include <list.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

pcb_t pcb[NUM_MAX_TASK];
pcb_t pid0_pcb = {.pid = 0, .preempt_count = 0};

atomic_int current_pid;

LIST_HEAD(ready_queue);

LIST_HEAD(exited_pcb);
LIST_HEAD(using_pcb);

/* current running task PCB */
pcb_t *volatile current_running;

/* global process id */
pid_t process_id = 1;

pcb_t *get_next_active_thread() {
  int max_priority = -1;
  pcb_t *maximum = NULL;
  pcb_t *current = list_entry(ready_queue.next, pcb_t, list);
  while (&current->list != &ready_queue) {
    if (current->should_run < -1) {
      current->should_run = -1;
    } else if (current->should_run > max_priority) {
      maximum = current;
      max_priority = current->should_run;
    }
    current = list_entry(current->list.next, pcb_t, list);
  }
  if (max_priority == -1) {
    current = list_entry(ready_queue.next, pcb_t, list);
    while (&current->list != &ready_queue) {
      current->should_run = current->priority * 9;
      if (current->should_run > max_priority) {
        maximum = current;
        max_priority = current->should_run;
      }
      current = list_entry(current->list.next, pcb_t, list);
    }
  }
  return maximum;
}

pcb_t *find_pcb(int pid) {
  pcb_t *head = list_entry(using_pcb.next, pcb_t, state_list);
  bool found = false;
  while (&head->state_list != &using_pcb) {
    if (head->pid == pid) {
      found = true;
      break;
    }
    head = list_entry(head->state_list.next, pcb_t, state_list);
  }
  return found ? head : NULL;
}

static void free_pte(page_table_entry_t entry) {
  for (int i = 0; i < 256; ++i) {
    uintptr_t child = ((uintptr_t *)entry)[i];
    if (child != 0) {
      if (child & PAGE_READ) {
        free_page(pa_to_ka(pfn_to_pa(child)));
      } else {
        free_pte(pa_to_ka(pfn_to_pa(child)));
      }
    }
  }
  free_page(entry);
}

void clean_up(pcb_t *clean_pcb) {
  free_pte(clean_pcb->page_directory);
  if (clean_pcb->mode == ENTER_ZOMBIE_ON_EXIT) {
    clean_pcb->status = TASK_ZOMBIE;
  } else {
    clean_pcb->status = TASK_EXITED;
  }
  while (!list_empty(&clean_pcb->wait_list)) {
    pcb_t *ready = list_entry(clean_pcb->wait_list.next, pcb_t, list);
    unblock(ready);
  }
  if (clean_pcb->mode == AUTO_CLEANUP_ON_EXIT) {
    list_move(&clean_pcb->state_list, &exited_pcb);
  }
  list_del(&clean_pcb->list);
}

extern void __switch_to(pcb_t *prev, pcb_t *next);

void switch_to(pcb_t *prev, pcb_t *next) {
  prev->should_run--;
  if (prev != next)
    next->status = TASK_RUNNING;
  list_del(&next->list);
  current_running = next;
  set_satp(8, next->pid, (ka_to_pa(next->page_directory) >> 12ul) & SATP_PPN);
  flush_all_tlb();
  __switch_to(prev, next);
}

void scheduler(void) {
  // Modify the current_running pointer.
  pcb_t *next = get_next_active_thread();
  pcb_t *current = current_running;
  current->status = TASK_READY;
  list_move_tail(&current->list, &ready_queue);

  // switch_to current_running
  switch_to(current, next);
}
