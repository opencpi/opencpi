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
 * CPI Worker Report
 *
 * Revision History:
 *
 *     03/06/2009 - Frank Pilhofer
 *                  - Renamed to "CPI Worker Report."
 *                  - Added support for tests.
 *
 *     02/20/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>
#include <CpiOsDebug.h>
#include <CpiOsAssert.h>
#include <CpiUtilVfs.h>
#include <CpiUtilFileFs.h>
#include <CpiUtilEzxml.h>
#include <CpiUtilCommandLineConfiguration.h>
#include <CpiScaPropertyParser.h>
#include <sca_props.h>

/*
 * ----------------------------------------------------------------------
 * Command-line configuration
 * ----------------------------------------------------------------------
 */

class CpiWorkerReportConfigurator
  : public CPI::Util::CommandLineConfiguration
{
public:
  CpiWorkerReportConfigurator ();

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

CpiWorkerReportConfigurator::
CpiWorkerReportConfigurator ()
  : CPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    verbose (false)
{
}

CPI::Util::CommandLineConfiguration::Option
CpiWorkerReportConfigurator::g_options[] = {
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "implementation", "Implementation UUID",
    CPI_CLC_OPT(&CpiWorkerReportConfigurator::implementation) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    CPI_CLC_OPT(&CpiWorkerReportConfigurator::verbose) },
#if !defined (NDEBUG)
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    CPI_CLC_OPT(&CpiWorkerReportConfigurator::debugBreak) },
#endif
  { CPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    CPI_CLC_OPT(&CpiWorkerReportConfigurator::help) },
  { CPI::Util::CommandLineConfiguration::OptionType::END }
};

static
void
printUsage (CpiWorkerReportConfigurator & config,
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

class CpiPropsEnumerator : public CPI::SCA::PropertyParser {
public:
  CpiPropsEnumerator (std::ostream & out)
    throw ();
  void enumerateProps ()
    throw ();

protected:
  std::ostream & m_out;
};

CpiPropsEnumerator::
CpiPropsEnumerator (std::ostream & out)
  throw ()
  : m_out (out)
{
}

void
CpiPropsEnumerator::
enumerateProps ()
  throw ()
{
  m_out << "Component type: " << getType () << std::endl;

  /*
   * Enumeration of the worker's properties.
   */

  for (unsigned int pi=0; pi<m_numProperties; pi++) {
    CPI::SCA::Property & p = m_properties[pi];
    CPI::SCA::SimpleType & pt = p.types[0];

    cpiAssert (!p.is_struct);
    cpiAssert (p.num_members == 1);

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
    case CPI::SCA::SCA_boolean: m_out << "boolean"; break;
    case CPI::SCA::SCA_char: m_out << "char"; break;
    case CPI::SCA::SCA_double: m_out << "double"; break;
    case CPI::SCA::SCA_float: m_out << "float"; break;
    case CPI::SCA::SCA_short: m_out << "short"; break;
    case CPI::SCA::SCA_long: m_out << "long"; break;
    case CPI::SCA::SCA_octet: m_out << "octet"; break;
    case CPI::SCA::SCA_ulong: m_out << "unsigned long"; break;
    case CPI::SCA::SCA_ushort: m_out << "unsigned short"; break;
    case CPI::SCA::SCA_string: m_out << "string"; break;
    }

    m_out << " at offset " << ((p.is_sequence) ? p.data_offset : p.offset);

    if (p.is_sequence) {
      cpiAssert (pt.data_type != CPI::SCA::SCA_string);
      m_out << ", array size " << p.sequence_size;
    }
    else if (pt.data_type == CPI::SCA::SCA_string) {
      m_out << ", string length " << pt.size;
    }

    if (p.is_sequence) {
      m_out << ", size " << (p.sequence_size * propertySize (pt.data_type));
    }
    else if (pt.data_type == CPI::SCA::SCA_string) {
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
    CPI::SCA::Port & p = m_ports[pi];

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
    CPI::SCA::Test & t = m_tests[ti];

    m_out << "Test " << t.testId;

    if (t.numInputs) {
      m_out << ": Input values: ";

      for (unsigned int ii=0; ii<t.numInputs; ii++) {
        if (ii) {
          m_out << ", ";
        }

        cpiAssert (t.inputValues[ii] < m_numProperties);

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

        cpiAssert (t.resultValues[oi] < m_numProperties);

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
cpiWorkerReportInt (int argc, char * argv[])
{
  CpiWorkerReportConfigurator config;

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

  CPI::Util::FileFs::FileFs fileFs ("/");

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

  CPI::Util::EzXml::Doc doc;
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
  cpiAssert (root && type);

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

  CpiPropsEnumerator codeGenerator (std::cout);

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
  cpiWorkerReport (int argc, char * argv[])
  {
    return cpiWorkerReportInt (argc, argv) ? 0 : -1;
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

  return cpiWorkerReportInt (argc, argv) ? 0 : 1;
}

