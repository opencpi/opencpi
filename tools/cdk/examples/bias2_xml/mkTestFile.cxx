#include <stdint.h>
#include <unistd.h>

int main() {
  for (uint32_t n = 0; n < 100; n++)
    write(1, &n, sizeof(n));
  return 0;
}
