#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
int main(int argc, char **argv) {
  int fd;
  uint32_t val;
  off_t off;
  assert(argc == 3);
  errno = 0;
  off = strtoul(argv[1], NULL, 16);
  assert(errno == 0);
  assert((fd = open("/dev/mem", O_RDWR)) != -1);
  if (argv[2][0] == 'm') {
    uint32_t *p;
    printf("Accessing, via pointer indirection:");
    assert ((p = mmap(NULL, sizeof(val), PROT_READ, MAP_SHARED, fd, off)) != (uint32_t *)-1);
    val = *p;
  } else {
    printf("Accessing, via seek/read:");
    assert(lseek(fd, off, SEEK_SET) != -1);
    assert(read(fd, &val, sizeof(val)) == sizeof(val));
  }
  printf("value: 0x%x\n", val);
  return 0;
}
