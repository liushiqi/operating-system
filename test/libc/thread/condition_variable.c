#include <limits.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#include <thread.h>

void condition_variable_init(condition_variable_t *cond) {
  cond->value = 0;
  cond->previous = 0;
}

void condition_variable_destroy(__attribute__((unused)) condition_variable_t *cond) {}

void condition_variable_wait(condition_variable_t *cond, mutex_t *mutex) {
  int value = atomic_load_explicit(&cond->value, memory_order_relaxed);
  mutex_unlock(mutex);
  sys_futex_wait(&cond->value, value);
  mutex_lock(mutex);
}

void condition_variable_signal(condition_variable_t *cond) {
  unsigned value = atomic_load_explicit(&cond->previous, memory_order_relaxed) + 1;
  atomic_store_explicit(&cond->value, value, memory_order_relaxed);

  sys_futex_wakeup(&cond->value, 1);
}

void condition_variable_broadcast(condition_variable_t *cond) {
  unsigned value = atomic_load_explicit(&cond->previous, memory_order_relaxed) + 1;
  atomic_store_explicit(&cond->value, value, memory_order_relaxed);

  sys_futex_wakeup(&cond->value, INT_MAX);
}
