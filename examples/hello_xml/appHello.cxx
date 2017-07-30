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
