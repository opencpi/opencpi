#include <unistd.h>
#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.h"

namespace OA = OCPI::API;

int
main(int argc, char **argv) {
  try {
    OA::Application app("<application>"
			"  <instance component='copy' externals='true'/>"
			"</application>");
    app.initialize();
    OA::ExternalPort
      &pFromMe = app.getPort("in"),
      &pToMe = app.getPort("out");
    app.start();
    size_t length;
    do {
      OA::ExternalBuffer *bFromMe, *bToMe;
      uint8_t *data;
      uint8_t opcode;
      bool end;
      // Get a buffer (that is empty) to put data into that is from me
      while (!(bFromMe = pFromMe.getBuffer(data, length)))
	usleep(1000);
      // Read data from standard input into our output buffer to the worker
      ssize_t n = read(0, data, length);
      assert(n >= 0);
      // Send our output buffer on its way - as input to the worker/app,
      // giving it back to the system.
      bFromMe->put(n, 23, n == 0);
      // Get a buffer (that is full) to get data that is from the worker/app
      while (!(bToMe = pToMe.getBuffer(data, length, opcode, end)))
	usleep(1000);
      if (length) {
	n = write(1, data, length);
	assert(n == (ssize_t)length);
      }
      // Give the buffer back to the system, I'm done with it.
      bToMe->release();
    } while (length);
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return 1;
  }
  return 0;
}
