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

/**
 * \file
 * \brief Time Emit output format classes
 *
 * Revision History:
 *
 *     10/10/2012 - John F. Miller
 *                  Initial version.
 */


#include <iostream>
#include <fstream>
#include <math.h>
#include <OcpiUtilCommandLineConfiguration.h>

class OcpiConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiConfigurator(); 

public:
  bool help;
  bool verbose;
  long deviation;
  std::string tf;
  std::string ef;

private:
  static CommandLineConfiguration::Option g_options[];
};

// Configuration
static  OcpiConfigurator config;

OcpiConfigurator::
OcpiConfigurator()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false)
  
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiConfigurator::g_options[] = {

  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "tf", "time filename",
    OCPI_CLC_OPT(&OcpiConfigurator::tf), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "ef", "Expected time filename",
    OCPI_CLC_OPT(&OcpiConfigurator::ef), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "deviation", "allowable error in %",
    OCPI_CLC_OPT(&OcpiConfigurator::deviation), 0 },

  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiConfigurator & a_config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  a_config.printOptions (std::cout);
}

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

  if ( config.tf.empty() ) {
    std::cerr << "No Time file specified" << std::endl;
    printUsage (config, argv[0]);
    return 1;
  }

  if ( config.ef.empty() ) {
    std::cerr << "No Expected Time file specified" << std::endl;
    printUsage (config, argv[0]);
    return 1;
  }

  std::ifstream * tfin = new std::ifstream;
  try {
    tfin->open( config.tf.c_str() );
  }
  catch( std::string & oops ) {
    std::cerr << "Unable to open file " << config.tf << " Error = " << oops << std::endl;
  }
  catch( ...) {
    std::cerr << "Unable to open file " << config.tf << " Unknown error" << std::endl;
  }
  if ( tfin->fail() ) {
    throw std::string("Could not open input file");
  }

  std::ifstream * efin = new std::ifstream;
  try {
    efin->open( config.ef.c_str() );
  }
  catch( std::string & oops ) {
    std::cerr << "Unable to open file " << config.ef << " Error = " << oops << std::endl;
  }
  catch( ...) {
    std::cerr << "Unable to open file " << config.ef << " Unknown error" << std::endl;
  }
  if ( efin->fail() ) {
    throw std::string("Could not open input file");
  }

  char line[1024];
  double d=0;
  int c = 0;
  while ( ! tfin->eof() ) {
    tfin->getline( line, 1024 );
    std::cout << line << std::endl;
    
    //3,237631318,1,0,Worker:fr_test_data:1,"Worker Run",142773
    long long v;
    if ( sscanf( line, "%*d,%*d,%*d,%*d,Worker:unit_test:%*d,\"Worker Run\",%lld", &v ) != 1 ) {
      break;
    }
    std::cout << "v = " << v << std::endl;
    d += (double)v;
    c++;
  }
  double Ravg = d/c;

  d = 0;
  c = 0;
  while ( ! efin->eof() ) {
    efin->getline( line, 1024 );
    std::cout << line << std::endl;
    long long v;
    if ( sscanf( line, "%*d,%*d,%*d,%*d,Worker:unit_test:%*d,\"Worker Run\",%lld", &v ) != 1 ) {
      break;
    }
    std::cout << "v = " << v << std::endl;
    d += (double)v;
    c++;
  }
  double Eavg = d/c;

  double delta = fabs( Ravg - Eavg );
  std::cout << "Expected Average Runtime = " << Eavg << " Calculated Average runtime = " << Ravg << " Delta = " << delta << std::endl;
  int ret=0;
  double dev = (double)config.deviation/100.00;
  double Adelta = Eavg * dev;
  std::cout << "Aceptable deviation = " << Adelta << std::endl;
  if ( delta  > Adelta ) {
    std::cout << "Timed Average in run() method FAILED";
    ret = -1;
  }
  else {
    std::cout << "Timed Average in run() method PASSED";
    ret = 0;
  }
  std::cout << std::endl;

  tfin->close();
  delete tfin;
  efin->close();
  delete efin;
  return ret;
}
