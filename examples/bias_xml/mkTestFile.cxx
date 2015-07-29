#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  unsigned count = argv[1] ? atoi(argv[1]) : 1000;
  for (uint32_t n = 0; n < count; n++) {
    write(1, &n, sizeof(n));
    uint32_t x = n + 0x01020304;
    write(2, &x, sizeof(n));
  }
  return 0;
}
