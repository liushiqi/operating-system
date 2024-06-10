#include <alloca.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#define SHELL_BUFF_SIZE 128
#define MAX_ARGS 256

const char prompt[] = {"root@liu-riscv:"};

char input[SHELL_BUFF_SIZE];
char *argv[MAX_ARGS];
char pwd[1000];

int main() {
  memset(input, 0, sizeof(input));
  memcpy(input, prompt, 17);
  printf("\033[2J\033[0;0H");
  sys_flush();
  while (true) {
    sys_pwd(pwd);
    printf("%s%s$ ", prompt, pwd);
    sys_flush();
    memset(input, 0, sizeof(input));
    gets(input, SHELL_BUFF_SIZE);
    char *ptr = strtok(input, " ");
    int argc = 0;
    while (ptr != NULL) {
      argv[argc] = ptr;
      ptr = strtok(NULL, " ");
      argc += 1;
    }
    if (strcmp(input, "cd") == 0) {
      sys_change_dir(argv[1]);
    } else {
      pid_t pid = sys_exec(input, argc, argv);
      if (pid == -1) {
        printf("executable %s not found.\n", input);
      }
      sys_wait_pid(pid);
    }
  }
}
