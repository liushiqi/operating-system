#include <stdatomic.h>
#include <sys/syscall.h>
#include <thread.h>

void semaphore_init(semaphore_t *semaphore, int count) { semaphore->remain = count; }

void semaphore_up(semaphore_t *semaphore) { atomic_fetch_add_explicit(&(semaphore->remain), 1, memory_order_release); }

void semaphore_down(semaphore_t *semaphore) {
  while (1) {
    while (atomic_load_explicit(&semaphore->remain, memory_order_acquire) < 1)
      sys_futex_wait(&semaphore->remain, semaphore->remain);
    int temp = atomic_fetch_add_explicit(&(semaphore->remain), -1, memory_order_acq_rel);
    if (temp >= 1)
      break;
    else
      atomic_fetch_add_explicit(&(semaphore->remain), 1, memory_order_release);
  }
}
