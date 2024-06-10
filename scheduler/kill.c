#include <scheduler.h>
#include <stdio.h>

int kill(pid_t pid) {
  pcb_t *killed = find_pcb(pid);
  if (killed == NULL) {
    return 1;
  } else {
    clean_up(killed);
    killed->status = TASK_EXITED;
    list_move(&killed->state_list, &exited_pcb);
    return 0;
  }
}
