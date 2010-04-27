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

#include <iostream>
#include <string>
#include <CpiLoggerOStreamOutput.h>
#include <CpiLoggerDebugLogger.h>

int
main ()
{
  /*
   * Create a logger that dumps messages on standard output, and set
   * our process-wide "Producer Id" identifier (for production, this
   * should probably include the process name and process id).
   */

  CPI::Logger::OStreamOutput out (std::cout);
  out.setProducerId ("Test");

  /*
   * Print a regular "Administrative Event" log message. Note that
   * the log level MUST be emitted upfront, followed by an optional
   * (but suggested) producer name.
   */

  out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
      << CPI::Logger::ProducerName ("Demo")
      << "Hello World"
      << std::flush;

  /*
   * Arrange for the printing of all debug messages from the "Demo"
   * producer with a verbosity level of 5 or less.
   */

  CPI::Logger::debug ("Demo", 5);

  /*
   * For debug messages, a DebugLogger should be created, instead
   * of using the logger directly.
   *
   * Create a DebugLogger delegating to our regular logger. In a
   * release version, when NDEBUG is defined and optimization is
   * enabled, the compiler will not generate any code for this.
   *
   * Note that the log level MUST NOT be set, it defaults to "Debug".
   *
   * Note that the Producer Name MUST be emitted upfront, it is a
   * mandatory field for debug messages, to allow filtering.
   *
   * Setting the verbosity level is optional, it defaults to 1.
   */

  CPI::Logger::DebugLogger dl (out);
  dl << CPI::Logger::ProducerName ("Demo")
     << CPI::Logger::Verbosity (2)
     << "Hello Debugger"
     << std::flush;

  return 0;
}
