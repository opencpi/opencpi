
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <iostream>
#include <signal.h>
#include <cstdlib>
#include <unistd.h>
#include <execinfo.h>
#include <stdarg.h>
#include <OcpiOsDebug.h>
#include <OcpiOsMutex.h>

namespace OCPI {
  namespace OS {
    void
    dumpStack (std::ostream & out)
      throw ()
    {
      void * bt[40];

      int bts = backtrace (bt, 40);
      char ** btsyms = backtrace_symbols (bt, bts);

      for (int i=0; i<bts; i++) {
	out << btsyms[i] << std::endl;
      }

      free (btsyms);
    }

    void
    debugBreak ()
      throw ()
    {
      kill (getpid(), SIGINT);
    }

    static Mutex mine;
    static unsigned logLevel = UINT_MAX;
    void
    logPrint(unsigned n, const char *fmt, ...) throw() {
      if (logLevel != UINT_MAX && n > logLevel)
	return;
      mine.lock();
      if (logLevel == UINT_MAX) {
	const char *e = getenv("OCPI_LOG_LEVEL");
	logLevel = e ? atoi(e) : 9;
      }
      if (n <= (unsigned)logLevel)  {
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "OCPI(%2d): ", n);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (fmt[strlen(fmt)-1] != '\n')
	  fprintf(stderr, "\n");
	fflush(stderr);
      }
      mine.unlock();
    }
  }
}
