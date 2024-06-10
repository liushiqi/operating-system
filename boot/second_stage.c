#define SECOND_STAGE
#include <csr.h>
#include <elf.h>
#include <interrupt.h>
#include <naivefs.h>
#include <page_table.h>
#include <stdint.h>
#include <stdio.h>

#define align_4k(end) (((end) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

typedef void (*kernel_entry_t)(uintptr_t, uintptr_t, uintptr_t);
extern char end[];
static uintptr_t free_memory_position;

typedef struct {
  int file_length;
  unsigned char *content;
} file_t;

file_t ret_file;

static file_t *find_kernel() {
  char *name = "/kernel";
  struct stat buf;
  int ret = naive_getattr(name, &buf);
  if (ret < 0)
    return NULL;
  void *buffer = (void *)alloc_page(align(buf.st_size, 4096ul) / 4096);
  naive_read(name, buffer, buf.st_size, 0);
  ret_file.file_length = buf.st_size;
  ret_file.content = buffer;
  return &ret_file;
}

/** passed to kernel. */
uintptr_t riscv_dtb;

uintptr_t alloc_page(int page_num) {
  uintptr_t ret = free_memory_position;
  free_memory_position += (uintptr_t)page_num * PAGE_SIZE;
  memset((void *)ret, 0, PAGE_SIZE * page_num);
  return ret;
}

void free_page(uintptr_t root) { (void)root; }

// using 2MB large page
void map_huge_page(uintptr_t virtual_address, uintptr_t physical_address, page_table_entry_t *page_directory) {
  uintptr_t first_index = (virtual_address >> HUGE_PAGE_SHIFT) & 0x1fful;
  page_table_entry_t *second = (page_table_entry_t *)pfn_to_pa(page_directory[first_index] & ~PAGE_VALID);
  if (second == NULL) {
    second = (page_table_entry_t *)alloc_page(1);
    page_directory[first_index] = pa_to_pfn((uintptr_t)second) | PAGE_VALID;
  }
  uintptr_t second_index = (virtual_address >> LARGE_PAGE_SHIFT) & 0x1fful;
  second[second_index] =
      pa_to_pfn(physical_address & ~(LARGE_PAGE_SIZE - 1ul)) | PAGE_DIRTY | PAGE_ACCESSED | PAGE_READ | PAGE_WRITE | PAGE_EXEC | PAGE_VALID;
}

/**
 * Sv-39 mode
 * 0x0000_0000_0000_0000-0x0000_003f_ffff_ffff is for user mode
 * 0xffff_ffc0_0000_0000-0xffff_ffff_ffff_ffff is for kernel mode
 */
uintptr_t setup_vm() {
  free_memory_position = align_4k((uintptr_t)end);
  clear_page_directory(free_memory_position);
  uint64_t *kernel_root_page = (uint64_t *)free_memory_position;
  memset(kernel_root_page, 0, PAGE_SIZE);
  free_memory_position += PAGE_SIZE;
  // map kernel virtual address(kva) to kernel physical
  // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
  // map all physical memory
  for (uintptr_t memory = 0x10000000; memory < 0x20000000; memory += LARGE_PAGE_SIZE) {
    map_huge_page(memory + 0xffffffc000000000, memory, kernel_root_page);
  }

  // map boot address
  kernel_root_page[0] = kernel_root_page[256];

  // write satp to enable paging
  set_satp(8, 0, ((uintptr_t)kernel_root_page >> 12ul) & SATP_PPN);

  // remember to flush TLB
  flush_all_tlb();

  return (uintptr_t)kernel_root_page;
}

int main(void) {
  init_exception();
  uintptr_t kernel_root_page = setup_vm();
  if (naive_init() == -1) {
    printf("Error: No file system found.\r\n");
    while (true)
      ;
  }
  file_t *file = find_kernel();
  kernel_entry_t start_kernel = (kernel_entry_t)load_elf(file->content, file->file_length, kernel_root_page, false);
  memset((void *)free_memory_position, 0, PAGE_SIZE);
  uintptr_t kernel_sp = free_memory_position + 0xffffffc000000000 + PAGE_SIZE;
  start_kernel(riscv_dtb + 0xffffffc000000000, kernel_sp, kernel_root_page + 0xffffffc000000000);
  return 0;
}

void interrupt_helper_here(uint64_t stval, uint64_t cause, uint64_t sepc) {
  printf("stval: 0x%lx scause: %lx\n\r", stval, cause);
  printf("sepc: 0x%lx instruction: 0x%x\n\r", sepc, *(uint32_t *)sepc);
  while (1) {
  }
}
