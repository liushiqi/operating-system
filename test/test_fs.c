#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#define O_RDWR 0

static char buff[64];

int main(void) {
  int i, j;
  int fd = sys_create("1.txt", O_RDWR);

  // write 'hello world!' * 10
  for (i = 0; i < 10; i++) {
    sys_write(fd, "hello world!\n", 13);
  }

  // read
  for (i = 0; i < 10; i++) {
    sys_read(fd, buff, 13);
    for (j = 0; j < 13; j++) {
      printf("%c", buff[j]);
    }
  }

  sys_close(fd);
}