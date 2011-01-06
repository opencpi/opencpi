
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


#include "OcpiXmlOutputter.h"
#include "OcpiUtilCommandLineConfiguration.h"

#include <cppunit/TestResult.h>
#include <cppunit/TextOutputter.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/plugin/PlugInManager.h>
#include <cppunit/plugin/PlugInParameters.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <tr1/functional>

namespace
{
  class OcpiUnitTestConfigurator : public OCPI::Util::CommandLineConfiguration
  {
    public:

      OcpiUnitTestConfigurator (
      const OCPI::Util::CommandLineConfiguration::Option* options )
        : OCPI::Util::CommandLineConfiguration ( options ),
          help ( false ),
          verbose ( false )
      {
        // Empty
      }

      bool help;
      bool verbose;
      MultiString  suites;
  };


  void print_usage ( OcpiUnitTestConfigurator& config,
                     const char* argv0 )
  {
    std::cout << "\n\nUsage: "
              << argv0
              << " [options]"
              << std::endl
              << "  options: "
              << std::endl;

    config.printOptions ( std::cout );
  }


  void add_test_suite ( CppUnit::TextUi::TestRunner* runner,
                        const std::string& suite_name )
  {
    CPPUNIT_NS::stdCOut ( ) << "  ** Adding test suite: "
                            <<  suite_name
                            << std::endl;
    runner->addTest ( CppUnit::TestFactoryRegistry::getRegistry( suite_name ).makeTest() );
  }


  bool run_tests ( CppUnit::TextUi::TestRunner& runner )
  {
    bool was_successful = false;

    CPPUNIT_NS::PlugInManager plugin_manager;
    {
      CPPUNIT_NS::TestResult controller;

      CPPUNIT_NS::TestResultCollector result;

      controller.addListener ( &result );

      CPPUNIT_NS::OStream* xml_stream =
               new CPPUNIT_NS::OFileStream ( "ocpi_test_results_junit.xml" );

      OCPI::XmlOutputter xmlOutputter ( &result, *xml_stream );

      CPPUNIT_NS::OStream* stream = &CPPUNIT_NS::stdCOut ();

      CPPUNIT_NS::TextOutputter textOutputter ( &result, *stream );

      CPPUNIT_NS::PlugInParameters plugin_params ( "text" );

      plugin_manager.load ( "libframework.so", plugin_params );

      plugin_manager.addListener ( &controller );

      // Runs the specified test
      try
      {
        CPPUNIT_NS::stdCOut ( ) << "  ** Running the tests..." << std::endl;
        runner.run ( controller );

        was_successful = result.wasSuccessful ( );
      }
      catch ( std::invalid_argument & )
      {
        CPPUNIT_NS::stdCOut ( )  <<  "Failed to resolve test path: "
                                 <<  "path"
                                 <<  "\n";
      }

      plugin_manager.removeListener ( &controller );

      CPPUNIT_NS::stdCOut ( ) << "  ** Test results" << std::endl;

      textOutputter.write ();

      plugin_manager.addXmlOutputterHooks ( &xmlOutputter );
      xmlOutputter.write ( );
      plugin_manager.removeXmlOutputterHooks ();
      delete xml_stream;
    }

    return was_successful;
  }

} // End: namespace<unamed>

int main ( int argc, char* argv [ ] )
{
  try
  {
    std::cout << "\nOpenCPI Test Facility\n" << std::endl;

    /* ---- Get the command line arguments ------------------------------- */

    OCPI::Util::CommandLineConfiguration::Option options [ ] =
    {
      { OCPI::Util::CommandLineConfiguration::OptionType::MULTISTRING,
        "suite", "Test suites to run",
        OCPI_CLC_OPT( &OcpiUnitTestConfigurator::suites ), 0 },
      { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
        "verbose", "Be verbose",
        OCPI_CLC_OPT( &OcpiUnitTestConfigurator::verbose ), 0 },
      { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
        "help", "This message",
        OCPI_CLC_OPT( &OcpiUnitTestConfigurator::help ), 0 },
      { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
    };

    OcpiUnitTestConfigurator config ( options );

    config.configure ( argc, argv );

    if ( config.help )
    {
      print_usage ( config, argv [ 0 ] );
      return 0;
    }

    /* ---- Make the tests for each suite -------------------------------- */

    CppUnit::TextUi::TestRunner runner;

    std::for_each ( config.suites.begin ( ),
                    config.suites.end ( ),
                    std::tr1::bind ( add_test_suite,
                                     &runner,
                                     std::tr1::placeholders::_1 ) );

    /* ---- Run the tests and reports the results ------------------------ */

    bool was_successful = run_tests ( runner );

    std::cout << "\nTesting"
              << ( ( was_successful ) ? " was " : " was not " )
              << "successful.\n"
              << std::endl;
  }
  catch ( const std::string& oops )
  {
    std::cerr << "Exception(s): " << oops << std::endl;
    return -1;
  }
  catch ( ... )
  {
    std::cerr << "Exception(u): unknown" << std::endl;
    return -1;
  }

  std::cout << "\nOpenCPI Test Facility is done\n" << std::endl;

  return 0;
}
