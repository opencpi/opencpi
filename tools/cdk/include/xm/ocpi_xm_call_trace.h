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

#ifndef INCLUDED_OCPI_XM_CALL_TRACE_H
#define INCLUDED_OCPI_XM_CALL_TRACE_H

#include <string>
#include <fstream>
#include <iomanip>
#include <iostream>

#define OCPI_CALL_TRACE OCPI::CallTrace trace_temp ( __FUNCTION__ );

namespace OCPI
{
/**
  @brief
    Class CallTrace provides a simple interface for logging a call trace
    during execution.

  Below is an example use of the class.

  @code
      #include "CallTrace.h"

      void cat ( void ) { CALL_TRACE; }

      void foo ( void ) { CALL_TRACE; cat ( ); }

      void buz ( void ) { CALL_TRACE; }

      void bar ( void ) { CALL_TRACE; buz ( ); }

      void baz ( void ) { CALL_TRACE; bar ( ); }

      int main ( void ) { CALL_TRACE; foo ( ); baz ( ); return ( 0 ); }
  @endcode

  Below is the output from CallTrace for the previous example.

  @code
    0 Enter main()
    1 Enter  foo()
    2 Enter    cat()
    2 Leave    cat()
    1 Leave  foo()
    1 Enter  baz()
    2 Enter    bar()
    3 Enter      buz()
    3 Leave      buz()
    2 Leave    bar()
    1 Leave  baz()
    0 Leave main()
  @endcode

  The macro OCPI_CALL_TRACE invokes the explicit constructor of the CallTrace
  class with the built-in function name macro (__FUNCTION__).  The
  constructor writes logs an "entry" into the function in the log file
  (called trace.txt). When the function exits the CallTrace object
  instantiated by the OCPI_CALL_TRACE macro goes out of scope. When destructor
  logs an "exit" from the function in the log file.

  @note
    This class is not thread safe.
    The output from one run to the next is concatenated to the
    trace.txt file.

************************************************************************** */

  class  CallTrace
  {
    public:

      /**
        @brief
          Create a CallTrace object.

        This function adds an "Enter" entry to the log for the currently
        executing function and the increments the call depth.

      ******************************************************************** */

      explicit CallTrace ( const char* function_name )
        : d_function_name ( function_name )
      {
        log_message ( "Entering" );
        ++d_depth;
      }

      /**
        @brief
          Destroy a CallTrace object.

        This function adds a "Leave" entry to the log for the currently
        executing function and decrements the call depth.

      ******************************************************************** */

      ~CallTrace ( )
      {
        --d_depth;
        log_message ( "Leaving " );
      }

    private:

      /**
        @brief
          Open the trace.txt file and add an event.

        This function opens the log file traced.txt and adds a "Enter" or
        "Leave" entry to the log for the currently executing function.

      ******************************************************************** */

      void log_message ( const char* action )
      {
        std::ofstream ofs ( "trace.txt", ( std::ios::out | std::ios::app ) );

        if ( ofs )
        {
          ofs << d_depth
              << " "
              << action
              << " "
              << std::setw ( ( d_depth + 1 ) * 2 )
              << std::setfill ( ' ' )
              << d_function_name
              << std::endl;

          ofs.close();
        }
      }

    private:

      std::string d_function_name;
      /**< Name of the currently executing function. */

      static std::size_t d_depth;
      /**< Depth of the currently executing function relative to main(). */

    }; // End: class CallTrace

} // End: namespace OCPI

#endif // End: INCLUDED_OCPI_XM_CALL_TRACE_H


