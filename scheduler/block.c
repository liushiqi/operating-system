#include <scheduler.h>
#include <string.h>

void block(pcb_t *blocked, list_head *queue, char *block_queue) {
  blocked->status = TASK_BLOCKED;
  strncpy(blocked->block_queue, block_queue, 32);
  list_move(&blocked->list, queue);
  // block the pcb task into the block queue
  if (blocked == current_running) {
    // Modify the current_running pointer.
    pcb_t *next = get_next_active_thread();

    // switch_to current_running
    switch_to(blocked, next);
  }
}
