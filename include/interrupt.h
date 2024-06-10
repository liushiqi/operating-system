/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
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

#ifndef INCLUDE_INTERRUPT_H_
#define INCLUDE_INTERRUPT_H_

#include "context.h"
#include <scheduler.h>

/* ERROR code */
enum IrqCode {
  INTERRUPT_USER_SOFT = 0,
  INTERRUPT_SUPER_SOFT = 1,
  INTERRUPT_MACH_SOFT = 3,
  INTERRUPT_USER_TIMER = 4,
  INTERRUPT_SUPER_TIMER = 5,
  INTERRUPT_MACH_TIMER = 7,
  INTERRUPT_USER_EXT = 8,
  INTERRUPT_SUPER_EXT = 9,
  INTERRUPT_MACH_EXT = 11,
  INTERRUPT_COUNT
};

enum ExcCode {
  EXCEPTION_INST_MISALIGNED = 0,
  EXCEPTION_INST_ACCESS = 1,
  EXCEPTION_BREAKPOINT = 3,
  EXCEPTION_LOAD_ACCESS = 5,
  EXCEPTION_STORE_ACCESS = 7,
  EXCEPTION_SYSCALL = 8,
  EXCEPTION_INST_PAGE_FAULT = 12,
  EXCEPTION_LOAD_PAGE_FAULT = 13,
  EXCEPTION_STORE_PAGE_FAULT = 15,
  EXCEPTION_COUNT
};

typedef void (*handler_t)(regs_context_t *, uint64_t, uint64_t);

extern void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause);

/* exception handler entry */
extern void exception_handler_entry(void);
extern void init_exception();
extern void setup_exception();

extern void reset_irq_timer();
extern void handle_int(regs_context_t *regs, uint64_t stval, uint64_t cause);
extern void handle_exception(regs_context_t *regs, uint64_t stval, uint64_t cause);
extern void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause);
extern void handle_syscall(regs_context_t *regs, uint64_t stval, uint64_t cause);
extern void handle_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause);

extern void enable_interrupt(void);
extern void disable_interrupt(void);
extern void enable_preempt(void);
extern void disable_preempt(void);

#endif
