#include <stdio.h>
#include <sys/syscall.h>

int puts(const char *str) { return printf("%s", str); }

int gets(char *line, size_t n) {
  size_t length = 0;
  while (1) {
    int c;
    while ((c = sys_get_char()) == -1)
      ;
    if (c == 8 || c == 127) {
      if (length != 0) {
        length -= 1;
        line[length] = '\0';
        puts("\b \b");
      }
    } else if (c == '\n' || c == '\r') {
      sys_put_char('\n');
      break;
    } else {
      if (length < n) {
        line[length] = (char)c;
        length += 1;
        sys_put_char((char)c);
      }
    }
    sys_flush();
  }
  return length;
}
