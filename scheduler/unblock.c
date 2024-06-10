#include <scheduler.h>
#include <string.h>

void unblock(pcb_t *unblocked) {
  // unblock the `pcb` from the block queue
  unblocked->status = TASK_READY;
  strncpy(unblocked->block_queue, "0", 32);
  list_move_tail(&unblocked->list, &ready_queue);
}
