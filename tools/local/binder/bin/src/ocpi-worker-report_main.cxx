
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


/*
 * OCPI Worker Report
 *
 * Revision History:
 *
 *     03/06/2009 - Frank Pilhofer
 *                  - Renamed to "OCPI Worker Report."
 *                  - Added support for tests.
 *
 *     02/20/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>
#include <OcpiOsDebug.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilEzxml.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <OcpiScaPropertyParser.h>
#include <sca_props.h>

/*
 * ----------------------------------------------------------------------
 * Command-line configuration
 * ----------------------------------------------------------------------
 */

class OcpiWorkerReportConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiWorkerReportConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool verbose;

  std::string implementation;

private:
  static CommandLineConfiguration::Option g_options[];
};

OcpiWorkerReportConfigurator::
OcpiWorkerReportConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    verbose (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiWorkerReportConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "implementation", "Implementation UUID",
    OCPI_CLC_OPT(&OcpiWorkerReportConfigurator::implementation), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiWorkerReportConfigurator::verbose), 0 },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&OcpiWorkerReportConfigurator::debugBreak), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiWorkerReportConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiWorkerReportConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options] <SPD-SCD-or-PRF-file>" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * ----------------------------------------------------------------------
 * Code generator class.
 * ----------------------------------------------------------------------
 */

class OcpiPropsEnumerator : public OCPI::SCA::PropertyParser {
public:
  OcpiPropsEnumerator (std::ostream & out)
    throw ();
  void enumerateProps ()
    throw ();

protected:
  std::ostream & m_out;
};

OcpiPropsEnumerator::
OcpiPropsEnumerator (std::ostream & out)
  throw ()
  : m_out (out)
{
}

void
OcpiPropsEnumerator::
enumerateProps ()
  throw ()
{
  m_out << "Component type: " << getType () << std::endl;

  /*
   * Enumeration of the worker's properties.
   */

  for (unsigned int pi=0; pi<m_numProperties; pi++) {
    OCPI::SCA::Property & p = m_properties[pi];
    OCPI::SCA::SimpleType & pt = p.types[0];

    ocpiAssert (!p.is_struct);
    ocpiAssert (p.num_members == 1);

    if (p.is_sequence) {
      m_out << "Property "
            << pi+1
            << ": \""
            << p.name
            << "\" length: Readwrite unsigned long at offset "
            << p.offset
            << ", size 4."
            << std::endl;
    }

    m_out << "Property "
          << pi+1
          << ": \""
          << p.name
          << "\": ";

    if (p.is_readable && p.is_writable) {
      m_out << "Readwrite ";
    }
    else if (p.is_readable) {
      m_out << "Readonly ";
    }
    else if (p.is_writable) {
      m_out << "Writeonly ";
    }

    if (p.is_test) {
      m_out << "test parameter, ";
    }

    if (p.is_sequence) {
      m_out << "array of ";
    }

    switch (pt.data_type) {
    case OCPI::SCA::SCA_boolean: m_out << "boolean"; break;
    case OCPI::SCA::SCA_char: m_out << "char"; break;
    case OCPI::SCA::SCA_double: m_out << "double"; break;
    case OCPI::SCA::SCA_float: m_out << "float"; break;
    case OCPI::SCA::SCA_short: m_out << "short"; break;
    case OCPI::SCA::SCA_long: m_out << "long"; break;
    case OCPI::SCA::SCA_octet: m_out << "octet"; break;
    case OCPI::SCA::SCA_ulong: m_out << "unsigned long"; break;
    case OCPI::SCA::SCA_ushort: m_out << "unsigned short"; break;
    case OCPI::SCA::SCA_string: m_out << "string"; break;
    default: m_out << "UNKNOWN"; break;
    }

    m_out << " at offset " << ((p.is_sequence) ? p.data_offset : p.offset);

    if (p.is_sequence) {
      ocpiAssert (pt.data_type != OCPI::SCA::SCA_string);
      m_out << ", array size " << p.sequence_size;
    }
    else if (pt.data_type == OCPI::SCA::SCA_string) {
      m_out << ", string length " << pt.size;
    }

    if (p.is_sequence) {
      m_out << ", size " << (p.sequence_size * propertySize (pt.data_type));
    }
    else if (pt.data_type == OCPI::SCA::SCA_string) {
      m_out << ", size " << pt.size;
    }
    else {
      m_out << ", size " << propertySize (pt.data_type);
    }

    m_out << "." << std::endl;
  }

  /*
   * Enumeration of the worker's ports.
   */

  for (unsigned int pi=0; pi<m_numPorts; pi++) {
    OCPI::SCA::Port & p = m_ports[pi];

    m_out << "Port "
          << pi
          << ": \""
          << p.name
          << "\": ";

    if (p.provider) {
      m_out << "Input port";
    }
    else {
      m_out << "Output port";
    }

    m_out << "." << std::endl;
  }

  /*
   * Enumeration of the worker's tests.
   */

  for (unsigned int ti=0; ti<m_numTests; ti++) {
    OCPI::SCA::Test & t = m_tests[ti];

    m_out << "Test " << t.testId;

    if (t.numInputs) {
      m_out << ": Input values: ";

      for (unsigned int ii=0; ii<t.numInputs; ii++) {
        if (ii) {
          m_out << ", ";
        }

        ocpiAssert (t.inputValues[ii] < m_numProperties);

        m_out << m_properties[t.inputValues[ii]].name;
      }
    }

    if (t.numResults) {
      if (t.numInputs) {
        m_out << "; ";
      }
      else {
        m_out << ": ";
      }

      m_out << "Result values: ";

      for (unsigned int oi=0; oi<t.numResults; oi++) {
        if (oi) {
          m_out << ", ";
        }

        ocpiAssert (t.resultValues[oi] < m_numProperties);

        m_out << m_properties[t.resultValues[oi]].name;
      }
    }

    m_out << "." << std::endl;
  }
}

/*
 * ----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------
 */

static
bool
findImplInSPD (ezxml_t spdRoot,
               const std::string & implId)
  throw ()
{
  ezxml_t implNode = ezxml_child (spdRoot, "implementation");

  while (implNode) {
    const char * id = ezxml_attr (implNode, "id");

    if (id && implId == id) {
      return true;
    }

    implNode = ezxml_next (implNode);
  }

  return false;
}

static
bool
ocpiWorkerReportInt (int argc, char * argv[])
{
  OcpiWorkerReportConfigurator config;

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
   * Construct a FileFs for file access.
   */

  OCPI::Util::FileFs fileFs ("/");

  /*
   * Read file.
   */

  if (config.verbose) {
    std::cout << "Determining file type of \""
              << argv[1]
              << "\" ... "
              << std::flush;
  }

  std::string fileName;

  try {
    fileName = fileFs.fromNativeName (argv[1]);
  }
  catch (const std::string & oops) {
    if (config.verbose) {
      std::cout << "failed." << std::endl;
    }
    std::cerr << "Oops: \"" << argv[1] << "\": " << oops << "." << std::endl;
    return false;
  }

  OCPI::Util::EzXml::Doc doc;
  ezxml_t root;

  try {
    root = doc.parse (fileFs, fileName);
  }
  catch (const std::string & oops) {
    std::string errMsg = "Failed to parse \"";
    errMsg += fileName;
    errMsg += "\": ";
    errMsg += oops;
    throw errMsg;
  }

  const char * type = ezxml_name (root);
  ocpiAssert (root && type);

  if (config.verbose) {
    if (std::strcmp (type, "softpkg") == 0) {
      std::cout << "Software Package Descriptor." << std::endl;
    }
    else if (std::strcmp (type, "softwarecomponent") == 0) {
      std::cout << "Software Component Descriptor." << std::endl;
    }
    else if (std::strcmp (type, "properties") == 0) {
      std::cout << "Property File." << std::endl;
    }
    else {
      std::cout << type << std::endl;
    }
  }

  if (config.verbose) {
    std::cout << "Processing \""
              << argv[1]
              << "\" ... "
              << std::flush;
  }

  OcpiPropsEnumerator codeGenerator (std::cout);

  try {
    if (std::strcmp (type, "softpkg") == 0) {
      if (config.implementation.length() &&
          !findImplInSPD (root, config.implementation)) {
        std::cerr << "Warning: Implementation \""
                  << config.implementation
                  << "\" not found in software package.  Ignoring."
                  << std::endl;
        config.implementation.clear ();
      }

      codeGenerator.processSPD (fileFs, fileName, config.implementation, root);
    }
    else if (std::strcmp (type, "softwarecomponent") == 0) {
      if (config.implementation.length()) {
        std::cerr << "Warning: SCD file provided.  Ignoring implementation identifier."
                  << std::endl;
      }

      codeGenerator.processSCD (fileFs, fileName, root);
    }
    else if (std::strcmp (type, "properties") == 0) {
      if (config.implementation.length()) {
        std::cerr << "Warning: SCD file provided.  Ignoring implementation identifier."
                  << std::endl;
      }

      codeGenerator.processPRF (root);
    }
    else {
      std::string errMsg = "Input file \"";
      errMsg += fileName;
      errMsg += "\" is not an SPD, SCD or PRF file, but a \"";
      errMsg += type;
      errMsg += "\" file";
      throw errMsg;
    }
  }
  catch (const std::string & oops) {
    if (config.verbose) {
      std::cout << "failed." << std::endl;
    }
    std::cerr << "Oops: " << oops << "." << std::endl;
    return false;
  }

  if (config.verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Enumerate properties.
   */

  codeGenerator.enumerateProps ();

  /*
   * Done.
   */

  return true;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  ocpiWorkerReport (int argc, char * argv[])
  {
    return ocpiWorkerReportInt (argc, argv) ? 0 : -1;
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
        OCPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return ocpiWorkerReportInt (argc, argv) ? 0 : 1;
}

