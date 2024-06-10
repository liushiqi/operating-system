#include <limits.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#include <thread.h>

int barrier_init(barrier_t *barrier, unsigned count) {
  mutex_init(&barrier->lock);
  barrier->event = 0;
  barrier->initial_needed = barrier->still_needed = count;
  return 0;
}

int barrier_wait(barrier_t *barrier) {
  mutex_lock(&barrier->lock);
  if (barrier->still_needed-- > 1) {
    atomic_uint current_event = barrier->event;
    mutex_unlock(&barrier->lock);
    do {
      sys_futex_wait(&barrier->event, current_event);
    } while (barrier->event == current_event);
  } else {
    ++barrier->event;
    barrier->still_needed = barrier->initial_needed;
    sys_futex_wakeup(&barrier->event, INT_MAX);
    mutex_unlock(&barrier->lock);
  }
  return 0;
}

int barrier_destroy(__attribute__((unused)) barrier_t *barrier) { return 0; }
