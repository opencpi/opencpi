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

/*
 * CPI Magic Property String Encoder
 *
 * Revision History:
 *
 *     10/14/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <cstring>
#include <cstdlib>
#include <string>
#include <fstream>
#include <CpiOsAssert.h>
#include <CpiUtilVfs.h>
#include <CpiUtilFileFs.h>
#include <CpiOsDebug.h>
#include <CpiUtilCommandLineConfiguration.h>
#include <CpiScaPropertyParser.h>
#include <sca_props.h>

class PropertyEncoderConfigurator
  : public CPI::Util::CommandLineConfiguration
{
public:
  PropertyEncoderConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool verbose;

private:
  static CommandLineConfiguration::Option g_options[];
};

PropertyEncoderConfigurator::
PropertyEncoderConfigurator ()
  : CPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    verbose (false)
{
}

CPI::Util::CommandLineConfiguration::Option
PropertyEncoderConfigurator::g_options[] = {
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    CPI_CLC_OPT(&PropertyEncoderConfigurator::verbose) },
#if !defined (NDEBUG)
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    CPI_CLC_OPT(&PropertyEncoderConfigurator::debugBreak) },
#endif
  { CPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    CPI_CLC_OPT(&PropertyEncoderConfigurator::help) },
  { CPI::Util::CommandLineConfiguration::OptionType::END }
};

static
void
printUsage (PropertyEncoderConfigurator & config,
	    const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options] <SCD-or-PRF-file>" << std::endl
	    << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * ----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------
 */

static
bool
propertyEncoderInt (int argc, char * argv[])
{
  PropertyEncoderConfigurator config;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: " << oops << std::endl;
    return false;
  }

  if (config.help || argc != 2) {
    printUsage (config, argv[0]);
    return false;
  }

  /*
   * Construct a FileFs to read the XML files from.
   */

  CPI::Util::FileFs::FileFs fileFs ("/");

  /*
   * Figure out the name of our SCD file.
   */

  std::string scdFileName;

  try {
    scdFileName = fileFs.fromNativeName (argv[1]);
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: \"" << argv[1] << "\": " << oops << "." << std::endl;
    return false;
  }

  try {
    CPI::SCA::PropertyParser props (fileFs, scdFileName);
    std::string ps = props.encode ();
    std::cout << ps << std::endl;
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: " << oops << "." << std::endl;
    return false;
  }

  return true;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  propertyEncoder (int argc, char * argv[])
  {
    return propertyEncoderInt (argc, argv) ? 0 : -1;
  }
}

/*
 * Entrypoint for everybody else.
 */

int
main (int argc, char * argv[])
{
#if !defined (NDEBUG)
  {
    for (int i=1; i<argc; i++) {
      if (std::strcmp (argv[i], "--break") == 0) {
	CPI::OS::debugBreak ();
	break;
      }
    }
  }
#endif

  return propertyEncoderInt (argc, argv) ? 0 : 1;
}

