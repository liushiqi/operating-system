#include <list.h>
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static uintptr_t current_kernel_position;
static uintptr_t max_kernel_memory;

static inline unsigned int log2(unsigned int v) {
  unsigned int l = 0;
  while ((1ul << l) < v)
    l++;
  return l;
}

list_head free_memories[11] = {LIST_HEAD_INNER(free_memories[0]), LIST_HEAD_INNER(free_memories[1]), LIST_HEAD_INNER(free_memories[2]),
                               LIST_HEAD_INNER(free_memories[3]), LIST_HEAD_INNER(free_memories[4]), LIST_HEAD_INNER(free_memories[5]),
                               LIST_HEAD_INNER(free_memories[6]), LIST_HEAD_INNER(free_memories[7]), LIST_HEAD_INNER(free_memories[8]),
                               LIST_HEAD_INNER(free_memories[9]), LIST_HEAD_INNER(free_memories[10])};
LIST_HEAD(used_memory);
LIST_HEAD(unused_memory_struct);

typedef struct memory {
  uintptr_t address;
  uintptr_t size;
  list_node_t list;
} memory_t;

static memory_t *get_memory_struct() {
  if (!list_empty(&unused_memory_struct)) {
    return list_entry(unused_memory_struct.next, memory_t, list);
  } else {
    return kernel_malloc(sizeof(memory_t));
  }
}

void init_memory(uintptr_t free_memory_start) {
  current_kernel_position = free_memory_start;
  max_kernel_memory = current_kernel_position + PAGE_SIZE * 2;
  uintptr_t free_memory_pos = free_memory_start + PAGE_SIZE * 2;
  memset((void *)current_kernel_position, 0, PAGE_SIZE * 2);
  for (int i = 1024; i > 0; i /= 2)
    for (; free_memory_pos + PAGE_SIZE * i < 0xffffffc016000000; free_memory_pos += PAGE_SIZE * i) {
      memory_t *memory = get_memory_struct();
      memory->address = free_memory_pos;
      memory->size = i;
      list_add_tail(&memory->list, &free_memories[log2(i)]);
    }
}

memory_t *alloc_page_helper(int level) {
  if (!list_empty(&free_memories[level])) {
    memory_t *found = list_entry(free_memories[level].next, memory_t, list);
    list_move(&found->list, &used_memory);
    // printf("allocated a page with size: %lu from: %lx\r\n", found->size, found->address);
    return found;
  } else {
    memory_t *allocated = alloc_page_helper(level + 1);
    allocated->size /= 2;
    memory_t *another = get_memory_struct();
    // printf("got a struct in address %p\r\n", another);
    another->address = allocated->address + allocated->size * PAGE_SIZE;
    another->size = allocated->size;
    list_move(&another->list, &free_memories[level]);
    // printf("allocated a page with size: %lu from: %lx\r\n", allocated->size, allocated->address);
    return allocated;
  }
}

uintptr_t alloc_page(int num_page) {
  // print_memories();
  int allocated = (int)log2(num_page);
  // printf("trying to malloc %d pages\r\n", allocated);
  memory_t *mem = alloc_page_helper(allocated);
  memset((void *)mem->address, 0, PAGE_SIZE * num_page);
  // printf("[memory] malloced from %lx to %lx\r\n", mem->address, mem->address + num_page * PAGE_SIZE);
  return mem->address;
}

unsigned lower_ones(unsigned target, unsigned bits) { return target | ((1u << bits) - 1u); }

void print_memories() {
  for (int i = 0; i < 11; ++i) {
    printf("%d ", i);
    memory_t *memory = list_entry(free_memories[i].next, memory_t, list);
    while (&memory->list != &free_memories[i]) {
      printf("0x%lx 0x%lx ", memory->address, memory->size);
      memory = list_entry(memory->list.next, memory_t, list);
    }
    printf("\r\n");
  }
}

static void insert_memory(memory_t *memory, unsigned level) {
  memory_t *next = list_entry(free_memories[level].next, memory_t, list);
  while (&next->list != &free_memories[level] && next->address < memory->address) {
    next = list_entry(next->list.next, memory_t, list);
  }
  if (level == 10) {
    list_move_tail(&memory->list, &next->list);
    return;
  }
  memory_t *prev = list_entry(next->list.prev, memory_t, list);
  memory_t *associated = NULL;
  if (&prev->list != &free_memories[level] && lower_ones(prev->address, level + 13) == lower_ones(memory->address, level + 13)) {
    associated = memory;
    memory = prev;
  } else if (&next->list != &free_memories[level] && lower_ones(next->address, level + 13) == lower_ones(memory->address, level + 13))
    associated = next;
  if (associated != NULL) {
    memory->size *= 2;
    insert_memory(memory, level + 1);
    list_move(&associated->list, &unused_memory_struct);
  } else {
    list_move_tail(&memory->list, &next->list);
  }
}

void free_page(uintptr_t base_addr) {
  // print_memories();
  memory_t *memory = list_entry(used_memory.next, memory_t, list);
  while (&memory->list != &used_memory) {
    if (memory->address == base_addr)
      break;
    memory = list_entry(memory->list.next, memory_t, list);
  }
  if (&memory->list == &used_memory)
    return;
  unsigned int level = log2(memory->size);
  // printf("[memory] free memory from %lx to %lx\r\n", memory->address, memory->address + memory->size * PAGE_SIZE);
  insert_memory(memory, level);
  // print_memories();
}

void *kernel_malloc(size_t size) {
  uintptr_t ret;
  if (align(current_kernel_position, 4ul) + size + sizeof(memory_t) > max_kernel_memory) {
    uintptr_t new_page = alloc_page(1);
    current_kernel_position = new_page;
    max_kernel_memory = new_page + PAGE_SIZE;
  }
  ret = align(current_kernel_position, 4ul);
  current_kernel_position = align(current_kernel_position, 4ul) + size;
  memset((void *)ret, 0, size);
  return (void *)ret;
}
