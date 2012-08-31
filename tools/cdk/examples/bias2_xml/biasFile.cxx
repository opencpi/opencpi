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
    "  <instance worker='bias' selection='";
  if (argv[1] && argv[2])
    hello += argv[2];
  hello +=
    "'>"
    "    <property name='biasValue' value='0x01010101'/>"
    "  </instance>"
    "  <instance worker='file_write'>"
    "    <property name='filename' value='test.outputwrong'/>"
    "  </instance>"
    "  <connection>"
    "    <port instance='file_read' name='out'/>"
    "    <port instance='bias0' name='in'/>"
    "  </connection>"
    "  <connection>"
    "    <port instance='bias0' name='out'/>"
    "    <port instance='bias1' name='in'/>"
    "  </connection>"
    "  <connection>"
    "    <port instance='bias1' name='out'/>"
    "    <port instance='file_write' name='in'/>"
    "  </connection>"
    "</application>";

  try {
    OA::Application app(hello);
    fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    fprintf(stderr, "Application established: containers, workers, connections all created\n");
    fprintf(stderr, "Communication with the application established\n");
#if 1
    OA::Property p(app, "file_write:filename");
    p.setStringValue("test.output");
    OA::Property p1(app, "file_read:test");
    short s[] = {-1,-2, 0, -32768, 32767};
    p1.setShortSequenceValue(s, 4);
#else
    app.setProperty("file_write", "filename", "test.output");
#endif
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
