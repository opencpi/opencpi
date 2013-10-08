
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

/**
 * \file
 * \brief Time Emit output format classes
 *
 * Revision History:
 *
 *     01/10/2012 - John F. Miller
 *                  Initial version.
 */


#include <iostream>
#include <fstream>
#include <OcpiTimeEmit.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <OcpiUtilEzxml.h>
#include <OcpiTimeEmitOutputFormat.h>

using namespace OCPI::API;
using namespace OCPI::Time;

class OcpiTimeCvtConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiTimeCvtConfigurator(); 

public:
  bool help;
  bool verbose;
  std::string format;
  std::string infilename;
  std::string outfilename;
  bool        smart;

private:
  static CommandLineConfiguration::Option g_options[];
};

// Configuration
static  OcpiTimeCvtConfigurator config;

OcpiTimeCvtConfigurator::
OcpiTimeCvtConfigurator()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
    smart(true)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiTimeCvtConfigurator::g_options[] = {

  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "format", "Output format <CSV, VCD >",
    OCPI_CLC_OPT(&OcpiTimeCvtConfigurator::format), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "in", "Input filename",
    OCPI_CLC_OPT(&OcpiTimeCvtConfigurator::infilename), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "out", "Output filename",
    OCPI_CLC_OPT(&OcpiTimeCvtConfigurator::outfilename), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "smart", "Smart format formatting for CSV files only",
    OCPI_CLC_OPT(&OcpiTimeCvtConfigurator::smart), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiTimeCvtConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiTimeCvtConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiTimeCvtConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

using namespace OCPI::Time;

int main( int argc, char** argv )
{

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Error: " << oops << std::endl;
    return 1;
  }
  if (config.help) {
    printUsage (config, argv[0]);
    return 1;
  }

  if ( config.infilename.empty() ) {
    std::cerr << "No Input file specified" << std::endl;
    printUsage (config, argv[0]);
    return 1;
  }

  std::ostream * out;
  if ( ! config.outfilename.empty() ) {
    try {
      std::ofstream * fout = new std::ofstream;
      fout->open( config.outfilename.c_str() );
      out =fout;
    }
    catch( std::string & oops ) {
      std::cerr << "Unable to create output file " << config.outfilename << " Error = " << oops << std::endl;
    }
    catch( ...) {
      std::cerr << "Unable to create output file " << config.outfilename << " Unknown error" << std::endl;
    }
  }
  else {
    out = &std::cout;
  }

  // Get the XML formatted data
  OCPI::TimeEmit::Formatter::XMLReader xml_data( config.infilename );  

  if ( config.format == "VCD" ) {
    OCPI::TimeEmit::Formatter::VCDWriter vcd_formatter( xml_data ); 
    *out << vcd_formatter;
  }
  else {
    OCPI::TimeEmit::Formatter::CSVWriter csv_formatter( xml_data, config.smart );  
    *out << csv_formatter;    
  }

  if ( ! config.outfilename.empty() ) { 
    delete out;
  }

}

