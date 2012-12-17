
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


// -*- c++ -*-

#ifndef OCPIOSDEBUG_H__
#define OCPIOSDEBUG_H__

/**
 * \file
 * \brief Operations related to debugging.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Moved ocpiAssert to OcpiOsAssert.h
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <iostream>

#define OCPI_LOG_DEBUG_MIN 10
#define OCPI_LOG_DEBUG 10
#define OCPI_LOG_WIERD 6
#define OCPI_LOG_INFO 8
#define OCPI_LOG_BAD 2

namespace OCPI {
  namespace OS {

    /**
     * Writes a stack trace of the current thread (similar to the "bt"
     * command in gdb) to the stream. The operation returns, and the
     * program can continue.
     *
     * \param[in] out The stream to print the stack trace to.
     *
     * \note On Linux, code must be linked with the "-rdynamic" option for
     * symbol names to be reported correctly.
     *
     * \note On VxWorks, the stack trace is always printed to the console
     * rather than being written to \a out.  The stack trace uses symbols
     * from the system symbol table, which must be populated.  Since
     * non-exported static or inline functions are not loaded into the
     * system symbol table, they will be misreported.  The stack trace
     * will instead show the name of the preceding non-static function.
     */

    void dumpStack (std::ostream & out)
      throw ();

    /**
     * Breaks the program, so that it can be debugged.
     *
     * In Windows, this starts the debugger if the program is not being
     * debugged already.
     *
     * In Unix, this sends a SIGINT signal to the current process, which
     * should be intercepted by the debugger if the program is being
     * debugged.
     *
     * In VxWorks DKM, this suspends the task.  After attaching the
     * debugger, taskResume() can be used from the console to resume
     * the task.
     *
     * This operation returns (as instructed by the debugger), and the
     * program can continue.
     */

    void debugBreak ()
      throw ();

    void logSetLevel(unsigned n);
    void logPrint(unsigned n, const char *fmt, ...) throw() __attribute__((format(printf, 2, 3)));
  }
}

#endif
