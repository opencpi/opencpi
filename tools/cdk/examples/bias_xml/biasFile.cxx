#include <iostream>
#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.h"

namespace OA = OCPI::API;

int main(int argc, char **argv) {
  std::string hello =
    "<application done='file_write'>"
    "  <instance worker='file_read'>"
    "    <property name='filename' value='test.input'/>"
    "    <property name='granularity' value='4'/>"
    "    <property name='messageSize' value='16'/>"
    "  </instance>"
    "  <instance worker='bias' selection='";
  if (argv[1])
    hello += argv[1];
  hello +=
    "'>"
    "    <property name='biasValue' value='0x01020304'/>"
    "  </instance>"
    "  <instance worker='file_write'>"
    "    <property name='filename' value='test.outputwrong'/>"
    "  </instance>"
    "  <connection>"
    "    <port instance='file_read' name='out'/>"
    "    <port instance='bias' name='in'/>"
    "  </connection>"
    "  <connection>"
    "    <port instance='bias' name='out'/>"
    "    <port instance='file_write' name='in'/>"
    "  </connection>"
    "</application>";
  std::cerr<<hello;
  try {
    OA::Application app(hello);
    fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    fprintf(stderr, "Application established: containers, workers, connections all created\n");
    fprintf(stderr, "Communication with the application established\n");
    app.setProperty("file_write", "filename", "test.output");
    app.start();
    fprintf(stderr, "Application started/running\n");
    app.wait();
    fprintf(stderr, "Application finished\n");
    std::string name, value;
    for (unsigned n = 0; app.getProperty(n, name, value); n++)
      fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
    return 0;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
  return 1;
}
