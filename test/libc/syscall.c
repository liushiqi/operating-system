#include <stdint.h>
#include <sys/syscall.h>

int errno;

pid_t sys_exec(char *name, int argc, char **argv) { return invoke_syscall(SYS_exec, (uintptr_t)name, (uintptr_t)argc, (uint64_t)argv); }

void sys_exit(void) { invoke_syscall(SYS_exit, 0, 0, 0); }

void sys_sleep(uint32_t time) { invoke_syscall(SYS_sleep, time, 0, 0); }

void sys_yield(void) { invoke_syscall(SYS_yield, 0, 0, 0); }

int sys_kill(pid_t pid) { return (int)invoke_syscall(SYS_kill, pid, 0, 0); }

int sys_wait_pid(pid_t pid) { return (int)invoke_syscall(SYS_wait_pid, pid, 0, 0); }

void sys_futex_wait(atomic_long *val_addr, uint64_t val) { invoke_syscall(SYS_futex_wait, (uintptr_t)val_addr, val, 0); }

void sys_futex_wakeup(atomic_long *val_addr, int num_wakeup) { invoke_syscall(SYS_futex_wakeup, (uintptr_t)val_addr, num_wakeup, 0); }

void sys_put_str(const char *buff) { invoke_syscall(SYS_put_str, (uintptr_t)buff, 0, 0); }

int sys_get_char() { return (int)invoke_syscall(SYS_get_char, 0, 0, 0); }

void sys_put_char(int c) { invoke_syscall(SYS_put_char, c, 0, 0); }

void sys_flush() { invoke_syscall(SYS_flush, 0, 0, 0); }

long sys_get_timebase(void) { return invoke_syscall(SYS_get_timebase, 0, 0, 0); }

long sys_get_tick(void) { return invoke_syscall(SYS_get_tick, 0, 0, 0); }

int sys_get_process_info(int pid, process_status_t *status) { return (int)invoke_syscall(SYS_get_process_info, pid, (uintptr_t)status, 0); }

pid_t sys_getpid(void) { return invoke_syscall(SYS_getpid, 0, 0, 0); }

int sys_get_max_pid(void) { return (int)invoke_syscall(SYS_get_max_pid, 0, 0, 0); }

int sys_open(char *path, int mode) {
  int ret = (int)invoke_syscall(SYS_open, (long)path, mode, 0);
  if (ret < 0) {
    errno = -ret;
    return -1;
  } else {
    return ret;
  }
}

int sys_create(char *path, int mode) {
  int ret = (int)invoke_syscall(SYS_create, (long)path, mode, 0);
  if (ret < 0) {
    errno = -ret;
    return -1;
  } else {
    return ret;
  }
}

int sys_getattr(char *path, struct stat *attr) {
  int ret = (int)invoke_syscall(SYS_getattr, (long)path, (long)attr, 0);
  if (ret < 0) {
    errno = -ret;
    return -1;
  } else {
    return ret;
  }
}

int sys_read(int fd, char *buf, int size) {
  int ret = (int)invoke_syscall(SYS_read, fd, (long)buf, size);
  if (ret < 0) {
    errno = -ret;
    return -1;
  } else {
    return ret;
  }
}

int sys_write(int fd, char *buf, int size) {
  int ret = (int)invoke_syscall(SYS_write, fd, (long)buf, size);
  if (ret < 0) {
    errno = -ret;
    return -1;
  } else {
    return ret;
  }
}

int sys_close(int fd) {
  int ret = (int)invoke_syscall(SYS_close, fd, 0, 0);
  if (ret < 0) {
    errno = -ret;
    return -1;
  } else {
    return ret;
  }
}

int sys_change_dir(char *path) {
  int ret = (int)invoke_syscall(SYS_change_dir, (long)path, 0, 0);
  if (ret < 0) {
    errno = -ret;
    return -1;
  } else {
    return ret;
  }
}

struct dirent *sys_readdir(char *path, struct dirent *buf) {
  long ret = invoke_syscall(SYS_readdir, (long)path, (long)buf, 0);
  if (ret < 0) {
    errno = -(int)ret;
    return -1;
  } else {
    return 0;
  }
}

void sys_pwd(char *path) { invoke_syscall(SYS_pwd, (long)path, 0, 0); }
