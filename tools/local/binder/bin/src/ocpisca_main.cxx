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
 * ocpisca: the OpenCPI tool to generate OpenCPI XML from SCA XML
 *
 * Revision History:
 *
 *     06/03/2010 - Jim Kulp
 *                  Change output to OpenCPI XML
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

class OcpiScaConfigurator
  : public CPI::Util::CommandLineConfiguration
{
public:
  OcpiScaConfigurator ();

public:
  std::string name;
  std::string implementation;
  std::string specFile;
  std::string implFile;
  std::string specDir;
  std::string implDir;
  std::string implModel;
  bool verbose;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool help;

private:
  static CommandLineConfiguration::Option g_options[];
};

OcpiScaConfigurator::
OcpiScaConfigurator ()
  : CPI::Util::CommandLineConfiguration (g_options),
    implementation ("0"),
    specDir("."),
    implDir("."),
    implModel("RCC"),
    verbose (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    help (false)
{
}

CPI::Util::CommandLineConfiguration::Option
OcpiScaConfigurator::g_options[] = {
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "name", "Worker implementation name.  Default is softpkg \"name\" attribute.",
    CPI_CLC_OPT(&OcpiScaConfigurator::name) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "implementation", "Implementation UUID or 0-origin implementation index in SPD file.  Default is first implementation in SPD file.",
    CPI_CLC_OPT(&OcpiScaConfigurator::implementation) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "specFile", "Name of generated spec file (before \"_spec.xml\" is appended).  Default is SPD file name (without dir or .spd.xml)",
    CPI_CLC_OPT(&OcpiScaConfigurator::specFile) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "implFile", "Name of generated impl file (before \".xml\" is appended).  Default is SPD file name (without dir or .spd.xml)",
    CPI_CLC_OPT(&OcpiScaConfigurator::implFile) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "specDir", "Directory where spec file will be placed.  Default is CWD.",
    CPI_CLC_OPT(&OcpiScaConfigurator::specDir) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "implDir", "Directory where impl file will be placed. Default is CWD.",
    CPI_CLC_OPT(&OcpiScaConfigurator::implDir) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "implModel", "Authoring model for implementation. Default is RCC.",
    CPI_CLC_OPT(&OcpiScaConfigurator::implModel) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    CPI_CLC_OPT(&OcpiScaConfigurator::verbose) },
#if !defined (NDEBUG)
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    CPI_CLC_OPT(&OcpiScaConfigurator::debugBreak) },
#endif
  { CPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    CPI_CLC_OPT(&OcpiScaConfigurator::help) },
  { CPI::Util::CommandLineConfiguration::OptionType::END }
};

static
void
printUsage (OcpiScaConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options] <SPD-or-SCD-file>" << std::endl
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
ocpiXmlFromScaXmlInt (int argc, char * argv[])
{
  OcpiScaConfigurator config;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: " << oops << std::endl;
    return false;
  }

  if (config.help || argc < 2) {
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

  std::string fileName;

  try {
    fileName = fileFs.fromNativeName (argv[1]);
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: \"" << argv[1] << "\": " << oops << "." << std::endl;
    return false;
  }

  try {
    CPI::SCA::PropertyParser spd (fileFs, fileName, config.implementation.c_str());
    
    const char *err =  spd.emitOcpiXml(config.name, config.specFile,
				       config.specDir, config.implFile,
				       config.implDir, config.implModel,
				       &argv[2], config.verbose);
    if (err)
      std::cerr << "Error: " << err << std::endl;
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
  ocpiXmlFromScaXml (int argc, char * argv[])
  {
    return ocpiXmlFromScaXmlInt (argc, argv) ? 0 : -1;
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

  return ocpiXmlFromScaXmlInt (argc, argv) ? 0 : 1;
}

