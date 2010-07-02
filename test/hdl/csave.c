#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#define CONFIG_BYTES 128
#define NWORDS (CONFIG_BYTES/sizeof(uint32_t))

int main(int argc, char **argv)
{
  char name[1000];
  uint32_t buf[NWORDS];
  int fd;


  if (argc != 4) {
    fprintf(stderr, "Usage: csave <bus> <device> <function>\n");
    return 1;
  }
  sprintf(name, "/sys/bus/pci/devices/0000:%02d:%02d.%d/config", atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
  assert((fd = open(name, O_RDONLY)) != -1);
  assert(read(fd, buf, CONFIG_BYTES) == CONFIG_BYTES);
  assert(write(1, buf, CONFIG_BYTES) == CONFIG_BYTES);
  return 0;
}
