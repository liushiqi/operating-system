#include <stdio.h>
#include <sys/syscall.h>

int main() {
  printf("\033[2J\033[0;0H");
  sys_flush();
  return 0;
}
