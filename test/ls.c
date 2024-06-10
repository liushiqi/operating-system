#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

char pwd[1000];
char name[1000];

struct dirent entries[100];

int main(int argc, char **argv) {
  char *path;
  if (argc == 2)
    path = argv[1];
  else {
    path = pwd;
    sys_pwd(path);
  }
  int ret = sys_readdir(path, entries);
  if (ret == -1) {
    printf("%s\n", strerror(errno));
    sys_exit();
  }
  printf("inode   size    name\n");
  int i = 0;
  while (true) {
    if (entries[i].d_ino == 0) {
      printf("%lu %s\n", entries[i].d_ino, entries[i].d_name);
      break;
    }
    struct stat stat;
    strcpy(name, pwd);
    strcat(name, "/");
    strcat(name, entries[i].d_name);
    sys_getattr(name, &stat);
    printf("%7lu %7zd %s\n", stat.st_ino, stat.st_size, entries[i].d_name);
    i += 1;
  }
  return 0;
}
