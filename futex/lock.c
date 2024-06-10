#include <mutex.h>
#include <scheduler.h>
#include <stdatomic.h>

void spin_lock_init(spin_lock_t *lock) { lock->status = UNLOCKED; }

int spin_lock_try_acquire(spin_lock_t *lock) { return atomic_exchange(&lock->status, LOCKED); }

void spin_lock_acquire(spin_lock_t *lock) {
  while (spin_lock_try_acquire(lock) == LOCKED)
    ;
}

void spin_lock_release(spin_lock_t *lock) { atomic_exchange(&lock->status, UNLOCKED); }

void do_mutex_lock_init(mutex_lock_t *lock) {
  spin_lock_init(&lock->lock);
  init_list_head(&lock->block_queue);
}

void do_mutex_lock_acquire(mutex_lock_t *lock) {
  while (spin_lock_try_acquire(&lock->lock) == LOCKED) {
    block(current_running, &lock->block_queue, "mutex_lock_queue");
  }
}

void do_mutex_lock_release(mutex_lock_t *lock) {
  spin_lock_release(&lock->lock);
  if (!list_empty(&lock->block_queue)) {
    pcb_t *unblocked = list_entry(lock->block_queue.next, pcb_t, list);
    unblock(unblocked);
  }
}
