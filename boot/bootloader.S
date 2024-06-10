#include <sbi.h>
#include <asm/asm.h>

.equ os_size_loc, 0x102001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x10201000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x10201000

.section .text.boot
ENTRY(_start)
  // fence on all memory and I/O
  fence

  // save FDT
  mv s0, a0
  mv s1, a1

  // output a \n
  li a0, 10
  li a7, SBI_CONSOLE_PUTCHAR
  ecall

  la a0, message
  li a7, SBI_CONSOLE_PUTSTR
  ecall

  li s2, kernel
  li s3, 1
  li s4, os_size_loc
  lh s4, (s4)
  add s4, s4, 1
  li s5, 32768
.loop:
  mv a0, s2
  li a1, 64
  mv a2, s3
  li a7, SBI_SD_READ
  ecall
  add s3, s3, 64
  add s2, s2, s5
  ble s3, s4, .loop

  // output a \n
  li a0, 10
  li a7, SBI_CONSOLE_PUTCHAR
  ecall

  // jump to main
  call kernel_main
stop:
  li a7, SBI_SHUTDOWN
  ecall
END(_start)

.data
message: .string "Bootloader boot beginning.\n\r"
