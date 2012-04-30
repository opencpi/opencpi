
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
 * Update the SCA Generic Assembly Controller in an assembly.
 *
 * Revision History:
 *
 *     06/26/2009 - Frank Pilhofer
 *                  Bugfix: data connections were being discarded.
 *
 *     05/12/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <map>
#include <set>
#include <string>
#include <cstring>
#include <cstdlib>
#include <OcpiOsDebug.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilUri.h>
#include <OcpiUtilUUID.h>
#include <OcpiUtilMisc.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilEzxml.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <OcpiScaPropertyParser.h>
#include <OcpiScaAssemblyParser.h>
#include <sca_props.h>

namespace {
  /*
   * How to reference the SAD DTD file.
   */

  const char *
  g_defaultSadDtdName =
#if ! defined (OCPI_USES_SCA22)
    "softwareassembly.dtd"
#else
    "dtd/softwareassembly.2.2.dtd"  /* SCA Architect recognizes this name */
#endif
    ;

  /*
   * How to reference the SCD DTD file.
   */

  const char *
  g_defaultScdDtdName =
#if ! defined (OCPI_USES_SCA22)
    "softwarecomponent.dtd"
#else
    "dtd/softwarecomponent.2.2.dtd"  /* SCA Architect recognizes this name */
#endif
    ;

  /*
   * How to reference the PRF DTD file.
   */

  const char *
  g_defaultPrfDtdName =
#if ! defined (OCPI_USES_SCA22)
    "properties.dtd"
#else
    "dtd/properties.2.2.dtd"  /* SCA Architect recognizes this name */
#endif
    ;
}

/*
 * ----------------------------------------------------------------------
 * Command-line configuration
 * ----------------------------------------------------------------------
 */

class OcpiSgacUpdaterConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiSgacUpdaterConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool verbose;

  std::string sadDtdName;
  std::string scdDtdName;
  std::string prfDtdName;

private:
  static CommandLineConfiguration::Option g_options[];
};

OcpiSgacUpdaterConfigurator::
OcpiSgacUpdaterConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    verbose (false),
    sadDtdName (g_defaultSadDtdName),
    scdDtdName (g_defaultScdDtdName),
    prfDtdName (g_defaultPrfDtdName)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiSgacUpdaterConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "sadDtdName", "How to reference the SAD DTD file",
    OCPI_CLC_OPT(&OcpiSgacUpdaterConfigurator::sadDtdName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "scdDtdName", "How to reference the SCD DTD file",
    OCPI_CLC_OPT(&OcpiSgacUpdaterConfigurator::scdDtdName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "prfDtdName", "How to reference the PRF DTD file",
    OCPI_CLC_OPT(&OcpiSgacUpdaterConfigurator::prfDtdName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiSgacUpdaterConfigurator::verbose), 0 },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&OcpiSgacUpdaterConfigurator::debugBreak), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiSgacUpdaterConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiSgacUpdaterConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options] <SAD-file>" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * ----------------------------------------------------------------------
 * Generic Assembly Controller Updater
 * ----------------------------------------------------------------------
 */

class OcpiSgacUpdater {
public:
  OcpiSgacUpdater (OCPI::Util::Vfs::Vfs & fs,
                  const std::string & sadFileName,
                  std::ostream & out,
                  bool verbose)
    throw ();

  ~OcpiSgacUpdater ()
    throw ();

  void updateAssembly ()
    throw (std::string);

protected:
  void updateAssemblyController ()
    throw (std::string);

  void updateConnections (ezxml_t sadRoot)
    throw ();

  void loadPropertyFiles ()
    throw (std::string);

  void locateScdPrfFiles (const std::string & spdFileName)
    throw (std::string);

  void writeSCDFile (std::ostream & out)
    throw ();

  void writePRFFile (std::ostream & out)
    throw ();

  std::string resolveFileName (const std::string & baseName,
                               const std::string & relName) const
    throw ();

public:
  static std::string s_sadDtdName;
  static std::string s_scdDtdName;
  static std::string s_prfDtdName;

protected:
  OCPI::Util::Vfs::Vfs & m_fs;
  const std::string & m_sadFileName;
  std::ostream & m_out;
  bool m_verbose;

  OCPI::SCA::AssemblyParser m_assemblyParser;
  std::string m_scdRelName, m_scdFileName;
  std::string m_prfRelName, m_prfFileName;

  /*
   * The information that we need for each component instance.
   */

  struct InstanceInfo {
    InstanceInfo ()
      throw ();
    ~InstanceInfo ()
      throw ();
    std::string usageName;
    OCPI::SCA::PropertyParser * props;
  };

  /*
   * Indexed by component instantiation id.
   */

  typedef std::map<std::string, InstanceInfo> StringToInfoMap;
  StringToInfoMap m_instances;
};

OcpiSgacUpdater::InstanceInfo::
InstanceInfo ()
  throw ()
  : props (0)
{
}

OcpiSgacUpdater::InstanceInfo::
~InstanceInfo ()
  throw ()
{
  delete props;
}

OcpiSgacUpdater::
OcpiSgacUpdater (OCPI::Util::Vfs::Vfs & fs,
                const std::string & sadFileName,
                std::ostream & out,
                bool verbose)
  throw ()
  : m_fs (fs),
    m_sadFileName (sadFileName),
    m_out (out),
    m_verbose (verbose)
{
}

OcpiSgacUpdater::
~OcpiSgacUpdater ()
  throw ()
{
}

std::string
OcpiSgacUpdater::
s_sadDtdName = g_defaultSadDtdName;

std::string
OcpiSgacUpdater::
s_scdDtdName = g_defaultScdDtdName;

std::string
OcpiSgacUpdater::
s_prfDtdName = g_defaultPrfDtdName;

void
OcpiSgacUpdater::
updateAssembly ()
  throw (std::string)
{
  std::string sadRelName = OCPI::Util::Vfs::relativeName (m_sadFileName);

  /*
   * Parse SAD file.
   */

  if (m_verbose) {
    m_out << "Parsing \"" << sadRelName << "\" ... " << std::flush;
  }

  OCPI::Util::EzXml::Doc sadDoc;
  ezxml_t sadRoot;

  try {
    sadRoot = sadDoc.parse (m_fs, m_sadFileName);
  }
  catch (const std::string & oops) {
    if (m_verbose) {
      m_out << "failed." << std::endl;
    }

    std::string msg = "Failed to parse \"";
    msg += m_sadFileName;
    msg += "\": ";
    msg += oops;
    throw msg;
  }

  const char * type = ezxml_name (sadRoot);
  ocpiAssert (sadRoot && type);

  if (std::strcmp (type, "softwareassembly") != 0) {
    if (m_verbose) {
      m_out << "failed." << std::endl;
    }

    std::string msg = "Input file \"";
    msg += m_sadFileName;
    msg += "\" is not a Software Assembly Descriptor, but a \"";
    msg += type;
    msg += "\" file";
    throw msg;
  }

  try {
    m_assemblyParser.processSAD (m_fs, m_sadFileName, sadRoot);
  }
  catch (const std::string & oops) {
    if (m_verbose) {
      m_out << "failed." << std::endl;
    }

    std::string msg = "Error parsing \"";
    msg += m_sadFileName;
    msg += "\": ";
    msg += oops;
    throw msg;
  }

  if (m_verbose) {
    m_out << "done." << std::endl;
  }

  /*
   * Update the assembly controller's properties.
   */

  updateAssemblyController ();

  /*
   * Update the assembly controller's connections.
   */

  updateConnections (sadRoot);

  /*
   * Write the updated SAD file.
   */

  if (m_verbose) {
    m_out << "Updating \"" << sadRelName << "\" ... " << std::flush;
  }

  try {
    std::ostream * sadOut =
      m_fs.openWriteonly (m_sadFileName,
                          std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

    try {
      char * newSAD = ezxml_toxml (sadRoot);
      *sadOut << "<?xml version=\"1.0\" encoding=\"iso-8859-15\"?>" << std::endl;
      *sadOut << "<!DOCTYPE softwareassembly SYSTEM \"" << s_sadDtdName << "\">" << std::endl;
      *sadOut << newSAD << std::endl;
      free (newSAD);
    }
    catch (...) {
      try {
        m_fs.close (sadOut);
      }
      catch (...) {
      }
      throw;
    }

    m_fs.close (sadOut);
  }
  catch (const std::string & oops) {
    if (m_verbose) {
      m_out << "failed." << std::endl;
    }

    std::string msg = oops;
    msg += " while writing \"";
    msg += m_sadFileName;
    msg += "\"";
    throw msg;
  }

  if (m_verbose) {
    m_out << "done." << std::endl;
  }
}

void
OcpiSgacUpdater::
updateAssemblyController ()
  throw (std::string)
{
  /*
   * Locate assembly controller.
   */

  const OCPI::SCA::Assembly & assembly = m_assemblyParser.get ();
  const std::string acInstanceId = assembly.assemblyController;
  OCPI::SCA::StringUIntMap::const_iterator acit = assembly.allComponents.find (acInstanceId);

  if (acit == assembly.allComponents.end()) {
    std::string msg = "Assembly controller \"";
    msg += acInstanceId;
    msg += "\" not found in assembly";
    throw msg;
  }

  OCPI::SCA::ComponentPlacements::const_iterator ocpit;

  if ( (int ) (*acit).second == -1) {
    ocpit = assembly.componentPlacement.find (acInstanceId);
    ocpiAssert (ocpit != assembly.componentPlacement.end());
  }
  else {
    const OCPI::SCA::HostCollocation & hc = assembly.hostCollocation[(*acit).second];
    ocpit = hc.componentPlacement.find (acInstanceId);
    ocpiAssert (ocpit != hc.componentPlacement.end());
  }

  const OCPI::SCA::ComponentPlacement & assemblyController = (*ocpit).second;

  /*
   * We're going to re-generate the SCD and PRF files from scratch.  But
   * we need to figure out their file names.  Unfortunately that means
   * reading both the SPD and the SCD.
   */

  locateScdPrfFiles (assemblyController.spdFileName);

  /*
   * Load the property files for all components.
   */

  loadPropertyFiles ();

  /*
   * Write SCD file.
   */

  if (m_verbose) {
    m_out << "Writing SCD file \"" << m_scdRelName << "\" ... " << std::flush;
  }

  std::ostream * scdOut;

  try {
    scdOut = m_fs.openWriteonly (m_scdFileName,
                                 std::ios_base::out | std::ios_base::trunc);
  }
  catch (const std::string & oops) {
    if (m_verbose) {
      m_out << "failed." << std::endl;
    }

    std::string msg = "Error writing to \"";
    msg += m_scdFileName;
    msg += "\": ";
    msg += oops;
    throw msg;
  }

  writeSCDFile (*scdOut);
  m_fs.close (scdOut);

  if (m_verbose) {
    m_out << "done." << std::endl;
  }

  /*
   * Write PRF file.
   */

  if (m_verbose) {
    m_out << "Writing PRF file \"" << m_prfRelName << "\" ... " << std::flush;
  }

  std::ostream * prfOut;

  try {
    prfOut = m_fs.openWriteonly (m_prfFileName,
                                 std::ios_base::out | std::ios_base::trunc);
  }
  catch (const std::string & oops) {
    if (m_verbose) {
      m_out << "failed." << std::endl;
    }

    std::string msg = "Error writing to \"";
    msg += m_prfFileName;
    msg += "\": ";
    msg += oops;
    throw msg;
  }

  writePRFFile (*prfOut);
  m_fs.close (prfOut);

  if (m_verbose) {
    m_out << "done." << std::endl;
  }
}

void
OcpiSgacUpdater::
updateConnections (ezxml_t sadRoot)
  throw ()
{
  const OCPI::SCA::Assembly & assembly = m_assemblyParser.get ();
  const std::string acInstanceId = assembly.assemblyController;

  /*
   * We want to re-write all connections between the assembly controller
   * and any other component's supported interface, but keep all other
   * connections in the assembly.
   *
   * So the first thing to do is to locate the "connections" element.
   * If it exists, we cut it and substitute our own in its place.  If
   * it doesn't exist, we must insert it.
   */

  ezxml_t oldConnsNode = ezxml_child (sadRoot, "connections");
  ezxml_t connsNode;

  if (oldConnsNode) {
    /*
     * Remove this node and add an empty one in its place.
     */

    size_t off = oldConnsNode->off;
    ezxml_cut (oldConnsNode);
    connsNode = ezxml_add_child (sadRoot, "connections", off);
  }
  else {
    /*
     * Add a new "connections" element.  We need to find a good offset
     * first.  It must be added before the "externalports" element, if
     * it exists.
     */

    ezxml_t epNode = ezxml_child (sadRoot, "externalports");

    if (epNode) {
      /*
       * There is an "externalports" element.  Add new text just before it.
       */

      std::string txt = ezxml_txt (sadRoot);
      std::string insert = "\n    ";
      size_t insLen = insert.length ();
      size_t off = epNode->off;

      txt.insert (off, insert);
      ezxml_set_txt_d (sadRoot, txt.c_str());
      epNode->off += insLen;

      /*
       * Insert new "connections" node.
       */

      connsNode = ezxml_add_child (sadRoot, "connections", off);
    }
    else {
      /*
       * There is no "externalports" node.  Add new "connections" node
       * at the end.
       */

      std::string txt = ezxml_txt (sadRoot);
      std::string insert = "    \n";
      size_t off = txt.length() + 4;

      txt.append (insert);
      ezxml_set_txt_d (sadRoot, txt.c_str());
      connsNode = ezxml_add_child (sadRoot, "implementation", off);
    }
  }

  /*
   * connsNode now points at an empty "connections" element.
   */

  std::string connTxt = "\n    ";
  size_t connOff = 1;

  unsigned int indent = 8;
  std::string sindent (indent, ' ');
  sindent += '\n';

  /*
   * Move over all existing connections that don't involve the assembly
   * controller.
   */

  if (oldConnsNode) {
    ezxml_t connectInterface = ezxml_child (oldConnsNode, "connectinterface");

    while (connectInterface) {
      bool moveConnection = true;

      ezxml_t usesPort = ezxml_child (connectInterface, "usesport");

      if (usesPort) {
        ezxml_t ciRef = ezxml_child (usesPort, "componentinstantiationref");

        if (ciRef) {
          const char * ciRefId = ezxml_attr (ciRef, "refid");

          if (ciRefId && acInstanceId == ciRefId) {
            /*
             * This connection involves the assembly controller.
             */

            ezxml_t csi = ezxml_child (connectInterface, "componentsupportedinterface");

            if (csi) {
              /*
               * And it is to another component's supported interface!
               */

              moveConnection = false;
            }
          }
        }
      }

      ezxml_t thisConnection = connectInterface;
      connectInterface = ezxml_next (connectInterface);

      if (moveConnection) {
        ezxml_move (thisConnection, connsNode, connOff+indent);
        connTxt.insert (connOff, sindent);
        connOff += indent + 1;
      }
    }
  }

  /*
   * Now add connections from the assembly controller to each component's
   * supported interface.
   */

  for (StringToInfoMap::const_iterator iit = m_instances.begin ();
       iit != m_instances.end (); iit++) {
    /*
     * Create a new "connectinterface" element.
     */

    ezxml_t connNode = ezxml_add_child (connsNode, "connectinterface", connOff + indent);
    ezxml_set_txt (connNode, "\n            \n            \n        ");

    ezxml_t upNode = ezxml_add_child (connNode, "usesport", 13);
    ezxml_set_txt (upNode, "\n                \n                \n            ");
    ezxml_t usesIdentifierNode = ezxml_add_child (upNode, "usesidentifier", 17);
    ezxml_set_txt_d (usesIdentifierNode, (*iit).second.usageName.c_str());
    ezxml_t acRefNode = ezxml_add_child (upNode, "componentinstantiationref", 34);
    ezxml_set_attr_d (acRefNode, "refid", acInstanceId.c_str ());

    ezxml_t csiNode = ezxml_add_child (connNode, "componentsupportedinterface", 26);
    ezxml_set_txt (csiNode, "\n                \n                \n            ");
    ezxml_t supportedIdentifierNode = ezxml_add_child (csiNode, "supportedidentifier", 17);
    ezxml_set_txt (supportedIdentifierNode, "IDL:CF/Resource:1.0");
    ezxml_t ciRefNode = ezxml_add_child (csiNode, "componentinstantiationref", 34);
    ezxml_set_attr_d (ciRefNode, "refid", (*iit).first.c_str());

    /*
     * And add it to the connections.
     */

    connTxt.insert (connOff, sindent);
    connOff += indent + 1;
  }

  /*
   * Wrap up.
   */

  ezxml_set_txt_d (connsNode, connTxt.c_str());
  ezxml_free (oldConnsNode);
}

void
OcpiSgacUpdater::
locateScdPrfFiles (const std::string & spdRelName)
  throw (std::string)
{
  std::string spdFileName = resolveFileName (m_sadFileName, spdRelName);

  try {
    if (m_verbose) {
      m_out << "Parsing \"" << spdRelName << "\" ... " << std::flush;
    }

    OCPI::Util::EzXml::Doc spdDoc (m_fs, spdFileName);
    ezxml_t spdRoot = spdDoc.getRootNode ();

    if (!ezxml_name (spdRoot) ||
        std::strcmp (spdRoot->name, "softpkg") != 0) {
      throw std::string ("Not a software package");
    }

    ezxml_t descriptorNode = ezxml_child (spdRoot, "descriptor");

    if (!descriptorNode) {
      throw std::string ("Software package lacks \"descriptor\" element");
    }

    ezxml_t scdLocalFileNode = ezxml_child (descriptorNode, "localfile");

    if (!scdLocalFileNode) {
      throw std::string ("Missing \"localfile\" element");
    }

    const char * scdRelName = ezxml_attr (scdLocalFileNode, "name");

    if (!scdRelName) {
      throw std::string ("No \"name\" attribute in \"localfile\" element");
    }

    m_scdRelName = scdRelName;
    m_scdFileName = resolveFileName (spdFileName, scdRelName);

    if (m_verbose) {
      m_out << "done." << std::endl;
    }
  }
  catch (const std::string & oops) {
    if (m_verbose) {
      m_out << "failed." << std::endl;
    }

    std::string msg = "Error reading \"";
    msg += spdFileName;
    msg += ": ";
    msg += oops;
    throw msg;
  }

  try {
    if (m_verbose) {
      m_out << "Reading \"" << m_scdRelName << "\" ... " << std::flush;
    }

    OCPI::Util::EzXml::Doc scdDoc (m_fs, m_scdFileName);
    ezxml_t scdRoot = scdDoc.getRootNode ();

    if (!ezxml_name (scdRoot) ||
        std::strcmp (scdRoot->name, "softwarecomponent") != 0) {
      throw std::string ("Not a component descriptor");
    }

    ezxml_t propertyFileNode = ezxml_child (scdRoot, "propertyfile");

    if (propertyFileNode) {
      ezxml_t localFileNode = ezxml_child (propertyFileNode, "localfile");

      if (!propertyFileNode) {
        throw std::string ("Missing \"localfile\" element");
      }

      const char * prfRelName = ezxml_attr (localFileNode, "name");

      if (!prfRelName) {
        throw std::string ("No \"name\" attribute in \"localfile\" element");
      }

      m_prfRelName = prfRelName;
      m_prfFileName = resolveFileName (m_scdFileName, prfRelName);

      if (!m_fs.exists (m_prfFileName)) {
        std::string msg = "Property file \"";
        msg += m_prfFileName;
        msg += "\" not found";
        throw msg;
      }
    }
    else {
      /*
       * The property file is optional.  If it doesn't exist, make up our
       * own name by replacing the file extension of the SCD file name.
       */

      std::string::size_type scdFileNameLen = m_scdFileName.length ();

      if (scdFileNameLen > 8 &&
          m_scdFileName.substr (scdFileNameLen-8) == ".scd.xml") {
        m_prfFileName = m_scdFileName.substr (0, scdFileNameLen-8);
        m_prfFileName += "_SCD.prf.xml"; // Adopt SCA Architect's naming convention.
      }
      else {
        m_prfFileName = m_scdFileName;
        m_prfFileName += "_SCD.prf.xml";
      }

      std::string::size_type lastSlash = m_prfFileName.rfind ('/');

      if (lastSlash != std::string::npos) {
        m_prfRelName = m_prfFileName.substr (lastSlash + 1);
      }
      else {
        m_prfRelName = m_prfFileName;
      }
    }

    if (m_verbose) {
      m_out << "done." << std::endl;
    }
  }
  catch (const std::string & oops) {
    if (m_verbose) {
      m_out << "failed." << std::endl;
    }

    std::string msg = "Error reading \"";
    msg += m_scdFileName;
    msg += ": ";
    msg += oops;
    throw msg;
  }
}

void
OcpiSgacUpdater::
loadPropertyFiles ()
  throw (std::string)
{
  /*
   * Process all component instances (that are not the assembly controller)
   * and load their property files.
   */

  const OCPI::SCA::Assembly & assembly = m_assemblyParser.get ();
  OCPI::SCA::ComponentPlacements::const_iterator ocpit;

  for (ocpit  = assembly.componentPlacement.begin();
       ocpit != assembly.componentPlacement.end(); ocpit++) {
    const std::string & componentInstantiationId = (*ocpit).first;

    if (componentInstantiationId == assembly.assemblyController) {
      continue;
    }

    const OCPI::SCA::ComponentPlacement & cp = (*ocpit).second;
    std::string spdFileName = resolveFileName (m_sadFileName, cp.spdFileName);
    OCPI::SCA::PropertyParser * pp = new OCPI::SCA::PropertyParser;

    if (m_verbose) {
      m_out << "Reading \"" << cp.spdFileName << "\" ... " << std::flush;
    }

    try {
      pp->parse (m_fs, spdFileName);
    }
    catch (const std::string & oops) {
      if (m_verbose) {
        m_out << "failed." << std::endl;
      }

      std::string msg = "Error processing \"";
      msg += spdFileName;
      msg += "\": ";
      msg += oops;
      delete pp;
      throw msg;
    }

    if (m_verbose) {
      m_out << "done." << std::endl;
    }

    InstanceInfo & ii = m_instances[componentInstantiationId];
    ii.usageName = cp.usageName;
    ii.props = pp;
  }

  OCPI::SCA::HostCollocations::const_iterator hcit;

  for (hcit  = assembly.hostCollocation.begin();
       hcit != assembly.hostCollocation.end(); hcit++) {
    const OCPI::SCA::HostCollocation & hc = (*hcit);

    for (ocpit  = hc.componentPlacement.begin();
         ocpit != hc.componentPlacement.end(); ocpit++) {
      const std::string & componentInstantiationId = (*ocpit).first;

      if (componentInstantiationId == assembly.assemblyController) {
        continue;
      }

      const OCPI::SCA::ComponentPlacement & cp = (*ocpit).second;
      std::string spdFileName = resolveFileName (m_sadFileName, cp.spdFileName);
      OCPI::SCA::PropertyParser * pp = new OCPI::SCA::PropertyParser;

      if (m_verbose) {
        m_out << "Reading \"" << cp.spdFileName << "\" ... " << std::flush;
      }

      try {
        pp->parse (m_fs, spdFileName);
      }
      catch (const std::string & oops) {
        if (m_verbose) {
          m_out << "failed." << std::endl;
        }

        std::string msg = "Error processing \"";
        msg += spdFileName;
        msg += "\": ";
        msg += oops;
        delete pp;
        throw msg;
      }

      if (m_verbose) {
        m_out << "done." << std::endl;
      }

      InstanceInfo & ii = m_instances[componentInstantiationId];
      ii.usageName = cp.usageName;
      ii.props = pp;
    }
  }
}

void
OcpiSgacUpdater::
writeSCDFile (std::ostream & out)
  throw ()
{
  out << "<?xml version=\"1.0\" encoding=\"iso-8859-15\"?>" << std::endl
      << "<!DOCTYPE softwarecomponent SYSTEM \"" << s_scdDtdName << "\">" << std::endl
      << "<softwarecomponent>" << std::endl
      << "    <corbaversion>2.2</corbaversion>" << std::endl
      << "    <componentrepid repid=\"IDL:CF/Resource:1.0\"/>" << std::endl
      << "    <componenttype>resource</componenttype>" << std::endl
      << "    <componentfeatures>" << std::endl
      << "        <supportsinterface repid=\"IDL:CF/Resource:1.0\" supportsname=\"Resource\"/>" << std::endl
      << "        <supportsinterface repid=\"IDL:CF/PropertySet:1.0\" supportsname=\"PropertySet\"/>" << std::endl
      << "        <supportsinterface repid=\"IDL:CF/PortSupplier:1.0\" supportsname=\"PortSupplier\"/>" << std::endl
      << "        <supportsinterface repid=\"IDL:CF/LifeCycle:1.0\" supportsname=\"LifeCycle\"/>" << std::endl
      << "        <supportsinterface repid=\"IDL:CF/TestableObject:1.0\" supportsname=\"TestableObject\"/>" << std::endl
      << "        <ports>" << std::endl;

  for (StringToInfoMap::const_iterator iit = m_instances.begin ();
       iit != m_instances.end(); iit++) {
    out << "            <uses repid=\"IDL:CF/Resource:1.0\" usesname=\"" << (*iit).second.usageName << "\"/>" << std::endl;
  }

  out << "            <uses repid=\"IDL:LogService/Log:1.0\" usesname=\"LogOut\"/>" << std::endl
      << "        </ports>" << std::endl
      << "    </componentfeatures>" << std::endl
      << "    <interfaces>" << std::endl
      << "        <interface repid=\"IDL:CF/Resource:1.0\" name=\"Resource\">" << std::endl
      << "            <inheritsinterface repid=\"IDL:CF/PropertySet:1.0\"/>" << std::endl
      << "            <inheritsinterface repid=\"IDL:CF/PortSupplier:1.0\"/>" << std::endl
      << "            <inheritsinterface repid=\"IDL:CF/LifeCycle:1.0\"/>" << std::endl
      << "            <inheritsinterface repid=\"IDL:CF/TestableObject:1.0\"/>" << std::endl
      << "        </interface>" << std::endl
      << "        <interface repid=\"IDL:LogService/Log:1.0\" name=\"Log\"/>" << std::endl
      << "    </interfaces>" << std::endl
      << "    <propertyfile>" << std::endl
      << "        <localfile name=\"" << m_prfRelName << "\"/>" << std::endl
      << "    </propertyfile>" << std::endl
      << "</softwarecomponent>" << std::endl;
}

void
OcpiSgacUpdater::
writePRFFile (std::ostream & out)
  throw ()
{
  out << "<?xml version=\"1.0\" encoding=\"iso-8859-15\"?>" << std::endl
      << "<!DOCTYPE properties SYSTEM \"" << s_prfDtdName << "\">" << std::endl
      << "<properties>" << std::endl;

  for (StringToInfoMap::const_iterator iit = m_instances.begin ();
       iit != m_instances.end(); iit++) {
    const InstanceInfo & ii = (*iit).second;

    unsigned int numProps;
    const OCPI::SCA::Property * props = ii.props->getProperties (numProps);

    for (unsigned int pi=0; pi<numProps; pi++) {
      const OCPI::SCA::Property & prop = props[pi];
      std::string name = ii.usageName;
      name += '.';
      name += prop.name;

      out << "    <"
          << (prop.is_sequence ? "simplesequence" : "simple")
          << " id=\"" << name << "\" name=\"" << name << "\" type=\"";

      switch (prop.types[0].data_type) {
      case OCPI::SCA::SCA_boolean: out << "boolean"; break;
      case OCPI::SCA::SCA_char:    out << "char"; break;
      case OCPI::SCA::SCA_double:  out << "double"; break;
      case OCPI::SCA::SCA_float:   out << "float"; break;
      case OCPI::SCA::SCA_short:   out << "short"; break;
      case OCPI::SCA::SCA_long:    out << "long"; break;
      case OCPI::SCA::SCA_octet:   out << "octet"; break;
      case OCPI::SCA::SCA_ulong:   out << "ulong"; break;
      case OCPI::SCA::SCA_ushort:  out << "ushort"; break;
      case OCPI::SCA::SCA_string:  out << "string"; break;
      default: ocpiAssert (0);
      }

      out << "\" mode=\"";

      if (prop.is_readable && prop.is_writable) {
        out << "readwrite";
      }
      else if (prop.is_readable) {
        out << "readonly";
      }
      else if (prop.is_writable) {
        out << "writeonly";
      }
      else {
        ocpiAssert (0);
      }

      out << "\"/>" << std::endl;
    }
  }

  /*
   * SCA Architect insists that PRODUCER_LOG_LEVEL be a sequence of longs.
   */

#if ! defined (OCPI_USES_SCA22)
  out << "    <simplesequence id=\"PRODUCER_LOG_LEVEL\" name=\"PRODUCER_LOG_LEVEL\" type=\"ushort\" mode=\"readwrite\">" << std::endl;
#else
  out << "    <simplesequence id=\"PRODUCER_LOG_LEVEL\" name=\"PRODUCER_LOG_LEVEL\" type=\"long\" mode=\"readwrite\">" << std::endl;
#endif

  /*
   * We wouldn't mind leaving the sequence empty (no need to configure), but
   * Zeligsoft CE complains: "Property is a sequence which requires at least
   * one value."
   */

  out << "        <values>" << std::endl;

  /*
   * SCA 2.2 log level values begin at 0.
   * SCA 2.2.2 CosLwLog log level values begin at 1.
   */

#if defined (OCPI_USES_SCA22)
  out << "            <value>0</value>" << std::endl;
#endif

  out << "            <value>1</value>" << std::endl
      << "            <value>2</value>" << std::endl
      << "            <value>3</value>" << std::endl
      << "            <value>4</value>" << std::endl
      << "            <value>5</value>" << std::endl
      << "            <value>6</value>" << std::endl
      << "            <value>7</value>" << std::endl
      << "            <value>8</value>" << std::endl;

#if ! defined (OCPI_USES_SCA22)
  out << "            <value>9</value>" << std::endl;
#endif

  out << "        </values>" << std::endl;
  out << "    </simplesequence>" << std::endl;

  out << "</properties>" << std::endl;
}

std::string
OcpiSgacUpdater::
resolveFileName (const std::string & baseName,
                 const std::string & relName) const
  throw ()
{
  std::string name;

  try {
    OCPI::Util::Uri uri (m_fs.nameToURI (baseName));
    uri += relName;
    name = m_fs.URIToName (uri.get());
  }
  catch (...) {
  }

  if (!name.length() || !m_fs.exists (name)) {
    std::string::size_type lastSlash = relName.rfind ('/');
    std::string tail;

    if (lastSlash != std::string::npos) {
      tail = relName.substr (lastSlash + 1);
    }
    else {
      tail = relName;
    }

    std::string baseDirName = OCPI::Util::Vfs::directoryName (baseName);
    std::string sameDirName = OCPI::Util::Vfs::joinNames (baseDirName, tail);

    if (m_fs.exists (sameDirName)) {
      name = sameDirName;
    }
  }

  return name;
}

/*
 * ----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------
 */

static
bool
ocpiUpdateSgacInt (int argc, char * argv[])
{
  OcpiSgacUpdaterConfigurator config;

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

  if (config.sadDtdName.length()) {
    OcpiSgacUpdater::s_sadDtdName = config.sadDtdName;
  }

  if (config.scdDtdName.length()) {
    OcpiSgacUpdater::s_scdDtdName = config.scdDtdName;
  }

  if (config.prfDtdName.length()) {
    OcpiSgacUpdater::s_prfDtdName = config.prfDtdName;
  }

  /*
   * Construct a FileFs for file access.
   */

  OCPI::Util::FileFs fileFs ("/");

  /*
   * Read file.
   */

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

  /*
   * Update Assembly Controller.
   */

  OcpiSgacUpdater updater (fileFs,
                          fileName,
                          std::cout,
                          config.verbose);

  try {
    updater.updateAssembly ();
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: " << oops << "." << std::endl;
    return false;
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
  ocpiUpdateSgac (int argc, char * argv[])
  {
    return ocpiUpdateSgacInt (argc, argv) ? 0 : -1;
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

  return ocpiUpdateSgacInt (argc, argv) ? 0 : 1;
}
