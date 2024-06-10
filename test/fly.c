#include <stdio.h>
#include <sys/syscall.h>

static char blank[] = {"                   "};
static char plane1[] = {"    ___         _  "};
static char plane2[] = {"| __\\_\\______/_| "};
static char plane3[] = {"<[___\\_\\_______| "};
static char plane4[] = {"|  o'o             "};

int main() {
  int i = 22, j = 10;

  for (int k = 0; k < 100; ++k) {
    for (i = 60; i > 0; i--) {
      /* move */
      printf("\033[%d;%dH%s\r", j + 0, i, plane1);

      printf("\033[%d;%dH%s\r", j + 1, i, plane2);

      printf("\033[%d;%dH%s\r", j + 2, i, plane3);

      printf("\033[%d;%dH%s\r", j + 3, i, plane4);
    }

    printf("\033[%d;%dH%s\r", j + 0, 1, blank);

    printf("\033[%d;%dH%s\r", j + 1, 1, blank);

    printf("\033[%d;%dH%s\r", j + 2, 1, blank);

    printf("\033[%d;%dH%s\r", j + 3, 1, blank);
  }
  return 0;
}
