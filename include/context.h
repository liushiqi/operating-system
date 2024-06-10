/**
 * @headerfile context.h include/context.h
 * The definitions of the structure used when exception handling and doing switch_to call.
 */
#ifndef CONTEXT_H
#define CONTEXT_H

#define OFFSET_REG_ZERO 0

/* return address */
#define OFFSET_REG_RA 8

/* pointers */
#define OFFSET_REG_SP 16 /**< stack pointer */
#define OFFSET_REG_GP 24 /**< global pointer */
#define OFFSET_REG_TP 32 /**< thread pointer */

/* temporary */
#define OFFSET_REG_T0 40
#define OFFSET_REG_T1 48
#define OFFSET_REG_T2 56

/* saved register */
#define OFFSET_REG_S0 64
#define OFFSET_REG_S1 72

/* args */
#define OFFSET_REG_A0 80
#define OFFSET_REG_A1 88
#define OFFSET_REG_A2 96
#define OFFSET_REG_A3 104
#define OFFSET_REG_A4 112
#define OFFSET_REG_A5 120
#define OFFSET_REG_A6 128
#define OFFSET_REG_A7 136

/* saved register */
#define OFFSET_REG_S2 144
#define OFFSET_REG_S3 152
#define OFFSET_REG_S4 160
#define OFFSET_REG_S5 168
#define OFFSET_REG_S6 176
#define OFFSET_REG_S7 184
#define OFFSET_REG_S8 192
#define OFFSET_REG_S9 200
#define OFFSET_REG_S10 208
#define OFFSET_REG_S11 216

/* temporary register */
#define OFFSET_REG_T3 224
#define OFFSET_REG_T4 232
#define OFFSET_REG_T5 240
#define OFFSET_REG_T6 248

/* privileged register */
#define OFFSET_REG_SSTATUS 256
#define OFFSET_REG_SEPC 264
#define OFFSET_REG_SBADADDR 272
#define OFFSET_REG_SCAUSE 280

/* Size of stack frame, word/double word alignment */
#define OFFSET_SIZE 288

#define PCB_KERNEL_SP 0
#define PCB_USER_SP 8
#define PCB_PREEMPT_COUNT 16

/* offset in switch_to */
#define SWITCH_TO_RA 0
#define SWITCH_TO_SP 8
#define SWITCH_TO_S0 16
#define SWITCH_TO_S1 24
#define SWITCH_TO_S2 32
#define SWITCH_TO_S3 40
#define SWITCH_TO_S4 48
#define SWITCH_TO_S5 56
#define SWITCH_TO_S6 64
#define SWITCH_TO_S7 72
#define SWITCH_TO_S8 80
#define SWITCH_TO_S9 88
#define SWITCH_TO_S10 96
#define SWITCH_TO_S11 104

#define SWITCH_TO_SIZE 112

#ifndef __ASSEMBLER__

#include <sys/type.h>

/**
 * @brief The structure to save the context when exception handling.
 * The structure to save the context when exception handling.
 * Should not be changed because it's also being defined in {@link context_regs.h}
 */
typedef struct {
  register_t regs[32]; /**< general purpose registers. */

  register_t sstatus; /**< csr register sstatus */
  register_t sepc;    /**< csr register sepc */
  register_t stval;   /**< csr register stval */
  register_t scause;  /**< csr register scause */
} regs_context_t;

/**
 * @brief The structure to save the context when doing switch_to.
 * The structure to save the context when doing switch_to.
 * Should not be changed because it's also being defined in {@link context_regs.h}
 */
typedef struct {
  register_t ra;
  register_t sp;
  register_t regs[12];
} switch_to_context_t;

#endif

#endif