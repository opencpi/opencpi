#include <unistd.h>
#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.hh"

namespace OA = OCPI::API;

int main(int /* argc */, char ** /* argv */) {
  std::string hello("<application>"
		    // instance name defaults to hello since there is only one
		    "  <instance component='local.hello' selection='model==\"rcc\"'/>"
		    "  <connection>"
		    "    <external name='out'/>"
		    "    <port instance='hello' name='out'/>"
		    "  </connection>"
		    "</application>");
  try {
    OA::Application app(hello);
    fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    fprintf(stderr, "Application established: containers, workers, connections all created\n");
    OA::ExternalPort &ep = app.getPort("out");
    fprintf(stderr, "Communication with the application established\n");
    app.start();
    fprintf(stderr, "Application started/running\n");
    OA::ExternalBuffer *b;
    for (unsigned i = 0; i < 10; i++) {
      uint8_t *data;
      size_t length;
      uint8_t opcode;
      bool end;
      if ((b = ep.getBuffer(data, length, opcode, end))) {
	fprintf(stderr, "%s", (char *)data);
	return 0;
      }
      sleep(1);
    }
    fprintf(stderr,"Worker never sent anything!\n");
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
  return 1;
}
