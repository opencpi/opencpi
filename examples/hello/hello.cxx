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
  try {
    OA::Application app("<application>"
			"  <instance component='hello' externals='true'/>"
			"</application>");
#if 0 // don't use undocumented/unsupported interfaces
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
#else
  app.initialize();
  OA::ExternalPort &ep = app.getPort("out");
  app.start();
#endif
  OA::ExternalBuffer *b;
  for (unsigned i = 0; i < 100; i++) {
    uint8_t *data;
    size_t length;
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
