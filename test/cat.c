#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

char buffer[1024];

int main(int argc, char **argv) {
  char *path = NULL;
  if (argc == 2)
    path = argv[1];
  else {
    printf("Invalid argument\n");
    sys_exit();
  }
  int fd = sys_open(path, 0);
  if (fd == -1) {
    printf("%s\n", strerror(errno));
  }
  while (true) {
    int ret = sys_read(fd, buffer, 1024);
    if (ret == -1) {
      printf("%s\n", strerror(errno));
      break;
    } else if (ret == 0) {
      break;
    }
    for (int i = 0; i < ret; ++i) {
      sys_put_char(buffer[i]);
    }
  }
  return 0;
}
