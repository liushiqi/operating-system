#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
  srand(clock());
  long mem2 = 0;
  uintptr_t mem1 = 0;
  int i = 0;
  for (i = 1; i < argc; i++) {
    mem1 = atol(argv[i]);
    mem2 = rand();
    *(long *)mem1 = mem2;
    printf("0x%lx, %ld\n", mem1, mem2);
    if (*(long *)mem1 != mem2) {
      printf("Error!\n");
    }
  }
  // Only input address.
  // Achieving input r/w command is recommended but not required.
  printf("Success!\n");
  return 0;
}
