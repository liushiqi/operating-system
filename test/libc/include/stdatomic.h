#ifndef STDATOMIC_H
#define STDATOMIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef volatile bool atomic_bool;
typedef volatile char atomic_char;
typedef volatile signed char atomic_schar;
typedef volatile unsigned char atomic_uchar;
typedef volatile short atomic_short;
typedef volatile unsigned short atomic_ushort;
typedef volatile int atomic_int;
typedef volatile unsigned int atomic_uint;
typedef volatile long atomic_long;
typedef volatile unsigned long atomic_ulong;
typedef volatile long long atomic_llong;
typedef volatile unsigned long long atomic_ullong;
typedef volatile uint_least16_t atomic_char16_t;
typedef volatile uint_least32_t atomic_char32_t;
typedef volatile wchar_t atomic_wchar_t;
typedef volatile int_least8_t atomic_int_least8_t;
typedef volatile uint_least8_t atomic_uint_least8_t;
typedef volatile int_least16_t atomic_int_least16_t;
typedef volatile uint_least16_t atomic_uint_least16_t;
typedef volatile int_least32_t atomic_int_least32_t;
typedef volatile uint_least32_t atomic_uint_least32_t;
typedef volatile int_least64_t atomic_int_least64_t;
typedef volatile uint_least64_t atomic_uint_least64_t;
typedef volatile int_fast8_t atomic_int_fast8_t;
typedef volatile uint_fast8_t atomic_uint_fast8_t;
typedef volatile int_fast16_t atomic_int_fast16_t;
typedef volatile uint_fast16_t atomic_uint_fast16_t;
typedef volatile int_fast32_t atomic_int_fast32_t;
typedef volatile uint_fast32_t atomic_uint_fast32_t;
typedef volatile int_fast64_t atomic_int_fast64_t;
typedef volatile uint_fast64_t atomic_uint_fast64_t;
typedef volatile size_t atomic_size_t;
typedef volatile intmax_t atomic_intmax_t;
typedef volatile uintmax_t atomic_uintmax_t;

typedef enum memory_order {
  memory_order_relaxed,
  memory_order_acquire,
  memory_order_consume,
  memory_order_release,
  memory_order_acq_rel,
  memory_order_seq_cst,
} memory_order;

#define __atomic_store(obj, desired, RELEASE)                                                                                                        \
  ({                                                                                                                                                 \
    typeof(obj) __obj = (obj);                                                                                                                       \
    typeof(desired) __desired = (desired);                                                                                                           \
    size_t __size = sizeof(*(obj));                                                                                                                  \
    switch (__size) {                                                                                                                                \
    case 1:                                                                                                                                          \
      asm volatile(RELEASE "sb %1, %0\n" : "+A"(*__obj) : "r"(__desired) : "memory");                                                                \
      break;                                                                                                                                         \
    case 2:                                                                                                                                          \
      asm volatile(RELEASE "sh %1, %0\n" : "+A"(*__obj) : "r"(__desired) : "memory");                                                                \
      break;                                                                                                                                         \
    case 4:                                                                                                                                          \
      asm volatile(RELEASE "sw %1, %0\n" : "+A"(*__obj) : "r"(__desired) : "memory");                                                                \
      break;                                                                                                                                         \
    case 8:                                                                                                                                          \
      asm volatile(RELEASE "sd %1, %0\n" : "+A"(*__obj) : "r"(__desired) : "memory");                                                                \
      break;                                                                                                                                         \
    }                                                                                                                                                \
  })

#define atomic_store_explicit(obj, desired, order)                                                                                                   \
  ({                                                                                                                                                 \
    switch (order) {                                                                                                                                 \
    case memory_order_release:                                                                                                                       \
    case memory_order_seq_cst:                                                                                                                       \
      __atomic_store((obj), (desired), "fence rw, w\n");                                                                                             \
      break;                                                                                                                                         \
    default:                                                                                                                                         \
      __atomic_store((obj), (desired), "");                                                                                                          \
      break;                                                                                                                                         \
    }                                                                                                                                                \
  })

#define atomic_store(obj, desired) __atomic_store((obj), (desired), "fence rw, w\n")

#define __atomic_load(obj, BARRIER, ACRUIRE)                                                                                                         \
  ({                                                                                                                                                 \
    typeof(obj) __obj = (obj);                                                                                                                       \
    typeof(*(obj)) __result;                                                                                                                         \
    size_t __size = sizeof(*(obj));                                                                                                                  \
    switch (__size) {                                                                                                                                \
    case 1:                                                                                                                                          \
      asm volatile(BARRIER "lb %0, %1\n" ACRUIRE : "=&r"(__result), "+A"(*__obj)::"memory");                                                         \
      break;                                                                                                                                         \
    case 2:                                                                                                                                          \
      asm volatile(BARRIER "lh %0, %1\n" ACRUIRE : "=&r"(__result), "+A"(*__obj)::"memory");                                                         \
      break;                                                                                                                                         \
    case 4:                                                                                                                                          \
      asm volatile(BARRIER "lw %0, %1\n" ACRUIRE : "=&r"(__result), "+A"(*__obj)::"memory");                                                         \
      break;                                                                                                                                         \
    case 8:                                                                                                                                          \
      asm volatile(BARRIER "ld %0, %1\n" ACRUIRE : "=&r"(__result), "+A"(*__obj)::"memory");                                                         \
      break;                                                                                                                                         \
    }                                                                                                                                                \
    __result;                                                                                                                                        \
  })

#define atomic_load_explicit(obj, order)                                                                                                             \
  ({                                                                                                                                                 \
    typeof(*(obj)) __result;                                                                                                                         \
    switch (order) {                                                                                                                                 \
    case memory_order_seq_cst:                                                                                                                       \
      __result = __atomic_load((obj), "fence rw, rw\n", "fence r, rw");                                                                              \
      break;                                                                                                                                         \
    case memory_order_acquire:                                                                                                                       \
      __result = __atomic_load((obj), "", "fence r, rw");                                                                                            \
      break;                                                                                                                                         \
    default:                                                                                                                                         \
      __result = __atomic_load((obj), "", "");                                                                                                       \
      break;                                                                                                                                         \
    }                                                                                                                                                \
    __result;                                                                                                                                        \
  })

#define atomic_load(obj) __atomic_load((obj), "fence rw, rw\n", "fence r, rw")

#define __atomic_operation(OPERATION, obj, desired, AQRL)                                                                                            \
  ({                                                                                                                                                 \
    typeof(obj) __obj = (obj);                                                                                                                       \
    typeof(desired) __desired = (desired);                                                                                                           \
    typeof(*(obj)) __result;                                                                                                                         \
    size_t __size = sizeof(*(obj));                                                                                                                  \
    switch (__size) {                                                                                                                                \
    case 4:                                                                                                                                          \
      asm volatile(OPERATION ".w" AQRL " %0, %2, %1\n" : "=&r"(__result), "+A"(*__obj) : "r"(__desired) : "memory");                                 \
      break;                                                                                                                                         \
    case 8:                                                                                                                                          \
      asm volatile(OPERATION ".d" AQRL " %0, %2, %1\n" : "=&r"(__result), "+A"(*__obj) : "r"(__desired) : "memory");                                 \
      break;                                                                                                                                         \
    }                                                                                                                                                \
    __result;                                                                                                                                        \
  })

#define __atomic_operation_explicit(OPERATION, obj, desired, order)                                                                                  \
  ({                                                                                                                                                 \
    typeof(*(obj)) __result;                                                                                                                         \
    switch (order) {                                                                                                                                 \
    case memory_order_seq_cst:                                                                                                                       \
    case memory_order_acq_rel:                                                                                                                       \
      __result = __atomic_operation(OPERATION, (obj), (desired), ".aqrl");                                                                           \
      break;                                                                                                                                         \
    case memory_order_acquire:                                                                                                                       \
    case memory_order_consume:                                                                                                                       \
      __result = __atomic_operation(OPERATION, (obj), (desired), ".aq");                                                                             \
      break;                                                                                                                                         \
    case memory_order_release:                                                                                                                       \
      __result = __atomic_operation(OPERATION, (obj), (desired), ".rl");                                                                             \
      break;                                                                                                                                         \
    default:                                                                                                                                         \
      __result = __atomic_operation(OPERATION, (obj), (desired), "");                                                                                \
      break;                                                                                                                                         \
    }                                                                                                                                                \
    __result;                                                                                                                                        \
  })

#define atomic_exchange(obj, desired) __atomic_operation("amoswap", (obj), (desired), ".aqrl")

#define atomic_exchange_explicit(obj, desired, order) __atomic_operation_explicit("amoswap", (obj), (desired), (order))

#define atomic_fetch_add(obj, desired) __atomic_operation("amoadd", (obj), (desired), ".aqrl")

#define atomic_fetch_add_explicit(obj, desired, order) __atomic_operation_explicit("amoadd", (obj), (desired), (order))

#define atomic_fetch_sub(obj, desired) __atomic_operation("amoadd", (obj), -(desired), ".aqrl")

#define atomic_fetch_sub_explicit(obj, desired, order) __atomic_operation_explicit("amoadd", (obj), -(desired), (order))

#define atomic_fetch_or(obj, desired) __atomic_operation("amoor", (obj), (desired), ".aqrl")

#define atomic_fetch_or_explicit(obj, desired, order) __atomic_operation_explicit("amoor", (obj), (desired), (order))

#define atomic_fetch_xor(obj, desired) __atomic_operation("amoxor", (obj), (desired), ".aqrl")

#define atomic_fetch_xor_explicit(obj, desired, order) __atomic_operation_explicit("amoxor", (obj), (desired), (order))

#define atomic_fetch_and(obj, desired) __atomic_operation("amoamd", (obj), (desired), ".aqrl")

#define atomic_fetch_and_explicit(obj, desired, order) __atomic_operation_explicit("amoamd", (obj), (desired), (order))

#define __atomic_compare_exchange(obj, expected, desired, AQ, RL)                                                                                    \
  ({                                                                                                                                                 \
    typeof(obj) __obj = (obj);                                                                                                                       \
    typeof(expected) __expected = (expected);                                                                                                        \
    typeof(desired) __desired = (desired);                                                                                                           \
    typeof(*(obj)) __result;                                                                                                                         \
    register unsigned int __rc;                                                                                                                      \
    size_t __size = sizeof(*(obj));                                                                                                                  \
    switch (__size) {                                                                                                                                \
    case 4:                                                                                                                                          \
      asm volatile("0: lr.w" AQ " %0, %2\n"                                                                                                          \
                   "   bne %0, %z3, 1f\n"                                                                                                            \
                   "   sc.w" RL " %1, %z4, %2\n"                                                                                                     \
                   "   bnez %1, 0b\n"                                                                                                                \
                   "   fence rw, rw\n"                                                                                                               \
                   "1:\n"                                                                                                                            \
                   : "=&r"(__result), "=&r"(__rc), "+A"(*__obj)                                                                                      \
                   : "rJ"(__expected), "rj"(__desired)                                                                                               \
                   : "memory");                                                                                                                      \
      break;                                                                                                                                         \
    case 8:                                                                                                                                          \
      asm volatile("0: lr.d" AQ " %0, %2\n"                                                                                                          \
                   "   bne %0, %z3, 1f\n"                                                                                                            \
                   "   sc.d" RL " %1, %z4, %2\n"                                                                                                     \
                   "   bnez %1, 0b\n"                                                                                                                \
                   "   fence rw, rw\n"                                                                                                               \
                   "1:\n"                                                                                                                            \
                   : "=&r"(__result), "=&r"(__rc), "+A"(*__obj)                                                                                      \
                   : "rJ"(__expected), "rj"(__desired)                                                                                               \
                   : "memory");                                                                                                                      \
      break;                                                                                                                                         \
    }                                                                                                                                                \
    __result;                                                                                                                                        \
  })

#define __atomic_compare_exchange_explicit(obj, expected, desired, succ, fail)                                                                       \
  ({                                                                                                                                                 \
    typeof(*(obj)) __result;                                                                                                                         \
    switch (succ) {                                                                                                                                  \
    case memory_order_seq_cst:                                                                                                                       \
      __result = __atomic_compare_exchange((obj), (expected), (desired), ".aqrl", ".rl");                                                            \
      break;                                                                                                                                         \
    case memory_order_acq_rel:                                                                                                                       \
      __result = __atomic_compare_exchange((obj), (expected), (desired), ".aq", ".rl");                                                              \
      break;                                                                                                                                         \
    case memory_order_acquire:                                                                                                                       \
    case memory_order_consume:                                                                                                                       \
      __result = __atomic_compare_exchange((obj), (expected), (desired), ".aq", "");                                                                 \
      break;                                                                                                                                         \
    case memory_order_release:                                                                                                                       \
      __result = __atomic_compare_exchange((obj), (expected), (desired), "", ".rl");                                                                 \
      break;                                                                                                                                         \
    default:                                                                                                                                         \
      __result = __atomic_compare_exchange((obj), (expected), (desired), "", "");                                                                    \
      break;                                                                                                                                         \
    }                                                                                                                                                \
    __result;                                                                                                                                        \
  })

#define atomic_compare_exchange_strong(obj, expected, desired)                                                                                       \
  __atomic_compare_exchange_explicit((obj), (expected), (desired), memory_order_seq_cst, memory_order_seq_cst)
#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail)                                                                  \
  __atomic_compare_exchange_explicit((obj), (expected), (desired), (succ), (fail))

#define atomic_compare_exchange_weak(obj, expected, desired)                                                                                         \
  __atomic_compare_exchange_explicit((obj), (expected), (desired), memory_order_seq_cst, memory_order_seq_cst)
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail)                                                                    \
  __atomic_compare_exchange_explicit((obj), (expected), (desired), (succ), (fail))

#endif /* ATOMIC_H */
