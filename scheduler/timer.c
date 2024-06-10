#include <interrupt.h>
#include <memory.h>
#include <time.h>
#include <stdint.h>

LIST_HEAD(timers);

uint64_t time_elapsed = 0;
uint32_t time_base = 0;
uint64_t MHZ = 10;

/* a small object pool for timer_t */
static LIST_HEAD(free_timers);

static timer_t *alloc_timer() {
  // if we have free timer, then return a free timer
  // otherwise, return a new timer which is allocated by calling kernel_malloc
  timer_t *ret = NULL;
  if (!list_empty(&free_timers)) {
    ret = list_entry(free_timers.next, timer_t, list);
    list_del(&ret->list);
  } else {
    ret = (timer_t *)kernel_malloc(sizeof(timer_t));
  }

  // initialize the timer
  ret->list.next = NULL;
  ret->list.prev = NULL;
  ret->timeout_tick = 0;
  ret->callback_func = (timer_callback_t)NULL;
  ret->parameter = NULL;
  return ret;
}

static void free_timer(timer_t *timer) { list_move_tail(&timer->list, &free_timers); }

void timer_create(timer_callback_t func, void *parameter, uint64_t tick) {
  disable_preempt();

  timer_t *timer = alloc_timer();
  timer->callback_func = func;
  timer->parameter = parameter;
  timer->timeout_tick = tick;

  timer_t *head = list_entry(timers.prev, timer_t, list);
  while (&head->list != &timers && head->timeout_tick > tick) {
    head = list_entry(head->list.prev, timer_t, list);
  }
  list_add(&timer->list, &head->list);

  enable_preempt();
}

void timer_check(void) {
  disable_preempt();
  timer_t *head = list_entry(timers.next, timer_t, list);
  uint64_t ticks = get_ticks();
  while (!list_empty(&timers) && &head->list != &timers && head->timeout_tick <= ticks) {
    head->callback_func(head->parameter);
    timer_t *tmp = head;
    head = list_entry(tmp->list.next, timer_t, list);
    free_timer(tmp);
  }
  enable_preempt();
}

uint64_t get_ticks() {
  asm volatile("rdtime %0" : "=r"(time_elapsed));
  return time_elapsed;
}

uint64_t get_timer() { return get_ticks() / time_base; }

uint64_t get_time_base() { return time_base; }

void latency(uint64_t time) {
  uint64_t begin_time = get_timer();

  while (get_timer() - begin_time < time)
    ;
}
