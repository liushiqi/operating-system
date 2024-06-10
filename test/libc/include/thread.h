/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                              A Mini PThread-like library
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef _THREAD_H_
#define _THREAD_H_

#include <stdatomic.h>
#include <stdint.h>

/* on success, these functions return zero. Otherwise, return an error number */
#define EBUSY 1  /* the lock is busy(for example, it is locked by another thread) */
#define EINVAL 2 /* the lock is invalid */

typedef atomic_long spinlock_t;

/* A stupid implementation, this will be slow. */
typedef struct {
  atomic_long data;
} mutex_t;

int spin_init(spinlock_t *lock);
int spin_destroy(spinlock_t *lock);
int spin_trylock(spinlock_t *lock);
int spin_lock(spinlock_t *lock);
int spin_unlock(spinlock_t *lock);

int mutex_init(mutex_t *lock);
int mutex_destroy(mutex_t *lock);
int mutex_try_lock(mutex_t *lock);
int mutex_lock(mutex_t *lock);
int mutex_unlock(mutex_t *lock);

typedef struct {
  mutex_t lock;
  atomic_long event;
  atomic_uint still_needed;
  atomic_uint initial_needed;
} barrier_t;

int barrier_init(barrier_t *barrier, unsigned count);
int barrier_wait(barrier_t *barrier);
int barrier_destroy(barrier_t *barrier);

typedef struct {
  atomic_long value;
  atomic_uint previous;
} condition_variable_t;

void condition_variable_init(condition_variable_t *cond);
void condition_variable_destroy(condition_variable_t *cond);
void condition_variable_wait(condition_variable_t *cond, mutex_t *mutex);
void condition_variable_signal(condition_variable_t *cond);
void condition_variable_broadcast(condition_variable_t *cond);

typedef struct {
  atomic_long remain;
} semaphore_t;

void semaphore_init(semaphore_t *, int);
void semaphore_up(semaphore_t *);
void semaphore_down(semaphore_t *);

#endif
