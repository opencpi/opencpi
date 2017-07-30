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

#include "gtest/gtest.h"

#include "OcpiOsDebug.h"
#include "OcpiLoggerNullOutput.h"

namespace
{
  class TestOcpiLoggerNullOutput : public ::testing::Test
  {
    // Empty
  };

  // Test 1: Null output test
  TEST( TestOcpiLoggerNullOutput, test_1 )
  {
      OCPI::Logger::NullOutput null;

      null.setProducerId ("01-NullOutput");

      null << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("runNullTest")
           << "Hello World"
           << std::flush;

      EXPECT_EQ( null.good(), true );
  }

} // End: namespace<unnamed>
