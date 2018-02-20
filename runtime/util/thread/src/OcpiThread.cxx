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

#include "OcpiOsThreadManager.h"
#include "OcpiThread.h"
namespace OCPI {
namespace Util {


static void
exitbad(const char *e) {
  fprintf(stderr, "Exiting due to background thread exception: %s\n", e);
  exit(1);
}
static void
thread_proc(void *obj) {
  // Argument is reference to "this". Invoke derived class run method.
  try {
    static_cast<Thread*>(obj)->run();
  } catch (std::string &e) {
    exitbad(e.c_str());
  } catch (const char *e) {
    exitbad(e);
  } catch (std::exception &e) {
    exitbad(e.what());
  } catch (...) {
    exitbad("Unexpected exception");
  }
}

void Thread::start() {
  // Create a new thread
  m_pobjThreadServices->start (thread_proc, (void *)this);
  m_joined = false;
}

void Thread::join() {
  m_pobjThreadServices->join ();
  m_joined = true;
}

}
}
