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
#include "ocpi-config.h" // for OCPI_OS_linux
#include "OcpiOsLoadableModule.h"

// TODO: Non-Linux versions?

namespace {

  using namespace OCPI::OS;

  class TestOcpiOsLoadableModule : public ::testing::Test {
    protected:
      std::string sym_to_load, library_to_load;
      void SetUp() /* override */ {
#ifdef OCPI_OS_linux
        library_to_load = "libm.";
        library_to_load += LoadableModule::suffix();
        sym_to_load = "atan";
#else
        library_to_load = sym_to_load = "FAIL";
        std::cerr << "[ WARNING! ] This platform is not fully testable and some tests may fail.\n";
#endif
      }
  };

  TEST_F( TestOcpiOsLoadableModule, empty_constructor )
  {
    LoadableModule loader;
    SUCCEED(); // Optional / explicit
  }

  TEST_F( TestOcpiOsLoadableModule, string_constructor )
  {
    LoadableModule loader(library_to_load);
    SUCCEED(); // Optional / explicit
  }

  TEST_F( TestOcpiOsLoadableModule, string_constructor_bad )
  {
    EXPECT_THROW( LoadableModule loader("X"), std::string );
  }

  TEST_F( TestOcpiOsLoadableModule, string_open )
  {
    LoadableModule loader;
    loader.open(library_to_load);
    SUCCEED(); // Optional / explicit
  }

#if !defined(NDEBUG)
  TEST_F( TestOcpiOsLoadableModule, string_open_twice_death )
  {
    LoadableModule loader;
    loader.open(library_to_load);
    EXPECT_DEATH( loader.open(library_to_load), "!o2vp \\(m_osOpaque\\) is false" );
  }
#endif

  TEST_F( TestOcpiOsLoadableModule, string_open_bad )
  {
    LoadableModule loader;
    std::string bad("X");
    EXPECT_THROW( loader.open(bad), std::string );
  }

  TEST_F( TestOcpiOsLoadableModule, getsymbol )
  {
    LoadableModule loader(library_to_load);
    void *sym = loader.getSymbol(sym_to_load);
    EXPECT_NE( sym, static_cast<void *>(NULL) );
  }

  TEST_F( TestOcpiOsLoadableModule, getsymbol_bad )
  {
    LoadableModule loader(library_to_load);
    void* BAD = reinterpret_cast<void *>(0xBAD);
    void *sym = BAD;
    EXPECT_THROW( sym = loader.getSymbol("X"), std::string );
    EXPECT_EQ( sym, BAD );
  }

  TEST_F( TestOcpiOsLoadableModule, load )
  {
    std::string err;
    void *handle = LoadableModule::load(library_to_load.c_str(), false, err);
    EXPECT_NE( handle, static_cast<void *>(NULL) );
    EXPECT_EQ( err, "" );
  }

  TEST_F( TestOcpiOsLoadableModule, load_bad )
  {
    std::string err;
    void *handle = LoadableModule::load("X", false, err);
    const char *err_msg = "error loading \"X\": ";

    EXPECT_EQ( err.substr(0, strlen(err_msg)), err_msg );
    EXPECT_EQ( handle, static_cast<void *>(NULL) );
  }

} // anon namespace
