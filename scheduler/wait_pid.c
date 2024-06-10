#include <scheduler.h>
#include <stddef.h>
#include <stdio.h>

int wait_pid(pid_t pid) {
  pcb_t *pcb_found = find_pcb(pid);
  if (pcb_found != NULL) {
    char s[64];
    snprintf(s, 64, "waitpid_list.%d", pid);
    if (pcb_found->status != TASK_ZOMBIE) {
      block(current_running, &pcb_found->wait_list, s);
    }
    pcb_found->status = TASK_EXITED;
    list_move(&pcb_found->state_list, &exited_pcb);
  }
  return pcb_found == NULL;
}
