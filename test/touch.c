#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

int main(int argc, char **argv) {
  char *path = NULL;
  if (argc == 2)
    path = argv[1];
  else {
    printf("Invalid argument\n");
    sys_exit();
  }
  int ret = sys_create(path, 0755);
  if (ret == -1) {
    printf("%s\n", strerror(errno));
  }
  return 0;
}
