#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.h"

namespace OA = OCPI::API;

int main(int argc, char **argv) {
  try {
  // Find a container to run our worker
  // (it returns a pointer since it might return NULL)
  OA::Container *c = OA::ContainerManager::find("rcc");
  assert(c); // we'll always find an rcc container

  // Create an application context on that container (and delete it when we exit)
  // (it returns a pointer since the caller is allowed to delete it)
  OA::ContainerApplication &a = *c->createApplication("hello-app");
  // Create a worker of type "hello", whose name in this app will be "hello_instance".
  OA::Worker &w = a.createWorker("hello_instance", "hello");
  // Get a handle (reference) on the "out" port of the worker
  OA::Port &p = w.getPort("out");
  // Get an external port (that this program can use) connected to the "out" port.
  OA::ExternalPort &ep = p.connectExternal();
  w.start(); // start the worker running
  OA::ExternalBuffer *b;
  for (unsigned i = 0; i < 100; i++) {
    uint8_t *data;
    uint32_t length;
    uint8_t opcode;
    bool end;
    if ((b = ep.getBuffer(data, length, opcode, end))) {
      fprintf(stderr, "%s", (char *)data);
      return 0;
    } else
      usleep(10000); // we have nothing useful to do, might as well give the CPU to components.
  }
  fprintf(stderr, "Worker never sent anything!\n");
  // Note that the ContainerApplication object MAY be deleted by the program,
  // hence it is a pointer.  But it doesn't HAVE to be deleted since it will
  // automatically be deleted on exit.
  return 1;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
}
