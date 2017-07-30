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
  std::string hello("<application package='ocpi'>"
		    // instance name defaults to file_read since there is only one
		    "  <instance component='file_read' externals='true'>"
		    "    <property name='filename' value='hello.file'/>"
		    "    <property name='messageSize' value='4'/>"
		    "  </instance>"
		    "</application>");
  try {
    OA::PValue pvs[] = { OA::PVBool("verbose", true), OA::PVEnd };
    OA::Application app(hello, pvs);
    fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    fprintf(stderr, "Application established: containers, workers, connections all created\n");
    OA::ExternalPort &ep = app.getPort("out");
    fprintf(stderr, "Communication with the application established\n");
    app.start();
    fprintf(stderr, "Application started/running\n");
    size_t length;
    bool end;
    do {
      OA::ExternalBuffer *b;
      uint8_t opcode;
      uint8_t *data;
      while (!(b = ep.getBuffer(data, length, opcode, end)))
	sleep(1);
      if (length > 0)
	write(2, data, length);
      b->release();
    } while (length > 0 && !end);
    return 0;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
  return 1;
}
