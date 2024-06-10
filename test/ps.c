#include <stdio.h>
#include <sys/scheduler.h>
#include <sys/syscall.h>

int main() {
  int max_pid = sys_get_max_pid();
  printf("pid status  ready_queue\n");
  for (int i = 0; i < max_pid; i++) {
    process_status_t status;
    int result = sys_get_process_info(i, &status);
    if (result == 0) {
      char *status_string =
          status.status == TASK_BLOCKED ? "blocked" : status.status == TASK_READY ? "ready" : status.status == TASK_RUNNING ? "running" : "zombie";
      printf("%3d %7s %s\n", status.pid, status_string, status.block_queue);
    }
  }
  return 0;
}
