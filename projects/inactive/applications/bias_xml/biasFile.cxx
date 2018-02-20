/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cinttypes>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.hh"

namespace OA = OCPI::API;

int main(int /*argc*/, char **argv) {
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
    "<application done='file_write' package='ocpi.core'>"
    "  <property name='top_bv' instance='bias' property='biasValue' dumpfile='sss'/>"
    "  <instance component='file_read'>"
    "    <property name='filename' value='test.input'/>"
    "    <property name='granularity' value='4'/>"
    "    <property name='messageSize' value='";
  hello += size;
  hello +=
    "'/>"
    "  </instance>"
    "  <instance component='bias' worker='bias_cc' selection='";
  hello += selection;
  hello +=
    "'>"
    "    <property name='biasValue' value='0x01020304'/>"
    "  </instance>"
    "  <instance component='file_write'>"
    "    <property name='filename' value='test.outputwrong'/>"
    "  </instance>"
    "  <connection>"
    "    <port instance='file_read' name='out' bufferCount='1'/>"
    "    <port instance='bias' name='in' bufferCount='";
  hello += nbuffers;
  hello +=
    "'/>"
    "  </connection>"
    "  <connection>"
    "    <port instance='bias' name='out' bufferCount='";
  hello += nbuffers;
  hello +=
    "'/>"
    "    <port instance='file_write' name='in' bufferCount='1'/>"
    "  </connection>"
    "</application>";
  try {
    OA::Application app(hello);
    fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    fprintf(stderr, "Application established: containers, workers, connections all created\n");
    fprintf(stderr, "Communication with the application established\n");
    app.setProperty("file_write", "filename", "test.output");
    OA::Property p(app, "bias.testws");
    p.setULongValue(12);
    OA::Property p1(app, "bias", "biasValue");
    fprintf(stderr, "Reading property with split args: biasValue is %" PRIu32 "\n",
	    p1.getULongValue());
    OA::Property p2(app, "bias.biasValue");
    fprintf(stderr, "Reading property with dotted args: biasValue is %" PRIu32 "\n",
	    p2.getULongValue());
    OA::Property p3(app, "top_bv");
    fprintf(stderr, "Reading mapped property: biasValue is %" PRIu32 "\n",
	    p3.getULongValue());
    app.start();
    fprintf(stderr, "Application started/running\n");
    app.wait();
    fprintf(stderr, "Application finished\n");
    app.finish();
    std::string name, value;
    for (unsigned n = 0; app.getProperty(n, name, value); n++)
      fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
    return 0;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
  return 1;
}
