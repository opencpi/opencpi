#include <iostream>
#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.h"


namespace OA = OCPI::API;

int main(int argc, char **argv) {
  const char *size = "100", *selection = "", *nbuffers="2";
  if (argv[1]) {
    selection = argv[1];
    if (argv[2]) {
      size = argv[2];
      if (argv[3])
      nbuffers = argv[3];
    }
  }
  std::string hello =
    "<application done='cilkmatrixsink_cc' package='ocpi'>"
    "  <instance component='cilkmatrixsrc_cc'>"
    "  </instance>"
    "  <instance component='cilkmatrix_cc' selection='";
  hello += selection;
  hello +=
    "'>"
    "    <property name='biasValue' value='0x01020304'/>"
    "  </instance>"
    "  <instance component='cilkmatrixsink_cc'>"
    "  </instance>"
    "  <connection>"
    "    <port instance='cilkmatrixsrc_cc' name='out' bufferCount='2'/>"
    "    <port instance='cilkmatrix_cc' name='in' bufferCount='";
  hello += nbuffers;
  hello +=
    "'/>"
    "  </connection>"
    "  <connection>"
    "    <port instance='cilkmatrix_cc' name='out' bufferCount='";
  hello += nbuffers;
  hello +=
    "'/>"
    "    <port instance='cilkmatrixsink_cc' name='in' bufferCount='1'/>"
    "  </connection>"
    "</application>";
  try {
    OA::Application app(hello);
    fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    fprintf(stderr, "Application established: containers, workers, connections all created\n");
    fprintf(stderr, "Communication with the application established\n");

    /*
#if 0 // original simple way
    app.setProperty("cilkmatrixsink_cc", "filename", "test.output");
#else
    OA::Property p(app, "file_write.filename");
    p.setStringValue("test.output");
#endif
    */

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
