#include <scheduler.h>

void exit(void) {
  // Modify the current_running pointer.
  pcb_t *next = get_next_active_thread();
  pcb_t *current = current_running;
  clean_up(current);

  // switch_to current_running
  switch_to(current, next);
}
