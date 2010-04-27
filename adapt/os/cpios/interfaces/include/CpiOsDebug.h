// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

// -*- c++ -*-

#ifndef CPIOSDEBUG_H__
#define CPIOSDEBUG_H__

/**
 * \file
 * \brief Operations related to debugging.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Moved cpiAssert to CpiOsAssert.h
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <iostream>

namespace CPI {
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

  }
}

#endif
