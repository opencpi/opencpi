#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.h"

namespace OA = OCPI::API;

int main(int argc, char **argv) {
  // Find a container to run our worker
  // (it returns a pointer since it might return NULL)
  OA::Container *c = OA::ContainerManager::find("rcc");
  assert(c); // we'll always find an rcc container

  // Create an application context on that container (and delete it when we exit)
  // (it returns a pointer since the caller is allowed to delete it)
  OA::ContainerApplication &a = *c->createApplication("hello-app");
  OA::Worker &w = a.createWorker("hello_instance", "hello");
  OA::Port &p = w.getPort("out");
  OA::ExternalPort &ep = p.connectExternal();
  w.start(); // start the worker running
  OA::ExternalBuffer *b;
  for (unsigned i = 0; i < 100; i++) {
    uint8_t opcode;
    uint32_t length;
    uint8_t *data;
    bool end;
    if ((b = ep.getBuffer(data, length, opcode, end))) {
      fprintf(stderr, "%s", (char *)data);
      return 0;
    }
  }
  fprintf(stderr, "Worker never sent anything!\n");
  return 1;
}
