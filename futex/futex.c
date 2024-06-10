#include <assert.h>
#include <futex.h>
#include <interrupt.h>
#include <memory.h>

futex_bucket_t futex_buckets[FUTEX_BUCKETS];

void init_system_futex() {
  disable_preempt();
  for (int i = 0; i < FUTEX_BUCKETS; ++i) {
    init_list_head(&futex_buckets[i]);
  }
  enable_preempt();
}

static int futex_hash(uint64_t x) {
  // a simple hash function
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ul;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebul;
  x = x ^ (x >> 31);
  return x % FUTEX_BUCKETS;
}

static futex_node_t *get_node(volatile uint64_t *val_addr, int create) {
  volatile uint64_t *real_addr =
      (volatile uint64_t *)va_to_pa((uintptr_t)val_addr, (const page_table_entry_t *)current_running->page_directory);
  int key = futex_hash((uint64_t)real_addr);
  list_node_t *head = &futex_buckets[key];
  for (list_node_t *p = head->next; p != head; p = p->next) {
    futex_node_t *node = list_entry(p, futex_node_t, list);
    if (node->futex_key == (uint64_t)real_addr) {
      return node;
    }
  }

  if (create) {
    futex_node_t *node = (futex_node_t *)kernel_malloc(sizeof(futex_node_t));
    node->futex_key = (uint64_t)val_addr;
    init_list_head(&node->block_queue);
    list_add_tail(&node->list, &futex_buckets[key]);
    return node;
  }

  return NULL;
}

void futex_wait(volatile uint64_t *val_addr, uint64_t val) {
  volatile uint64_t *real_addr =
      (volatile uint64_t *)va_to_pa((uintptr_t)val_addr, (const page_table_entry_t *)current_running->page_directory);
  futex_node_t *node = get_node(real_addr, 1);
  assert(node != NULL);

  if (*real_addr == val) {
    block(current_running, &node->block_queue, "futex_wait_queue");
    scheduler();
  }
}

void futex_wakeup(volatile uint64_t *val_addr, int num_wakeup) {
  volatile uint64_t *real_addr =
      (volatile uint64_t *)va_to_pa((uintptr_t)val_addr, (const page_table_entry_t *)current_running->page_directory);
  futex_node_t *node = get_node(real_addr, 0);

  if (node != NULL) {
    for (int i = 0; i < num_wakeup; ++i) {
      if (list_empty(&node->block_queue))
        break;
      pcb_t *waked = list_entry(node->block_queue.next, pcb_t, list);
      unblock(waked);
    }
  }
}
