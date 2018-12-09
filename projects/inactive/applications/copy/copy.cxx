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
#include <string.h>
#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.hh"

namespace OA = OCPI::API;

int
main(int, char **) {
  try {
    OA::PValue pvs[] = { OA::PVString("model", "=rcc"), OA::PVBool("verbose", true), OA::PVBool("dump", true), OA::PVEnd };
    OA::Application app("<application>"
			"  <instance component='local.copy' worker='copy_cc' externals='true'>"
			"    <property name='ocpi_debug' value='0'/>"
			"  </instance>"
			//			"  <external instance='copy' port='in' buffersize='4000'/>"
		        "</application>", pvs);
    app.initialize();
    OA::ExternalPort
      &pFromMe = app.getPort("in"),
      &pToMe = app.getPort("out");
    app.start();
    size_t length;
    FILE
      *in = fdopen(0, "r"),
      *out = fdopen(1, "w");
    assert(in && out);
    do {
      OA::ExternalBuffer *bFromMe, *bToMe;
      uint8_t *data;
      uint8_t opcode;
      bool end;
      // Get a buffer (that is empty) to put data into that is from me
      while (!(bFromMe = pFromMe.getBuffer(data, length)))
	usleep(1000);
      // Read data from standard input into our output buffer to the worker
      data[length-1] = 0;
      size_t n = fgets((char*)data, length, in) ? strlen((char*)data)+1 : 0;
      // Send our output buffer on its way - as input to the worker/app,
      // giving it back to the system.
      bFromMe->put(n, 23, n == 0);
      // Get a buffer (that is full) to get data that is from the worker/app
      while (!(bToMe = pToMe.getBuffer(data, length, opcode, end)))
	usleep(1000);
      if (length) {
	n = write(1, data, length-1);
	assert(n == length-1);
      }
      // Give the buffer back to the system, I'm done with it.
      bToMe->release();
    } while (length);
    app.wait();
    app.finish();
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return 1;
  }
  
  return 0;
}
