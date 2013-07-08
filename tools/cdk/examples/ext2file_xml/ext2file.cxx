#include <cstdio>
#include <cassert>
#include <cstring>
#include "OcpiApi.h"

namespace OA = OCPI::API;

int main(int argc, char **argv) {
  std::string hello("<application package='ocpi'>"
		    // instance name defaults to file_read since there is only one
		    "  <instance component='file_write' external='in'>"
		    "    <property name='filename' value='hello.file'/>"
		    "  </instance>"
		    "</application>");
  try {
    OA::Application app(hello);
    fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    fprintf(stderr, "Application established: containers, workers, connections all created\n");
    OA::ExternalPort &ep = app.getPort("in");
    fprintf(stderr, "Communication with the application established\n");
    app.start();
    fprintf(stderr, "Application started/running\n");
    for(unsigned n = 0; n < 5; n++) {
      OA::ExternalBuffer *b;
      uint8_t *data;
      size_t length;
      while (!(b = ep.getBuffer(data, length)))
	sleep(1);
      snprintf((char*)data, length, "Hello, World %u\n", n);
      printf("length: %zu\n", length);
      data[length-1] = 0;
      b->put(strlen((char*)data));
    }
    while (ep.tryFlush())
      sleep(1);
    return 0;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
  return 1;
}
