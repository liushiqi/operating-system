#include <errno.h>
#include <futex.h>
#include <interrupt.h>
#include <naivefs.h>
#include <sbi.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>

#define __SYSCALL(name) [SYS_##name] = syscall_##name

long syscall_exec(uint64_t, uint64_t, uint64_t);
long syscall_exit(uint64_t, uint64_t, uint64_t);
long syscall_sleep(uint64_t, uint64_t, uint64_t);
long syscall_yield(uint64_t, uint64_t, uint64_t);
long syscall_kill(uint64_t, uint64_t, uint64_t);
long syscall_wait_pid(uint64_t, uint64_t, uint64_t);

long syscall_futex_wait(uint64_t, uint64_t, uint64_t);
long syscall_futex_wakeup(uint64_t, uint64_t, uint64_t);

long syscall_put_str(uint64_t, uint64_t, uint64_t);
long syscall_get_char(uint64_t, uint64_t, uint64_t);
long syscall_put_char(uint64_t, uint64_t, uint64_t);
long syscall_flush(uint64_t, uint64_t, uint64_t);

long syscall_get_timebase(uint64_t, uint64_t, uint64_t);
long syscall_get_tick(uint64_t, uint64_t, uint64_t);

long syscall_get_process_info(uint64_t, uint64_t, uint64_t);
long syscall_getpid(uint64_t, uint64_t, uint64_t);
long syscall_get_max_pid(uint64_t, uint64_t, uint64_t);

long syscall_open(uint64_t, uint64_t, uint64_t);
long syscall_create(uint64_t, uint64_t, uint64_t);
long syscall_getattr(uint64_t, uint64_t, uint64_t);
long syscall_read(uint64_t, uint64_t, uint64_t);
long syscall_write(uint64_t, uint64_t, uint64_t);
long syscall_close(uint64_t, uint64_t, uint64_t);
long syscall_change_dir(uint64_t, uint64_t, uint64_t);
long syscall_readdir(uint64_t, uint64_t, uint64_t);
long syscall_pwd(uint64_t, uint64_t, uint64_t);

long (*syscall[NUM_SYSCALL])() = {
    __SYSCALL(exec),     __SYSCALL(exit),        __SYSCALL(sleep),        __SYSCALL(yield),      __SYSCALL(kill),
    __SYSCALL(wait_pid), __SYSCALL(futex_wait),  __SYSCALL(futex_wakeup), __SYSCALL(put_str),    __SYSCALL(get_char),
    __SYSCALL(put_char), __SYSCALL(flush),       __SYSCALL(get_timebase), __SYSCALL(get_tick),   __SYSCALL(get_process_info),
    __SYSCALL(getpid),   __SYSCALL(get_max_pid), __SYSCALL(open),         __SYSCALL(create),     __SYSCALL(getattr),
    __SYSCALL(read),     __SYSCALL(write),       __SYSCALL(close),        __SYSCALL(change_dir), __SYSCALL(readdir),
    __SYSCALL(pwd)};

void handle_syscall(regs_context_t *regs, __attribute__((unused)) uint64_t interrupt, __attribute__((unused)) uint64_t cause) {
  regs->sepc = regs->sepc + 4;
  regs->regs[10] = syscall[regs->regs[17]](regs->regs[10], regs->regs[11], regs->regs[12]);
}

long syscall_exec(uint64_t name, uint64_t argc, uint64_t argv) { return exec((char *)name, argc, (char **)argv); }

long syscall_exit(__attribute__((unused)) uint64_t __unused_0, __attribute__((unused)) uint64_t __unused_1,
                  __attribute__((unused)) uint64_t __unused_2) {
  exit();
  return 0;
}

long syscall_sleep(uint64_t time, __attribute__((unused)) uint64_t __unused_1, __attribute__((unused)) uint64_t __unused_2) {
  sleep(time);
  return 0;
}

long syscall_yield(__attribute__((unused)) uint64_t __unused_0, __attribute__((unused)) uint64_t __unused_1,
                   __attribute__((unused)) uint64_t __unused_2) {
  scheduler();
  return 0;
}

long syscall_kill(uint64_t pid, __attribute__((unused)) uint64_t __unused_1, __attribute__((unused)) uint64_t __unused_2) {
  kill(pid);
  return 0;
}

long syscall_wait_pid(uint64_t pid, __attribute__((unused)) uint64_t __unused_1, __attribute__((unused)) uint64_t __unused_2) {
  return wait_pid(pid);
}

long syscall_futex_wait(uint64_t ptr, uint64_t val, __attribute__((unused)) uint64_t __unused_2) {
  futex_wait((volatile uint64_t *)ptr, val);
  return 0;
}

long syscall_futex_wakeup(uint64_t ptr, uint64_t val, __attribute__((unused)) uint64_t __unused_2) {
  futex_wakeup((volatile uint64_t *)ptr, val);
  return 0;
}

long syscall_put_str(uint64_t str, __attribute__((unused)) uint64_t __unused_1, __attribute__((unused)) uint64_t __unused_2) {
  disable_preempt();
  char *string = (char *)str;
  int len = strlen(string);
  for (int i = 0; i < len; ++i) {
    screen_put_char(string[i]);
  }
  enable_preempt();
  return 0;
}

long syscall_get_char(__attribute__((unused)) uint64_t __unused_0, __attribute__((unused)) uint64_t __unused_1,
                      __attribute__((unused)) uint64_t __unused_2) {
  return sbi_console_getchar();
}

long syscall_put_char(uint64_t c, __attribute__((unused)) uint64_t __unused_1, __attribute__((unused)) uint64_t __unused_2) {
  screen_put_char((char)c);
  return 0;
}

long syscall_flush(__attribute__((unused)) uint64_t __unused_0, __attribute__((unused)) uint64_t __unused_1,
                   __attribute__((unused)) uint64_t __unused_2) {
  disable_preempt();
  screen_flush();
  enable_preempt();
  return 0;
}

long syscall_get_timebase(__attribute__((unused)) uint64_t __unused_0, __attribute__((unused)) uint64_t __unused_1,
                          __attribute__((unused)) uint64_t __unused_2) {
  return get_time_base();
}

long syscall_get_tick(__attribute__((unused)) uint64_t __unused_0, __attribute__((unused)) uint64_t __unused_1,
                      __attribute__((unused)) uint64_t __unused_2) {
  return get_ticks();
}

long syscall_get_process_info(uint64_t pid, uint64_t info, __attribute__((unused)) uint64_t __unused_2) {
  return get_process_info(pid, (process_status_t *)info);
}

long syscall_getpid(__attribute__((unused)) uint64_t __unused_0, __attribute__((unused)) uint64_t __unused_1,
                    __attribute__((unused)) uint64_t __unused_2) {
  return current_running->pid;
}

long syscall_get_max_pid(__attribute__((unused)) uint64_t __unused_0, __attribute__((unused)) uint64_t __unused_1,
                         __attribute__((unused)) uint64_t __unused_2) {
  return current_pid;
}

long syscall_open(uint64_t __path, uint64_t mode, __attribute__((unused)) uint64_t __unused_2) {
  char *path = (char *)pa_to_ka(va_to_pa(__path, (const page_table_entry_t *)current_running->page_directory));
  int fd = -EMFILE;
  for (int i = 0; i < 64; ++i) {
    if (current_running->files[i].valid == false) {
      struct stat buf;
      if ((path)[0] == '/') {
        int ret = naive_getattr((path), &buf);
        if (ret < 0)
          return ret;
        strcpy(current_running->files[i].path, path);
      } else {
        char *full_name = (char *)alloc_page(1);
        strcpy(full_name, current_running->pwd);
        strcat(full_name, path);
        int ret = naive_getattr(full_name, &buf);
        if (ret < 0) {
          free_page((uintptr_t)full_name);
          return ret;
        }
        free_page((uintptr_t)full_name);
        strcpy(current_running->files[i].path, full_name);
      }
      current_running->files[i].valid = true;
      current_running->files[i].inode = buf.st_ino;
      current_running->files[i].read_position = 0;
      current_running->files[i].write_position = 0;
      fd = i;
      break;
    }
  }
  return fd;
}

long syscall_create(uint64_t __path, uint64_t mode, __attribute__((unused)) uint64_t __unused_2) {
  char *path = (char *)pa_to_ka(va_to_pa(__path, (const page_table_entry_t *)current_running->page_directory));
  int fd = -EMFILE;
  for (int i = 0; i < 64; ++i) {
    if (current_running->files[i].valid == false) {
      struct stat buf;
      if ((path)[0] == '/') {
        int ret = naive_mknod(path, mode);
        if (ret < 0)
          return ret;
        strcpy(current_running->files[i].path, path);
        naive_getattr(path, &buf);
      } else {
        char *full_name = (char *)alloc_page(1);
        strcpy(full_name, current_running->pwd);
        strcat(full_name, path);
        int ret = naive_mknod(full_name, mode);
        if (ret < 0) {
          free_page((uintptr_t)full_name);
          return ret;
        }
        free_page((uintptr_t)full_name);
        strcpy(current_running->files[i].path, full_name);
        naive_getattr(full_name, &buf);
      }
      current_running->files[i].valid = true;
      current_running->files[i].inode = buf.st_ino;
      current_running->files[i].read_position = 0;
      current_running->files[i].write_position = 0;
      fd = i;
      break;
    }
  }
  return fd;
}

long syscall_getattr(uint64_t __path, uint64_t buf, __attribute__((unused)) uint64_t __unused_2) {
  char *path = (char *)pa_to_ka(va_to_pa(__path, (const page_table_entry_t *)current_running->page_directory));
  if ((path)[0] == '/') {
    int ret = naive_getattr(path, (struct stat *)pa_to_ka(va_to_pa((uintptr_t)buf, (const page_table_entry_t *)current_running->page_directory)));
    if (ret < 0)
      return ret;
    return 0;
  } else {
    char *full_name = (char *)alloc_page(1);
    strcpy(full_name, current_running->pwd);
    strcat(full_name, path);
    int ret =
        naive_getattr(full_name, (struct stat *)pa_to_ka(va_to_pa((uintptr_t)buf, (const page_table_entry_t *)current_running->page_directory)));
    free_page((uintptr_t)full_name);
    if (ret < 0) {
      free_page((uintptr_t)full_name);
      return ret;
    }
    return 0;
  }
}

long syscall_read(uint64_t fd, uint64_t buf, uint64_t size) {
  if (current_running->files[fd].valid == false) {
    return -EBADF;
  }
  int count = naive_read(current_running->files[fd].path,
                         (char *)pa_to_ka(va_to_pa((uintptr_t)buf, (const page_table_entry_t *)current_running->page_directory)), size,
                         current_running->files[fd].read_position);
  if (count < 0)
    return count;
  current_running->files[fd].read_position += count;
  return count;
}

long syscall_write(uint64_t fd, uint64_t buf, uint64_t size) {
  if (current_running->files[fd].valid == false) {
    return -EBADF;
  }
  naive_write(current_running->files[fd].path,
              (char *)pa_to_ka(va_to_pa((uintptr_t)buf, (const page_table_entry_t *)current_running->page_directory)), size,
              current_running->files[fd].write_position);
  current_running->files[fd].write_position += size;
  return 0;
}

long syscall_close(uint64_t fd, __attribute__((unused)) uint64_t __unused_1, __attribute__((unused)) uint64_t __unused_2) {
  if (current_running->files[fd].valid == false) {
    return -EBADF;
  } else
    current_running->files[fd].valid = false;
}

long syscall_change_dir(uint64_t __path, __attribute__((unused)) uint64_t __unused_1, __attribute__((unused)) uint64_t __unused_2) {
  char *_path = (char *)pa_to_ka(va_to_pa(__path, (const page_table_entry_t *)current_running->page_directory));
  char *path = kernel_malloc(strlen(path) + 1);
  strcpy(path, _path);
  char new_path[512] = {0};
  strcpy(new_path, current_running->pwd);
  char *name = strtok(path, "/");
  while (name != NULL) {
    if (strcmp(name, ".") == 0) {
      name = strtok(NULL, "/");
      continue;
    } else if (strcmp(name, "..") == 0) {
      char *s = strrchr(new_path, '/');
      if (s != new_path)
        *s = 0;
      continue;
    } else {
      strcat(new_path, "/");
      strcat(new_path, name);
      continue;
    }
  }
  int ret = naive_getattr(new_path, NULL);
  if (ret < 0) {
    return ret;
  } else {
    strcpy(current_running->pwd, new_path);
    return 0;
  }
}

long syscall_readdir(uint64_t __path, uint64_t __buf, __attribute__((unused)) uint64_t __unused_2) {
  char *path = (char *)pa_to_ka(va_to_pa(__path, (const page_table_entry_t *)current_running->page_directory));
  void *buf = (void *)pa_to_ka(va_to_pa(__buf, (const page_table_entry_t *)current_running->page_directory));
  return naive_readdir(path, buf);
}

long syscall_pwd(uint64_t __path, __attribute__((unused)) uint64_t __unused_1, __attribute__((unused)) uint64_t __unused_2) {
  char *path = (char *)pa_to_ka(va_to_pa(__path, (const page_table_entry_t *)current_running->page_directory));
  strcpy(path, current_running->pwd);
}