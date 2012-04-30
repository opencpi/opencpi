
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
 * OCPI RCC Generator
 *
 * Revision History:
 *
 *     06/23/2009 - Frank Pilhofer
 *                  Generate WWW_N_<IN|OUT>PUT_PORTS constants.
 *
 *     06/17/2009 - Frank Pilhofer
 *                  Bugfix: use worker name as-is for the property and
 *                  context structures.
 *
 *     05/01/2009 - Frank Pilhofer
 *                  Integrate RCC updates.
 *
 *     04/08/2009 - Frank Pilhofer
 *                  - Bugfix: Handle missing double, float, short types.
 *                  - Bugfix: Fix input/output port count.
 *
 *     02/25/2009 - Frank Pilhofer
 *                  Add support for tests.
 *
 *     01/22/2009 - Frank Pilhofer
 *                  Add support for SPD files.
 *
 *     12/05/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>
#include <ctime>
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

class OcpiRccGenConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiRccGenConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool verbose;

  std::string name;
  std::string implementation;
  std::string headerFile;
  std::string skeletonFile;

private:
  static CommandLineConfiguration::Option g_options[];
};

OcpiRccGenConfigurator::
OcpiRccGenConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    verbose (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiRccGenConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "name", "Worker implementation name",
    OCPI_CLC_OPT(&OcpiRccGenConfigurator::name), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "implementation", "Implementation UUID",
    OCPI_CLC_OPT(&OcpiRccGenConfigurator::implementation), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "gen-header-file", "Generate header file",
    OCPI_CLC_OPT(&OcpiRccGenConfigurator::headerFile), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "gen-skeleton-file", "Generate skeleton file",
    OCPI_CLC_OPT(&OcpiRccGenConfigurator::skeletonFile), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiRccGenConfigurator::verbose), 0 },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&OcpiRccGenConfigurator::debugBreak), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiRccGenConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiRccGenConfigurator & config,
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

class OcpiRccGenerator : public OCPI::SCA::PropertyParser {
public:
  OcpiRccGenerator (const std::string & workerName,
                   const std::string & inputFileName)
    throw ();
  void generateHeader (OCPI::Util::Vfs::Vfs & fs,
                       const std::string & fileName)
    throw (std::string);
  void generateSkeleton (OCPI::Util::Vfs::Vfs & fs,
                         const std::string & fileName,
                         const std::string & headerFileName)
    throw (std::string);

protected:
  static unsigned int structAlign (OCPI::SCA::DataType type)
    throw ();

  static std::string uppercase (const std::string & str)
    throw ();

protected:
  std::string m_workerName;
  std::string m_upperWorkerName;
  std::string m_blankedName;
  std::string m_inputFileName;
};

OcpiRccGenerator::
OcpiRccGenerator (const std::string & workerName,
                 const std::string & inputFileName)
  throw ()
  : m_workerName (workerName),
    m_upperWorkerName (uppercase (workerName)),
    m_blankedName (workerName.length(), ' '),
    m_inputFileName (inputFileName)
{
}

void
OcpiRccGenerator::
generateHeader (OCPI::Util::Vfs::Vfs & fs,
                const std::string & fileName)
  throw (std::string)
{
  std::time_t now = std::time (0);
  struct tm * tmnow = std::localtime (&now);
  char tmtmp[64];

  std::strftime (tmtmp, 64, "%F %T", tmnow);
  tmtmp[63] = '\0';

  /*
   * Enforce implementation limit of maximum of 32 ports.
   */

  if (m_numPorts > 32) {
    throw std::string ("Worker has more than 32 ports");
  }

  /*
   * May throw std::string, which is fine with us.
   */

  std::ostream * out = fs.openWriteonly (fileName);

  try {
    *out << "#ifndef RCC_WORKER_" << m_upperWorkerName << "_H__" << std::endl
         << "#define RCC_WORKER_" << m_upperWorkerName << "_H__" << std::endl
         << std::endl;

    *out << "/*" << std::endl
         << " * Header file for worker " << m_workerName << "." << std::endl
         << " * Generated at " << tmtmp << std::endl
         << " * from " << m_inputFileName << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "#include <RCC_Worker.h>" << std::endl
         << std::endl;

    *out << "#if defined (__cplusplus)" << std::endl
         << "extern \"C\" {" << std::endl
         << "#endif" << std::endl
         << std::endl;

    /*
     * Enumeration of the worker's ports.
     *
     * "The enumerated constants will be all upper case, of the form
     * WWW_PPP_RRR_DIR.  WWW is the worker implementation name and PPP is
     * the port name.  RRR and DIR are only included with bidirectional
     * component ports (when some IDL operations are not oneway).  RRR
     * will be "REQUEST" or "REPLY" as appropriate.  DIR will be "IN" or
     * "OUT"."
     *
     * At the moment, we do not support port IDL.  Each port has an implied
     * IDL of "oneway push (OctetSeq data);".  Therefore, the RRR and DIR
     * do not apply.
     */

    *out << "/*" << std::endl
         << " * Worker port ordinals." << std::endl
         << " */" << std::endl
         << std::endl;

    if (m_numPorts) {
      *out << "enum " << m_workerName << "PortOrdinal {" << std::endl;

      for (unsigned int pi=0; pi<m_numPorts; pi++) {
        *out << "  " << m_upperWorkerName << "_" << uppercase (m_ports[pi].name);

        if (pi+1 < m_numPorts) {
          *out << "," << std::endl;
        }
        else {
          *out << std::endl;
        }
      }

      *out << "};" << std::endl
           << std::endl;
    }
    else {
      *out << "/* This worker does not have any ports. */" << std::endl
           << std::endl;
    }

    /*
     * Number of input and output ports.
     */

    unsigned int numInputPorts = 0;
    unsigned int numOutputPorts = 0;

    for (unsigned int pi=0; pi<m_numPorts; pi++) {
      if (m_ports[pi].provider) {
        numInputPorts++;
      }
      else {
        numOutputPorts++;
      }
    }

    *out << "#define " << m_upperWorkerName << "_N_INPUT_PORTS "  << numInputPorts << std::endl
         << "#define " << m_upperWorkerName << "_N_OUTPUT_PORTS " << numOutputPorts << std::endl
         << std::endl;

    /*
     * Worker property space.
     */

    *out << "/*" << std::endl
         << " * Worker property space." << std::endl
         << " */" << std::endl
         << std::endl;

    if (m_numProperties) {
      /*
       * We do have to track the offset, rather than trusting the compiler
       * to match the alignment rules that the PropertyParser applies.  We
       * may have to insert padding where there is a discrepancy.  So far,
       * the only case seems to be the alignment of strings.  We want them
       * to be 4-byte aligned, where the compiler packs them without
       * alignment.
       */

      unsigned int offset = 0;

      *out << "typedef struct {" << std::endl;

      for (unsigned int pi=0; pi<m_numProperties; pi++) {
        OCPI::SCA::Property & p = m_properties[pi];
        OCPI::SCA::SimpleType & pt = p.types[0];

        ocpiAssert (!p.is_struct);
        ocpiAssert (p.num_members == 1);

        if (p.is_sequence) {
          *out << "  uint32_t " << p.name << "_length;" << std::endl;
          offset = roundUp (offset, 4) + 4;
        }
        else if (pt.data_type == OCPI::SCA::SCA_string) {
          /*
           * Strings shall be 4 byte aligned.  Might need padding, because
           * the compiler may think otherwise and pack the string without
           * any alignment.
           */

          if (offset != roundUp (offset, 4)) {
            unsigned int pad = roundUp (offset, 4) - offset;
            *out << "  char pad_" << p.name << "[" << pad << "];" << std::endl;
            offset += pad;
          }
        }
        else {
          ocpiAssert (roundUp (offset, propertyAlign (pt.data_type)) ==
                     roundUp (offset, structAlign (pt.data_type)));
          offset = roundUp (offset, propertyAlign (pt.data_type));
        }

        switch (pt.data_type) {
        case OCPI::SCA::SCA_boolean: *out << "  RCCBoolean "; break;
        case OCPI::SCA::SCA_char: *out << "  uint8_t "; break;
        case OCPI::SCA::SCA_double: *out << "  RCCDouble "; break;
        case OCPI::SCA::SCA_float: *out << "  RCCFloat "; break;
        case OCPI::SCA::SCA_short: *out << "  int16_t "; break;
        case OCPI::SCA::SCA_long: *out << "  int32_t "; break;
        case OCPI::SCA::SCA_octet: *out << "  uint8_t "; break;
        case OCPI::SCA::SCA_ulong: *out << "  uint32_t "; break;
        case OCPI::SCA::SCA_ushort: *out << "  uint16_t "; break;
        case OCPI::SCA::SCA_string: *out << "  RCCChar "; break;
        default: ocpiAssert (0);
        }

        *out << p.name;

        if (p.is_sequence) {
          ocpiAssert (pt.data_type != OCPI::SCA::SCA_string);
          *out << "[" << p.sequence_size << "]";
          offset += propertySize (pt.data_type) * p.sequence_size;
        }
        else if (pt.data_type == OCPI::SCA::SCA_string) {
          *out << "[" << pt.size+1 << "]";
          offset += pt.size+1;
        }
        else {
          offset += propertySize (pt.data_type);
        }

        *out << ";" << std::endl;
      }

      *out << "} " << m_workerName << "Properties;" << std::endl
           << std::endl;
    }

    *out << "#if defined (__cplusplus)" << std::endl
         << "}" << std::endl
         << "#endif" << std::endl;
    *out << "#endif" << std::endl;
  }
  catch (...) {
    try {
      fs.close (out);
    }
    catch (...) {
    }

    try {
      fs.remove (fileName);
    }
    catch (...) {
    }

    throw;
  }
}

void
OcpiRccGenerator::
generateSkeleton (OCPI::Util::Vfs::Vfs & fs,
                  const std::string & fileName,
                  const std::string & headerFileName)
  throw (std::string)
{
  /*
   * May throw std::string, which is fine with us.
   */

  std::ostream * out = fs.openWriteonly (fileName);

  try {
    *out << "#include <stddef.h>" << std::endl
         << "#include <RCC_Worker.h>" << std::endl
         << "#include \"" << headerFileName << "\"" << std::endl
         << std::endl;

    /*
     * Context
     */

    *out << "/*" << std::endl
         << " * Worker private memory." << std::endl
         << " *" << std::endl
         << " * This convenient data structure can be used to hold private data used by" << std::endl
         << " * the worker instance.  The container allocates (but does not initialize)" << std::endl
         << " * an instance of this structure per worker instance." << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "typedef struct {" << std::endl
         << "  /* The \"dummy\" member is only here to make this file compile -- C " << std::endl
         << "     disallows structures without members -- and should be removed when" << std::endl
         << "     other members are added. */" << std::endl
         << "  int dummy;" << std::endl
         << "} " << m_workerName << "Context;" << std::endl
         << std::endl;

    /*
     * Initialize
     */

    *out << "/*" << std::endl
         << " * Worker initialization." << std::endl
         << " *" << std::endl
         << " * The initialize operation provides the worker with the opportunity to" << std::endl
         << " * perform any one-time initialization to achieve a known state prior to" << std::endl
         << " * normal execution." << std::endl
         << " *" << std::endl
         << " * The property set and the worker context (if applicable) should be" << std::endl
         << " * initialized here." << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "static" << std::endl
         << "RCCResult" << std::endl
         << m_workerName << "Initialize (RCCWorker * wctx)" << std::endl
         << "{" << std::endl
         << "  " << m_workerName << "Context * ctx = (" << m_workerName << "Context *) wctx->memories[0];" << std::endl
         << "  " << m_workerName << "Properties * props = (" << m_workerName << "Properties *) wctx->properties;" << std::endl
         << std::endl
         << "  /* TODO: Initialize ctx here. */" << std::endl
         << "  /* TODO: Initialize props here. */" << std::endl
         << std::endl
         << "  return RCC_OK;" << std::endl
         << "}" << std::endl
         << std::endl;

    /*
     * Start
     */

    *out << "/*" << std::endl
         << " * Prepare to run." << std::endl
         << " *" << std::endl
         << " * The start operation provides the worker with the opportunity to perform" << std::endl
         << " * any one-time initialization that are dependent on initial configuration" << std::endl
         << " * property values, since those property values are not set prior to the" << std::endl
         << " * the initialize operation.  This operation also provides the opportunity" << std::endl
         << " * to prepare to resume internal processing after the stop operation is" << std::endl
         << " * used." << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "static" << std::endl
         << "RCCResult" << std::endl
         << m_workerName << "Start (RCCWorker * wctx)" << std::endl
         << "{" << std::endl
         << "  " << m_workerName << "Context * ctx = (" << m_workerName << "Context *) wctx->memories[0];" << std::endl
         << "  " << m_workerName << "Properties * props = (" << m_workerName << "Properties *) wctx->properties;" << std::endl
         << std::endl
         << "  /* TODO: Configure worker according to the property values. */" << std::endl
         << std::endl
         << "  return RCC_OK;" << std::endl
         << "}" << std::endl
         << std::endl;

    /*
     * Stop
     */

    *out << "/*" << std::endl
         << " * Stop processing." << std::endl
         << " *" << std::endl
         << " * The stop operation is provided to command a worker to stop internal" << std::endl
         << " * processing in a way that can be later restarted via the start operation." << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "static" << std::endl
         << "RCCResult" << std::endl
         << m_workerName << "Stop (RCCWorker * wctx)" << std::endl
         << "{" << std::endl
         << "  " << m_workerName << "Context * ctx = (" << m_workerName << "Context *) wctx->memories[0];" << std::endl
         << "  " << m_workerName << "Properties * props = (" << m_workerName << "Properties *) wctx->properties;" << std::endl
         << std::endl
         << "  /* TODO: Stop processing. */" << std::endl
         << std::endl
         << "  return RCC_OK;" << std::endl
         << "}" << std::endl
         << std::endl;

    /*
     * Release
     */

    *out << "/*" << std::endl
         << " * Final processing." << std::endl
         << " *" << std::endl
         << " * The release operation requests that the worker perform any final" << std::endl
         << " * processing.  Any resources shall be released." << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "static" << std::endl
         << "RCCResult" << std::endl
         << m_workerName << "Release (RCCWorker * wctx)" << std::endl
         << "{" << std::endl
         << "  " << m_workerName << "Context * ctx = (" << m_workerName << "Context *) wctx->memories[0];" << std::endl
         << "  " << m_workerName << "Properties * props = (" << m_workerName << "Properties *) wctx->properties;" << std::endl
         << std::endl
         << "  /* TODO: Release resources here. */" << std::endl
         << std::endl
         << "  return RCC_OK;" << std::endl
         << "}" << std::endl
         << std::endl;

    /*
     * AfterConfigure
     */

    if (m_numProperties) {
      *out << "/*" << std::endl
           << " * Notification that properties have changed." << std::endl
           << " *" << std::endl
           << " * The afterConfigure operation allows the worker to be notified when" << std::endl
           << " * some of its configuration properties have changed, according to its" << std::endl
           << " * notification requirements.  This operation notifies the worker that" << std::endl
           << " * one or more such values in the property structure have changed." << std::endl
           << " */" << std::endl
           << std::endl;

      *out << "static" << std::endl
           << "RCCResult" << std::endl
           << m_workerName << "AfterConfigure (RCCWorker * wctx)" << std::endl
           << "{" << std::endl
           << "  " << m_workerName << "Context * ctx = (" << m_workerName << "Context *) wctx->memories[0];" << std::endl
           << "  " << m_workerName << "Properties * props = (" << m_workerName << "Properties *) wctx->properties;" << std::endl
           << std::endl
           << "  /*" << std::endl
           << "   * TODO: Act upon modified properties." << std::endl
           << "   */" << std::endl
           << std::endl
           << "  return RCC_OK;" << std::endl
           << "}" << std::endl
           << std::endl;
    }

    /*
     * BeforeQuery
     */

    if (m_numProperties) {
      *out << "/*" << std::endl
           << " * Notification that properties will be queried." << std::endl
           << " *" << std::endl
           << " * The beforeQuery operation notifies the worker that the container is about" << std::endl
           << " * to read one or more values in the property structure." << std::endl
           << " */" << std::endl
           << std::endl;

      *out << "static" << std::endl
           << "RCCResult" << std::endl
           << m_workerName << "BeforeQuery (RCCWorker * wctx)" << std::endl
           << "{" << std::endl
           << "  " << m_workerName << "Context * ctx = (" << m_workerName << "Context *) wctx->memories[0];" << std::endl
           << "  " << m_workerName << "Properties * props = (" << m_workerName << "Properties *) wctx->properties;" << std::endl
           << std::endl
           << "  /*" << std::endl
           << "   * TODO: Update properties, if applicable." << std::endl
           << "   */" << std::endl
           << std::endl
           << "  return RCC_OK;" << std::endl
           << "}" << std::endl
           << std::endl;
    }

    /*
     * Run
     */

    *out << "/*" << std::endl
         << " * Process data." << std::endl
         << " *" << std::endl
         << " * The run operation requests that the worker perform its normal computation." << std::endl
         << " * The container only calls this operation when the worker's run condition is" << std::endl
         << " * true." << std::endl
         << " *" << std::endl
         << " * Normally this involves using messages in buffers at input ports to produce" << std::endl
         << " * messages at output ports." << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "static" << std::endl
         << "RCCResult" << std::endl
         << m_workerName  << "Run (RCCWorker * wctx," << std::endl
         << m_blankedName << "     RCCBoolean timedout," << std::endl
         << m_blankedName << "     RCCBoolean * newRunCondition)" << std::endl
         << "{" << std::endl
         << "  " << m_workerName << "Context * ctx = (" << m_workerName << "Context *) wctx->memories[0];" << std::endl
         << "  " << m_workerName << "Properties * props = (" << m_workerName << "Properties *) wctx->properties;" << std::endl
         << std::endl;

    for (unsigned int pi=0; pi<m_numPorts; pi++) {
      *out << "  RCCPort * p" << m_ports[pi].name << " = "
           << "&wctx->ports[" << m_upperWorkerName << "_" << uppercase (m_ports[pi].name) << "];"
           << std::endl;
    }

    if (m_numPorts) {
      *out << std::endl
           << "  /* TODO: Process data. Port readiness is indicated by their" << std::endl
           << "     current.data member being non-null.  Ports referenced by" << std::endl
           << "     the worker's run condition are guaranteed to be ready. */" << std::endl
           << std::endl;
    }

    *out << "  /* Returning RCC_ADVANCE indicates that all ports shall be" << std::endl
         << "     advanced, i.e., data at input ports is consumed, and data" << std::endl
         << "     at output ports is produced and sent to the next worker. */" << std::endl
         << std::endl
         << "  return RCC_ADVANCE;" << std::endl;

    *out << "}" << std::endl
         << std::endl;

    /*
     * Test
     */

    if (m_numTests) {
      *out << "/*" << std::endl
           << " * Tests." << std::endl
           << " */" << std::endl
           << std::endl;

      *out << "static" << std::endl
           << "RCCResult" << std::endl
           << m_workerName << "Test (RCCWorker * wctx)" << std::endl
           << "{" << std::endl
           << "  " << m_workerName << "Context * ctx = (" << m_workerName << "Context *) wctx->memories[0];" << std::endl
           << "  " << m_workerName << "Properties * props = (" << m_workerName << "Properties *) wctx->properties;" << std::endl
           << std::endl
           << "  /*" << std::endl
           << "   * Each test has a set of input values and result values, which are" << std::endl
           << "   * passed via the property space.  The test outcome is communicated" << std::endl
           << "   * via result values as well: if a test \"fails\", this function shall" << std::endl
           << "   * return RCC_OK while setting the result values appropriately." << std::endl
           << "   */" << std::endl
           << std::endl;

      for (unsigned int ti=0; ti<m_numTests; ti++) {
        *out << "  ";

        if (ti) {
          *out << "else ";
        }

        *out << "if (props->testId == " << m_tests[ti].testId << ") {" << std::endl
             << "    /*" << std::endl
             << "     * Test Id: " << m_tests[ti].testId << std::endl
             << "     *" << std::endl;

        if (!m_tests[ti].numInputs) {
          *out << "     * This test has no input values." << std::endl;
        }
        else {
          *out << "     * Input values:" << std::endl;
          for (unsigned int ivi=0; ivi<m_tests[ti].numInputs; ivi++) {
            *out << "     * - " << m_properties[m_tests[ti].inputValues[ivi]].name << std::endl;
          }
        }

        // Resultvalues are not optional, there should be some.
        *out << "     * Result values:" << std::endl;
        for (unsigned int rvi=0; rvi<m_tests[ti].numResults; rvi++) {
          *out << "     * - " << m_properties[m_tests[ti].resultValues[rvi]].name << std::endl;
        }

        *out << "     */" << std::endl
             << std::endl;

        *out << "    /* TODO: Implement test. */" << std::endl
             << "  }" << std::endl;
      }

      *out << "  else {" << std::endl
           << "    /* Unknown test id */" << std::endl
           << "    return RCC_ERROR;" << std::endl
           << "  }" << std::endl;

      *out << std::endl
           << "  return RCC_OK;" << std::endl
           << "}" << std::endl
           << std::endl;
    }

    /*
     * Memories
     */

    *out << "/*" << std::endl
         << " * Worker memory requests." << std::endl
         << " *" << std::endl
         << " * These memories are allocated by the container and provided to the worker" << std::endl
         << " * in the \"memories\" member of the RCCWorker structure." << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "static" << std::endl
         << "uint32_t" << std::endl
         << m_workerName << "Memories[] = {" << std::endl
         << "  sizeof (" << m_workerName << "Context)," << std::endl
         << "  0" << std::endl
         << "};" << std::endl
         << std::endl;

    /*
     * Dispatch table
     */

    *out << "/*" << std::endl
         << " * Worker dispatch table." << std::endl
         << " *" << std::endl
         << " * This is the only symbol that is exported (non-static) from this file." << std::endl
         << " * The name of the dispatch table is used as this worker's \"entrypoint\"." << std::endl
         << " */" << std::endl
         << std::endl;

    *out << "RCCDispatch" << std::endl
         << m_workerName << "Worker = {" << std::endl;

    *out << "  /*" << std::endl
         << "   * Information for consistency checking by the container." << std::endl
         << "   */" << std::endl
         << std::endl
         << "  .version = RCC_VERSION," << std::endl
         << "  .numInputs = " << m_upperWorkerName << "_N_INPUT_PORTS," << std::endl
         << "  .numOutputs = " << m_upperWorkerName << "_N_OUTPUT_PORTS," << std::endl
         << "  .propertySize = sizeof (" << m_workerName << "Properties)," << std::endl
         << "  .memSizes = " << m_workerName << "Memories," << std::endl
         << "  .threadProfile = 0," << std::endl
         << std::endl;

    *out << "  /*" << std::endl
         << "   * Methods.  Can be NULL if not needed." << std::endl
         << "   */" << std::endl
         << std::endl
         << "  .initialize = " << m_workerName << "Initialize," << std::endl
         << "  .start = " << m_workerName << "Start," << std::endl
         << "  .stop = " << m_workerName << "Stop," << std::endl
         << "  .release = " << m_workerName << "Release," << std::endl;

    if (m_numProperties) {
      *out << "  .afterConfigure = " << m_workerName << "AfterConfigure," << std::endl
           << "  .beforeQuery = " << m_workerName << "BeforeQuery," << std::endl;
    }
    else {
      *out << "  .afterConfigure = NULL," << std::endl
           << "  .beforeQuery = NULL," << std::endl;
    }

    if (m_numTests) {
      *out << "  .test = " << m_workerName << "AfterConfigure," << std::endl;
    }
    else {
      *out << "  .test = NULL," << std::endl;
    }

    *out << "  .run = " << m_workerName << "Run," << std::endl
         << std::endl;

    *out << "  /*" << std::endl
         << "   * Implementation information for container behavior." << std::endl
         << "   */" << std::endl
         << std::endl
         << "  .runCondition = NULL, /* Implies a run condition of all ports ready. */" << std::endl
         << "  .portInfo = NULL, /* Non-default port information */" << std::endl
         << "  .optionallyConnectedPorts = 0 /* Bit mask */" << std::endl;

    *out << "};" << std::endl;
  }
  catch (...) {
    try {
      fs.close (out);
    }
    catch (...) {
    }

    try {
      fs.remove (fileName);
    }
    catch (...) {
    }

    throw;
  }
}

unsigned int
OcpiRccGenerator::
structAlign (OCPI::SCA::DataType type)
  throw ()
{
  unsigned int align = 0;

  switch (type) {
  case OCPI::SCA::SCA_boolean: align = 1; break;
  case OCPI::SCA::SCA_char:    align = 1; break;
  case OCPI::SCA::SCA_double:  align = 8; break;
  case OCPI::SCA::SCA_float:   align = 4; break;
  case OCPI::SCA::SCA_short:   align = 2; break;
  case OCPI::SCA::SCA_long:    align = 4; break;
  case OCPI::SCA::SCA_octet:   align = 1; break;
  case OCPI::SCA::SCA_ulong:   align = 4; break;
  case OCPI::SCA::SCA_ushort:  align = 2; break;
  default: ocpiCheck (0); break;
  }

  return align;
}

std::string
OcpiRccGenerator::
uppercase (const std::string & str)
  throw ()
{
  const char * ptr = str.data ();
  std::string::size_type i, len = str.length ();
  std::string res;
  char tmp[64], *t;

  while (len) {
    for (t=tmp, i=0; i<64 && i<len; i++) {
      *t++ = std::toupper (*ptr++);
    }
    res.append (tmp, i);
    len -= i;
  }

  return res;
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
ocpiRccGeneratorInt (int argc, char * argv[])
{
  OcpiRccGenConfigurator config;

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

  if (!config.name.length() ||
      !config.headerFile.length()) {
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

  OcpiRccGenerator codeGenerator (config.name, argv[1]);

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
   * Write header file.
   */

  if (config.verbose) {
    std::cout << "Writing header file \""
              << config.headerFile
              << "\" ... "
              << std::flush;
  }

  std::string headerFileName;

  try {
    headerFileName = fileFs.fromNativeName (config.headerFile);
  }
  catch (const std::string & oops) {
    if (config.verbose) {
      std::cout << "failed." << std::endl;
    }
    std::cerr << "Oops: \"" << config.headerFile << "\": " << oops << "." << std::endl;
    return false;
  }

  try {
    codeGenerator.generateHeader (fileFs, headerFileName);
  }
  catch (const std::string & oops) {
    if (config.verbose) {
      std::cout << "failed." << std::endl;
    }
    std::cerr << "Oops: \"" << config.headerFile << "\": " << oops << "." << std::endl;
    return false;
  }

  if (config.verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Write skeleton file.
   */

  if (config.skeletonFile.length()) {
    /*
     * Test if the skeleton file exists.  We don't want to overwrite an
     * existing file.  If the user modified the earlier incarnation, all
     * her or his changes would be lost.
     */

    std::string skeletonFileName;
    bool skeletonFileExists;

    try {
      skeletonFileName = fileFs.fromNativeName (config.skeletonFile);
      skeletonFileExists = fileFs.exists (skeletonFileName);
    }
    catch (const std::string & oops) {
      std::cerr << "Oops: \"" << config.skeletonFile << "\": " << oops << "." << std::endl;
      return false;
    }

    if (skeletonFileExists) {
      std::cerr << "Warning: Not overwriting existing file \""
                << config.skeletonFile
                << "\"." << std::endl;
    }
    else {
      if (config.verbose) {
        std::cout << "Writing skeleton file \""
                  << config.skeletonFile
                  << "\" ... "
                  << std::flush;
      }

      try {
        codeGenerator.generateSkeleton (fileFs, skeletonFileName,
                                        config.headerFile);
      }
      catch (const std::string & oops) {
        if (config.verbose) {
          std::cout << "failed." << std::endl;
        }
        std::cerr << "Oops: \"" << config.skeletonFile << "\": " << oops << "." << std::endl;
        return false;
      }

      if (config.verbose) {
        std::cout << "done." << std::endl;
      }
    }
  }

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
  ocpiRccGenerator (int argc, char * argv[])
  {
    return ocpiRccGeneratorInt (argc, argv) ? 0 : -1;
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

  return ocpiRccGeneratorInt (argc, argv) ? 0 : 1;
}

