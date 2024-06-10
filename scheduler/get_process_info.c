#include <scheduler.h>
#include <string.h>
#include <sys/scheduler.h>

int get_process_info(int pid, process_status_t *info) {
  process_status_t *real_info = (process_status_t *)pa_to_ka(va_to_pa((uintptr_t)info, (const page_table_entry_t *)current_running->page_directory));
  pcb_t *pcb_found = find_pcb(pid);
  if (pcb_found != NULL) {
    real_info->pid = pid;
    real_info->status = pcb_found->status;
    strncpy(real_info->block_queue, pcb_found->block_queue, 32);
  }
  return pcb_found == NULL;
}
