#ifndef KERNEL_SBI_H
#define KERNEL_SBI_H

#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8
#define SBI_CONSOLE_PUTSTR 9
#define SBI_SD_WRITE 10
#define SBI_SD_READ 11

#ifndef __ASSEMBLER__

#include <stdint.h>

#define sbi_call(which, arg0, arg1, arg2)                                                                                                            \
  ({                                                                                                                                                 \
    register uintptr_t a0 asm("a0") = (uintptr_t)(arg0);                                                                                             \
    register uintptr_t a1 asm("a1") = (uintptr_t)(arg1);                                                                                             \
    register uintptr_t a2 asm("a2") = (uintptr_t)(arg2);                                                                                             \
    register uintptr_t a7 asm("a7") = (uintptr_t)(which);                                                                                            \
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory");                                                                         \
    a0;                                                                                                                                              \
  })

static inline void sbi_console_puts(const char *str) { sbi_call(SBI_CONSOLE_PUTSTR, (uint64_t)str, 0, 0); }

static inline uintptr_t sbi_sd_write(unsigned int mem_address, unsigned int num_of_blocks, unsigned int block_id) {
  return sbi_call(SBI_SD_WRITE, mem_address, num_of_blocks, block_id);
}

static inline uintptr_t sbi_sd_read(unsigned int mem_address, unsigned int num_of_blocks, unsigned int block_id) {
  return sbi_call(SBI_SD_READ, mem_address, num_of_blocks, block_id);
}

static inline void sbi_console_putchar(int ch) { sbi_call(SBI_CONSOLE_PUTCHAR, ch, 0, 0); }

static inline uint64_t sbi_console_getchar(void) { return sbi_call(SBI_CONSOLE_GETCHAR, 0, 0, 0); }

static inline void sbi_set_timer(uint64_t stime_value) { sbi_call(SBI_SET_TIMER, stime_value, 0, 0); }

static inline void sbi_shutdown(void) { sbi_call(SBI_SHUTDOWN, 0, 0, 0); }

static inline void sbi_clear_ipi(void) { sbi_call(SBI_CLEAR_IPI, 0, 0, 0); }

static inline void sbi_send_ipi(const unsigned long *hart_mask) { sbi_call(SBI_SEND_IPI, (uint64_t)hart_mask, 0, 0); }

static inline void sbi_remote_fence_i(const unsigned long *hart_mask) { sbi_call(SBI_REMOTE_FENCE_I, (uint64_t)hart_mask, 0, 0); }

static inline void sbi_remote_sfence_vma(const unsigned long *hart_mask) { sbi_call(SBI_REMOTE_SFENCE_VMA, (uint64_t)hart_mask, 0, 0); }

static inline void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask) { sbi_call(SBI_REMOTE_SFENCE_VMA_ASID, (uint64_t)hart_mask, 0, 0); }

#endif

#endif