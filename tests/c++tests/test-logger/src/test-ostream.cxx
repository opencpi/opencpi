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

#include <sstream>
#include <cstdio>
#include <string>

#include "gtest/gtest.h"

#include "OcpiOsDebug.h"
#include "OcpiLoggerOStreamOutput.h"
#include "OcpiOsThreadManager.h"
#include "OcpiOsMisc.h"


namespace
{
  class TestOcpiLoggerOStreamOutput : public ::testing::Test
  {
    // Empty
  };

  /*
   * ----------------------------------------------------------------------
   * Determines the prefix that OStreamOutput prepends to each message,
   * based on the Producer Id, Producer Name and Log Level. Must be the
   * same logic as in OcpiLoggerOStreamOutput.cxx
   * ----------------------------------------------------------------------
   */

  std::string
  makePrefix (const std::string & producerId,
              unsigned short logLevel,
              const std::string & producerName)
  {
    std::string res;

    if (producerId.length()) {
      res += "(";
      res += producerId;
      res += ") ";
    }

    if (producerName.length()) {
      res += producerName;
      res += ": ";
    }

    switch (logLevel) {
    case 1: res += "SECURITY ALARM: "; break;
    case 2: res += "FAILURE ALARM: "; break;
    case 3: res += "DEGRADED ALARM: "; break;
    case 4: res += "EXCEPTION ERROR: "; break;
    case 5: res += "FLOW CONTROL ERROR: "; break;
    case 6: res += "RANGE ERROR: "; break;
    case 7: res += "USAGE ERROR: "; break;
    case 8: res += "Administrative Event: "; break;
    case 9: res += "Statistic Report: "; break;
    }

    if (logLevel >= 10 && logLevel < 26) {
      char tmp[32];
      std::sprintf (tmp, "%d", (int) (logLevel - 9));
      res += "Debug(";
      res += tmp;
      res += "): ";
    }
    else if (logLevel >= 26) {
      char tmp[32];
      std::sprintf (tmp, "%d", (int) logLevel);
      res += "LogLevel(";
      res += tmp;
      res += "): ";
    }

    return res;
  }

  std::string
  makePrefix (const std::string & producerId,
              OCPI::Logger::Level::LwLogLevel level,
              const std::string & producerName)
  {
    return makePrefix (producerId,
                       static_cast<unsigned short> (level),
                       producerName);
  }

  // Test 1: Writing a "Hello World" log message
  TEST( TestOcpiLoggerOStreamOutput, test_1 )
  {
   std::ostringstream out;
   OCPI::Logger::OStreamOutput logger (out);
   logger.setProducerId ("02-OStreamOutput");
   logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
          << OCPI::Logger::ProducerName ("runHelloWorldTest")
          << "Hello World"
          << std::flush;

   std::string expected = makePrefix ("02-OStreamOutput",
                                      OCPI::Logger::Level::ADMINISTRATIVE_EVENT,
                                      "runHelloWorldTest");
   expected += "Hello World";
   expected += "\n";

   EXPECT_EQ( expected, out.str() );
  }

  // Test 2:  Test thread safety

  void threadSafetyTestThread1 ( void* opaque )
  {
    OCPI::Logger::Logger * logger =
      reinterpret_cast<OCPI::Logger::Logger *> (opaque);

    for (int i=0; i<20; i++) {
      *logger << OCPI::Logger::LogLevel (111)
              << OCPI::Logger::ProducerName ("threadSafetyTestThread1")
              << "Thread 1 - ";

      OCPI::OS::sleep (10);

      *logger << "Thread 1 - ";

      OCPI::OS::sleep (10);

      *logger << "Thread 1."
              << std::flush;
    }
  }

  void threadSafetyTestThread2 ( void* opaque )
  {
    OCPI::Logger::Logger * logger =
      reinterpret_cast<OCPI::Logger::Logger *> (opaque);

    for (int i=0; i<5; i++) {
      *logger << OCPI::Logger::LogLevel (222)
              << OCPI::Logger::ProducerName ("threadSafetyTestThread2")
              << "Hello from Thread 2 - ";

      OCPI::OS::sleep (50);

      *logger << "Hello from Thread 2 - ";

      OCPI::OS::sleep (50);

      *logger << "Goodbye from Thread 2."
              << std::flush;
    }
  }

  TEST( TestOcpiLoggerOStreamOutput, test_2 )
  {
    std::ostringstream out;
    OCPI::Logger::OStreamOutput logger (out);
    OCPI::Logger::Logger * pl = &logger;
    logger.setProducerId ("02-OStreamOutput");

    OCPI::OS::ThreadManager tm1 (threadSafetyTestThread1, pl);
    OCPI::OS::ThreadManager tm2 (threadSafetyTestThread2, pl);

    std::string expected1 = makePrefix ("02-OStreamOutput", 111,
                                        "threadSafetyTestThread1");
    expected1 += "Thread 1 - ";
    expected1 += "Thread 1 - ";
    expected1 += "Thread 1.";
    expected1 += "\n";

    std::string expected2 = makePrefix ("02-OStreamOutput", 222,
                                        "threadSafetyTestThread2");
    expected2 += "Hello from Thread 2 - ";
    expected2 += "Hello from Thread 2 - ";
    expected2 += "Goodbye from Thread 2.";
    expected2 += "\n";

    tm1.join ();
    tm2.join ();

    std::string data = out.str ();
    std::string::size_type pos = 0;
    int seen1 = 0;
    int seen2 = 0;

    while (pos < data.length()) {
      bool found1, found2;

      if (data.substr (pos, expected1.length()) == expected1) {
        found1 = true;
        seen1++;
      }
      else {
        found1 = false;
      }

      if (data.substr (pos, expected2.length()) == expected2) {
        found2 = true;
        seen2++;
      }
      else {
        found2 = false;
      }

      EXPECT_EQ( ( found1 && !found2 ) || ( !found1 && found2 ), true );

      if (found1) {
        pos += expected1.length ();
      }
      else {
        pos += expected2.length ();
      }
    }

    EXPECT_EQ( seen1, 20 );
    EXPECT_EQ( seen2, 5 );
  }

} // End: namespace<unnamed>
