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

// -*- c++ -*-

#ifndef OCPIOSWIN32DUMPSTACK_H__
#define OCPIOSWIN32DUMPSTACK_H__

/*
 * Abstract:
 *
 *     Win32 implementation of the OCPI::OS::dumpStack() command.
 *     (Internal to OCPI OS. Not exported.)
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <ostream>

namespace OCPI {
  namespace OS {
    namespace Win32 {

      /*
       * Writes a stack trace of the current thread (similar to the "bt"
       * command in gdb) to the stream. The operation returns, and the
       * program can continue.
       *
       * [in] out: The stream to print the stack trace to.
       */

      void dumpStack (std::ostream & out)
        throw ();

    }
  }
}

#endif
