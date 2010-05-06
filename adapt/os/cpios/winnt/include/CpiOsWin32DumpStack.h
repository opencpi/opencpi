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

#ifndef CPIOSWIN32DUMPSTACK_H__
#define CPIOSWIN32DUMPSTACK_H__

/*
 * Abstract:
 *
 *     Win32 implementation of the CPI::OS::dumpStack() command.
 *     (Internal to CPI OS. Not exported.)
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <ostream>

namespace CPI {
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
