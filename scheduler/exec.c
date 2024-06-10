#include <context.h>
#include <csr.h>
#include <elf.h>
#include <memory.h>
#include <naivefs.h>
#include <scheduler.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

#define USER_STACK_TOP (0x100000000ul - 16ul)

extern void ret_from_exception();

typedef struct {
  int file_length;
  unsigned char *content;
} file_t;

file_t ret_file;

static file_t *find_file(char *name) {
  printf("%s", name);
  if (name[0] == '/') {
    struct stat buf;
    int ret = naive_getattr(name, &buf);
    if (ret < 0)
      return NULL;
    void *buffer = (void *)alloc_page(align(buf.st_size, 4096ul) / 4096);
    naive_read(name, buffer, buf.st_size, 0);
    ret_file.file_length = buf.st_size;
    ret_file.content = buffer;
    return &ret_file;
  } else {
    char *full_name = (char *)alloc_page(1);
    strcpy(full_name, current_running->pwd);
    strcat(full_name, name);
    struct stat buf;
    int ret = naive_getattr(full_name, &buf);
    if (ret < 0)
      return NULL;
    void *buffer = (void *)alloc_page(align(buf.st_size, 4096ul) / 4096);
    naive_read(full_name, buffer, buf.st_size, 0);
    free_page((uintptr_t)buffer);
    ret_file.file_length = buf.st_size;
    ret_file.content = buffer;
    return &ret_file;
  }
}

void init_pcb_stack(intptr_t kernel_stack, intptr_t user_stack, intptr_t user_stack_virtual_address, intptr_t entry_point, int argc, char **argv,
                    pcb_t *target_pcb) {
  regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
  /* initialization registers
   * note: sp, gp, ra, sepc, sstatus
   */
  for (int i = 0; i < 32; i++) {
    pt_regs->regs[i] = 0;
  }
  size_t len = 0;
  size_t argv_pointer = user_stack - argc * sizeof(uintptr_t);
  len += argc * sizeof(uintptr_t);
  // printf("argc = %d, argv = %lx", argc, (uint64_t)argv);
  for (int i = 0; i < argc; ++i) {
    size_t length = strlen(argv[i]) + 1;
    len += align(length, 4ul);
    memcpy((void *)(user_stack - len), argv[i], length);
    ((uintptr_t *)argv_pointer)[i] = user_stack_virtual_address - len;
    // printf("copied %zd bytes to %lx\r\n", length, user_stack - len);
  }
  len += 1;
  target_pcb->user_sp = pt_regs->regs[2] = (((uint64_t)user_stack_virtual_address - align(len, 4ul)) & ~0xfful);
  pt_regs->regs[2] = target_pcb->user_sp;
  pt_regs->regs[8] = target_pcb->user_sp;
  pt_regs->regs[10] = (register_t)argc;
  pt_regs->regs[11] = (register_t)user_stack_virtual_address - argc * sizeof(uintptr_t);
  pt_regs->sepc = entry_point;
  /* when target_pcb->type is KERNEL_PROCESS/THREADS,
   * please set SR_SPP and SR_SPIE bit of sstatus.
   */
  if (target_pcb->type == KERNEL_THREAD || target_pcb->type == KERNEL_PROCESS) {
    pt_regs->sstatus = SR_SPP | SR_SPIE;
  } else {
    pt_regs->sstatus = SR_SPIE;
  }

  // set sp to simulate return from switch_to
  intptr_t new_ksp = kernel_stack - sizeof(regs_context_t) - sizeof(switch_to_context_t);
  target_pcb->kernel_sp = new_ksp;

  switch_to_context_t *st_regs = (switch_to_context_t *)new_ksp;
  for (int i = 0; i < 12; i++) {
    st_regs->regs[i] = 0;
  }
  st_regs->sp = kernel_stack - sizeof(regs_context_t) - sizeof(switch_to_context_t);
  st_regs->regs[0] = kernel_stack - sizeof(regs_context_t);
  st_regs->ra = (register_t)ret_from_exception;
}

pid_t exec(char *name, int argc, char **argv) {
  char *real_name = (char *)pa_to_ka(va_to_pa((uintptr_t)name, (const page_table_entry_t *)current_running->page_directory));
  char **real_argv = (char **)pa_to_ka(va_to_pa((uintptr_t)argv, (const page_table_entry_t *)current_running->page_directory));
  char *passed_argv[argc];
  for (int i = 0; i < argc; ++i) {
    passed_argv[i] = (char *)pa_to_ka(va_to_pa((uintptr_t)real_argv[i], (const page_table_entry_t *)current_running->page_directory));
  }
  file_t *elf_file;
  if ((elf_file = find_file(real_name)) == NULL) {
    return -1;
  }
  int pid = atomic_fetch_add(&current_pid, 1);
  pcb_t *target_pcb = NULL;
  if (!list_empty(&exited_pcb)) {
    target_pcb = list_entry(exited_pcb.next, pcb_t, state_list);
  } else {
    for (int i = 0; i < 15; i++) {
      if (pcb[i].status == PCB_UNUSED) {
        target_pcb = pcb + i;
        break;
      }
    }
  }
  if (target_pcb == NULL)
    return -1;
  page_table_entry_t root_table = alloc_page(1);
  uintptr_t entry_point = load_elf(elf_file->content, elf_file->file_length, root_table, true);
  free_page((uintptr_t)elf_file->content);
  memset(target_pcb->buffer, 0, sizeof(target_pcb->buffer));

  target_pcb->pid = pid;
  target_pcb->type = USER_PROCESS;
  target_pcb->status = TASK_READY;
  target_pcb->mode = AUTO_CLEANUP_ON_EXIT;
  target_pcb->priority = 1;
  target_pcb->buffer_pos = 0;
  strcpy(target_pcb->pwd, current_running->pwd);
  intptr_t kernel_stack = alloc_page(1);
  intptr_t user_stack = alloc_page(1);
  target_pcb->page_directory = root_table;
  strncpy(target_pcb->block_queue, "0", 32);
  init_pcb_stack(kernel_stack + PAGE_SIZE - 16ul, user_stack + PAGE_SIZE - 16ul, USER_STACK_TOP, entry_point, argc, passed_argv, target_pcb);
  map_page(USER_STACK_TOP & ~0xffful, ka_to_pa(user_stack), (page_table_entry_t *)root_table, PAGE_READ | PAGE_WRITE | PAGE_USER);
  memcpy((uint8_t *)(((page_table_entry_t *)root_table) + 256), (const uint8_t *)(((page_table_entry_t *)pid0_pcb.page_directory) + 256), 256);
  list_move(&target_pcb->list, &ready_queue);
  list_move(&target_pcb->state_list, &using_pcb);
  init_list_head(&target_pcb->wait_list);
  return pid;
}
