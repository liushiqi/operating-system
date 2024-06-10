#include <scheduler.h>
#include <string.h>
#include <time.h>

void sleep(uint32_t sleep_time) {
  // note: you can assume: 1 second = `timebase` ticks
  // 1. block the current_running
  // 2. create a timer which calls `unblock` when timeout
  // 3. reschedule because the current_running is blocked.
  timer_create((timer_callback_t)unblock, current_running, sleep_time * time_base / 2 + get_ticks());
  strncpy(current_running->block_queue, "timer", 32);
  // block the pcb task into the block queue
  pcb_t *blocked = current_running;
  blocked->status = TASK_BLOCKED;
  list_del(&blocked->list);
  // Modify the current_running pointer.
  pcb_t *next = get_next_active_thread();

  // switch_to current_running
  switch_to(blocked, next);
}
