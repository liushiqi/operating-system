#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

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

#ifndef __ASSEMBLER__

#include "scheduler.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

typedef unsigned long ino_t;
typedef int mode_t;
typedef int nlink_t;
typedef unsigned long uid_t;
typedef unsigned long gid_t;
typedef unsigned long off_t;

typedef struct dirent {
  ino_t d_ino;             /* Inode number */
  off_t d_off;             /* Not an offset; see below */
  unsigned short d_reclen; /* Length of this record */
  unsigned char d_type;    /* Type of file; not supported by all filesystem types */
  char d_name[256];        /* Null-terminated filename */
} dirent_t;

enum {
  DT_UNKNOWN = 0,
#define DT_UNKNOWN DT_UNKNOWN
  DT_FIFO = 1,
#define DT_FIFO DT_FIFO
  DT_CHR = 2,
#define DT_CHR DT_CHR
  DT_DIR = 4,
#define DT_DIR DT_DIR
  DT_BLK = 6,
#define DT_BLK DT_BLK
  DT_REG = 8,
#define DT_REG DT_REG
  DT_LNK = 10,
#define DT_LNK DT_LNK
  DT_SOCK = 12,
#define DT_SOCK DT_SOCK
  DT_WHT = 14
#define DT_WHT DT_WHT
};

struct stat {
  ino_t st_ino;     /* File serial number.	*/
  mode_t st_mode;   /* File mode.  */
  nlink_t st_nlink; /* Link count.  */
  uid_t st_uid;     /* User ID of the file's owner.	*/
  gid_t st_gid;     /* Group ID of the file's group.*/
  size_t st_size;   /* Size of file, in bytes.  */
  long st_atime;    /* Time of last access.  */
  long st_mtime;    /* Time of last modification.  */
  long st_ctime;    /* Time of last status change.  */
};

extern long invoke_syscall(long, long, long, long);

pid_t sys_exec(char *name, int argc, char **argv);

void sys_exit(void);

void sys_sleep(uint32_t time);

void sys_yield(void);

int sys_kill(pid_t pid);

int sys_wait_pid(pid_t pid);

void sys_futex_wait(atomic_long *val_addr, uint64_t val);

void sys_futex_wakeup(atomic_long *val_addr, int num_wakeup);

void sys_put_str(const char *buff);

int sys_get_char(void);

void sys_put_char(int c);

void sys_flush(void);

long sys_get_timebase(void);

long sys_get_tick(void);

int sys_get_process_info(int pid, process_status_t *status);

pid_t sys_getpid(void);

int sys_get_max_pid(void);

int sys_open(char *path, int mode);

int sys_create(char *path, int mode);

int sys_getattr(char *path, struct stat *mode);

int sys_read(int fd, char *buf, int size);

int sys_write(int fd, char *buf, int size);

int sys_close(int fd);

int sys_change_dir(char *path);

struct dirent *sys_readdir(char *path, struct dirent *buf);

void sys_pwd(char *path);

#endif

#endif
