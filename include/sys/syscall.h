#ifndef SYSCALL_H
#define SYSCALL_H

#define NUM_SYSCALL 64

/* define */
#define SYS_exec 0
#define SYS_exit 1
#define SYS_sleep 2
#define SYS_yield 3
#define SYS_kill 4
#define SYS_wait_pid 5

#define SYS_futex_wait 10
#define SYS_futex_wakeup 11

#define SYS_put_str 20
#define SYS_get_char 21
#define SYS_put_char 22
#define SYS_flush 23

#define SYS_get_timebase 30
#define SYS_get_tick 31

#define SYS_get_process_info 40
#define SYS_getpid 41
#define SYS_get_max_pid 42

#define SYS_open 50
#define SYS_create 51
#define SYS_getattr 52
#define SYS_read 53
#define SYS_write 54
#define SYS_close 55
#define SYS_change_dir 56
#define SYS_readdir 57
#define SYS_pwd 58

#endif
