#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <thread.h>

int mutex_init(mutex_t *lock) {
  lock->data = 0;
  return 0;
}

int mutex_destroy(__attribute__((unused)) mutex_t *lock) {
  // do nothing!
  return 0;
}

int mutex_try_lock(mutex_t *lock) {
  if (atomic_exchange(&lock->data, 1) == 1) {
    return EBUSY;
  } else {
    return 0;
  }
}

int mutex_lock(mutex_t *lock) {
  int c;
  if ((c = atomic_compare_exchange_strong(&lock->data, 0, 1)) != 0) {
    if (c != 2)
      c = atomic_exchange(&lock->data, 2);
    while (c != 0) {
      sys_futex_wait(&lock->data, 2);
      c = atomic_exchange(&lock->data, 2);
    }
  }
  return 0;
}

int mutex_unlock(mutex_t *lock) {
  if (atomic_fetch_sub(&lock->data, 1) != 1) {
    lock->data = 0;
    sys_futex_wakeup(&lock->data, 1);
  }
  return 0;
}
