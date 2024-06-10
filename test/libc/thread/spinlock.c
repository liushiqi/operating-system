#include <stdatomic.h>
#include <sys/syscall.h>
#include <thread.h>

int spin_init(spinlock_t *lock) {
  *lock = 0;
  return 0;
}

int spin_destroy(__attribute__((unused)) spinlock_t *lock) {
  // do nothing.
  return 0;
}

int spin_trylock(spinlock_t *lock) {
  /* return 0 or EBUSY */
  if (atomic_exchange(lock, 1) == 1) {
    return EBUSY;
  } else {
    return 0;
  }
}

int spin_lock(spinlock_t *lock) {
  int c;
  if ((c = atomic_compare_exchange_strong(lock, 0, 1)) != 0) {
    if (c != 2)
      c = atomic_exchange(lock, 2);
    while (c != 0) {
      sys_yield();
      c = atomic_exchange(lock, 2);
    }
  }
  return 0;
}

int spin_unlock(spinlock_t *lock) {
  atomic_exchange(lock, 0);
  return 0;
}
