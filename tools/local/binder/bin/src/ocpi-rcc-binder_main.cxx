
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
 * OCPI RCC Binder
 *
 * Revision History:
 *
 *     05/14/2009 - Frank Pilhofer
 *                  Add command-line option to set the SPD DTD file name.
 *
 *     03/27/2009 - Frank Pilhofer
 *                  Refactored code to match the OCPI RPL Binder.
 *
 *     12/17/2008 - Frank Pilhofer
 *                  - Add support for SPD files.
 *                  - Add option to back-annotate the SPD file.
 *
 *     11/10/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <cstring>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <OcpiOsAssert.h>
#include <OcpiOsDebug.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilZipFs.h>
#include <OcpiUtilEzxml.h>
#include <OcpiUtilUUID.h>
#include <OcpiUtilMisc.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <OcpiScaPropertyParser.h>
#include <sca_props.h>

namespace {
  /*
   * How to reference the SPD DTD file.
   */

  const char *
  g_defaultSpdDtdName =
#if ! defined (OCPI_USES_SCA22)
    "softpkg.dtd"
#else
    "dtd/softpkg.2.2.dtd"  /* SCA Architect recognizes this name */
#endif
    ;
}

class OcpiRccBinderConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiRccBinderConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool verbose;

  std::string workerDllName;
  MultiNameValue workerScdMap;
  MultiString dependentDlls;
  std::string outputFileName;
  bool updateSPD;
  std::string os;
  std::string processor;
  unsigned long ocpiDeviceId;
  std::string spdDtdName;

private:
  static CommandLineConfiguration::Option g_options[];
};

OcpiRccBinderConfigurator::
OcpiRccBinderConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    verbose (false),
    updateSPD (false),
    ocpiDeviceId (static_cast<unsigned long> (-1)),
    spdDtdName (g_defaultSpdDtdName)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiRccBinderConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "workerDll", "Worker DLL file",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::workerDllName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::MULTINAMEVALUE,
    "worker", "Entrypoint to SPD/SCD/PRF file mapping",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::workerScdMap), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::MULTISTRING,
    "dependsOnDll", "DLLs that the worker DLL depends on",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::dependentDlls), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "output", "Output executable file",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::outputFileName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "updateSPD", "Update implementation information in SPD",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::updateSPD), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "os", "Target platform operating system",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::os), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "processor", "Target platform processor type",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::processor), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "ocpiDeviceId", "Implementation device ID requirement",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::ocpiDeviceId), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "spdDtdName", "How to reference the SPD DTD file",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::spdDtdName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::verbose), 0 },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::debugBreak), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiRccBinderConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * ----------------------------------------------------------------------
 * Worker Information
 * ----------------------------------------------------------------------
 */

struct WorkerInfo {
  WorkerInfo ();
  ~WorkerInfo ();
  WorkerInfo (const WorkerInfo & other);
  WorkerInfo & operator= (const WorkerInfo & other);
  std::string relFileName;
  std::string fileName;
  std::string implementation;
  std::string propertyMagic;
  mutable OCPI::Util::EzXml::Doc * doc;
};

WorkerInfo::WorkerInfo ()
  : doc (0)
{
  doc = new OCPI::Util::EzXml::Doc;
}

WorkerInfo::~WorkerInfo ()
{
  delete doc;
}

WorkerInfo::WorkerInfo (const WorkerInfo & other)
  : doc (other.doc)
{
  other.doc = 0;
}

WorkerInfo &
WorkerInfo::operator= (const WorkerInfo & other)
{
  doc = other.doc;
  other.doc = 0;
  return *this;
}

typedef std::map<std::string,WorkerInfo> WorkerInfos;
typedef std::vector<std::pair<std::string,std::string> > DependencyInfos;

struct ExecutableInfo {
  WorkerInfos workerInfo;
  DependencyInfos dependency;
};

/*
 * ----------------------------------------------------------------------
 * SPD Updater
 * ----------------------------------------------------------------------
 */

class OcpiRccSpdUpdater {
public:
  OcpiRccSpdUpdater (const std::string & exeFileName,
                    const std::string & os,
                    const std::string & processor,
                    unsigned int ocpiDeviceId = static_cast<unsigned int> (-1))
    throw ();

  void updateSPD (ezxml_t root,
                  const std::string & entrypoint,
                  const std::string & implId)
    throw (std::string);

  ezxml_t createSPDfromSCD (const std::string & spdFileName,
                            const std::string & entrypoint,
                            const std::string & implId)
    throw ();

protected:
  ezxml_t findRccImplementation (ezxml_t root)
    throw ();

  static ezxml_t findImplementation (ezxml_t root, const std::string & implId)
    throw ();

  static void insertNode (ezxml_t newParent, ezxml_t newNode,
                          std::string & parentTxt, size_t & parentOff,
                          unsigned int indent)
    throw ();

  static void moveNode (ezxml_t newParent, ezxml_t oldParent, const char * name,
                        std::string & parentTxt, size_t & parentOff,
                        unsigned int indent)
    throw ();

  static ezxml_t createCodeElement (const std::string & outputFileName,
                                    const std::string & entrypoint)
    throw ();

  static ezxml_t createOsElement (const std::string & os)
    throw ();

  static ezxml_t createProcessorElement (const std::string & processor)
    throw ();

  static ezxml_t createOCPIContainerDependencyElement ()
    throw ();

  static ezxml_t createOCPIDeviceIdDependencyElement (unsigned int deviceId)
    throw ();

protected:
  static const char * s_ocpiRuntimePropertyId;
  static const char * s_ocpiRuntimeRccName;
  static const char * s_ocpiDeviceIdPropertyId;

protected:
  const std::string m_exeFileName;
  const std::string m_os;
  const std::string m_processor;
  unsigned int m_ocpiDeviceId;
};

const char *
OcpiRccSpdUpdater::
s_ocpiRuntimePropertyId = "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43";

const char *
OcpiRccSpdUpdater::
s_ocpiRuntimeRccName = "RCC";

const char *
OcpiRccSpdUpdater::
s_ocpiDeviceIdPropertyId = "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2";

OcpiRccSpdUpdater::
OcpiRccSpdUpdater (const std::string & exeFileName,
                  const std::string & os,
                  const std::string & processor,
                  unsigned int ocpiDeviceId)
  throw ()
  : m_exeFileName (exeFileName),
    m_os (os),
    m_processor (processor),
    m_ocpiDeviceId (ocpiDeviceId)
{
}

void
OcpiRccSpdUpdater::
updateSPD (ezxml_t root,
           const std::string & entrypoint,
           const std::string & implId)
  throw (std::string)
{
  ezxml_t oldImplNode = 0;
  ezxml_t implNode;

  if (implId.length()) {
    oldImplNode = findImplementation (root, implId);
  }
  else {
    oldImplNode = findRccImplementation (root);
  }

  if (oldImplNode) {
    /*
     * Remove this "implementation" node and add an empty one in its
     * place.
     */

    size_t off = oldImplNode->off;
    ezxml_cut (oldImplNode);
    implNode = ezxml_add_child (root, "implementation", off);
  }
  else {
    /*
     * Add a new implementation element.  We need to find a good
     * offset for it first.  It must be added before any "usesdevice"
     * elements.
     */

    ezxml_t udNode = ezxml_child (root, "usesdevice");

    if (udNode) {
      /*
       * There is a "usesdevice" element. Add new text just before
       * it.
       */

      std::string txt = ezxml_txt (root);
      std::string insert = "\n    ";
      size_t insLen = insert.length ();
      size_t off = udNode->off;

      txt.insert (off, insert);
      ezxml_set_txt_d (root, txt.c_str());

      /*
       * Update the offsets of all "usesdevice" elements.
       */

      while (udNode) {
        udNode->off += insLen;
        udNode = ezxml_next (udNode);
      }

      /*
       * Insert new "implementation" node.
       */

      implNode = ezxml_add_child (root, "implementation", off);
    }
    else {
      /*
       * There is no "usesdevice" node.  Add new "implementation"
       * node at the end.
       */

      std::string txt = ezxml_txt (root);
      std::string insert = "    \n";
      size_t off = txt.length() + 4;

      txt.append (insert);
      ezxml_set_txt_d (root, txt.c_str());
      implNode = ezxml_add_child (root, "implementation", off);
    }
  }

  /*
   * implNode now points at an empty "implementation" element that
   * we can populate.
   */

  std::string implTxt = "\n    ";
  size_t implOff = 1;

  if (oldImplNode) {
    /*
     * Copy attributes.
     */

    ezxml_set_attr_d (implNode, "id",
                      ezxml_attr (oldImplNode, "id"));
    ezxml_set_attr_d (implNode, "aepcompliance",
                      ezxml_attr (oldImplNode, "aepcompliance"));

    /*
     * Copy description, propertyfile.
     */

    moveNode (implNode, oldImplNode, "description", implTxt, implOff, 8);
    moveNode (implNode, oldImplNode, "propertyfile", implTxt, implOff, 8);
  }
  else {
    if (implId.length()) {
      ezxml_set_attr_d (implNode, "id", implId.c_str());
    }
    else {
      OCPI::Util::UUID::BinaryUUID uuid = OCPI::Util::UUID::produceRandomUUID ();
      std::string dceUUID = "DCE:";
      dceUUID += OCPI::Util::UUID::binaryToHex (uuid);
      ezxml_set_attr_d (implNode, "id", dceUUID.c_str());
    }

    ezxml_set_attr (implNode, "aepcompliance", "aep_compliant");
  }

  /*
   * Create code element.
   */

  insertNode (implNode,
              createCodeElement (m_exeFileName, entrypoint),
              implTxt, implOff, 8);

  /*
   * Move compiler, programminglanguage, humanlanguage.
   */

  if (oldImplNode) {
    moveNode (implNode, oldImplNode, "compiler", implTxt, implOff, 8);
    moveNode (implNode, oldImplNode, "programminglanguage", implTxt, implOff, 8);
    moveNode (implNode, oldImplNode, "humanlanguage", implTxt, implOff, 8);
  }

  /*
   * Create runtime element.
   */

  ezxml_t runtimeNode = ezxml_new ("runtime");
  ezxml_set_attr (runtimeNode, "name", s_ocpiRuntimeRccName);

  insertNode (implNode,
              runtimeNode,
              implTxt, implOff, 8);

  /*
   * OS.
   */

  if (m_os.length()) {
    insertNode (implNode,
                createOsElement (m_os),
                implTxt, implOff, 8);
  }
  else if (oldImplNode) {
    moveNode (implNode, oldImplNode, "os", implTxt, implOff, 8);
  }

  /*
   * Processor.
   */

  if (m_processor.length()) {
    insertNode (implNode,
                createProcessorElement (m_processor),
                implTxt, implOff, 8);
  }
  else if (oldImplNode) {
    moveNode (implNode, oldImplNode, "processor", implTxt, implOff, 8);
  }

  /*
   * OCPI Container Type Dependency.
   */

  insertNode (implNode,
              createOCPIContainerDependencyElement (),
              implTxt, implOff, 8);

  /*
   * OCPI Device Id Dependency.
   */

  bool haveOcpiDeviceIdDependency;

  if (m_ocpiDeviceId != static_cast<unsigned int> (-1)) {
    haveOcpiDeviceIdDependency = true;
    insertNode (implNode,
                createOCPIDeviceIdDependencyElement (m_ocpiDeviceId),
                implTxt, implOff, 8);
  }
  else {
    haveOcpiDeviceIdDependency = false;
  }

  /*
   * Move over other dependencies.
   */

  if (oldImplNode) {
    ezxml_t depNode = ezxml_child (oldImplNode, "dependency");
    bool move;

    while (depNode) {
      ezxml_t prefNode = ezxml_child (depNode, "propertyref");
      move = true;

      if (prefNode) {
        const char * prefId = ezxml_attr (prefNode, "refid");

        if (prefId && std::strcmp (prefId, s_ocpiRuntimePropertyId) == 0) {
          move = false;
        }
        else if (prefId && std::strcmp (prefId, s_ocpiDeviceIdPropertyId) == 0 &&
                 haveOcpiDeviceIdDependency) {
          move = false;
        }
      }

      if (move) {
        ezxml_move (depNode, implNode, implOff+8);
        implTxt.insert (implOff, "        \n");
        implOff += 9;
      }
      else {
        ezxml_remove (depNode);
      }

      depNode = ezxml_child (oldImplNode, "dependency");
    }
  }

  /*
   * Move over usesdevice.
   */

  if (oldImplNode) {
    moveNode (implNode, oldImplNode, "usesdevice", implTxt, implOff, 8);
  }

  /*
   * Wrap up.
   */

  ezxml_set_txt_d (implNode, implTxt.c_str());
  ezxml_free (oldImplNode);
}

ezxml_t
OcpiRccSpdUpdater::
createSPDfromSCD (const std::string & scdFileName,
                  const std::string & entrypoint,
                  const std::string & implId)
  throw ()
{
  ezxml_t root = ezxml_new ("softpkg");
  std::string rootTxt = "\n";
  size_t rootOff = 1;

  OCPI::Util::UUID::BinaryUUID uuid = OCPI::Util::UUID::produceRandomUUID ();
  std::string dceUUID = "DCE:";
  dceUUID += OCPI::Util::UUID::binaryToHex (uuid);
  ezxml_set_attr_d (root, "id", dceUUID.c_str());
  ezxml_set_attr_d (root, "name", entrypoint.c_str());
  ezxml_set_attr (root, "type", "sca_compliant");

  /*
   * Create mandatory author element.
   */

  insertNode (root,
              ezxml_new ("author"),
              rootTxt, rootOff, 4);

  /*
   * Create descriptor element.  It points to the SCD file.
   */

  ezxml_t descNode = ezxml_new ("descriptor");
  std::string descTxt = "\n    ";
  size_t descOff = 1;

  ezxml_t lfNode = ezxml_add_child (descNode, "localfile", descOff+8);
  descTxt.insert (descOff, "        \n");
  descOff += 9;

  ezxml_set_attr (lfNode, "name", scdFileName.c_str());

  ezxml_set_txt_d (descNode, descTxt.c_str());
  insertNode (root, descNode,
              rootTxt, rootOff, 4);

  /*
   * Create implementation element.
   */

  ezxml_t implNode = ezxml_new ("implementation");
  std::string implTxt = "\n    ";
  size_t implOff = 1;

  if (implId.length()) {
    ezxml_set_attr_d (implNode, "id", implId.c_str());
  }
  else {
    OCPI::Util::UUID::BinaryUUID implUuid = OCPI::Util::UUID::produceRandomUUID ();
    std::string dceImplUUID = "DCE:";
    dceImplUUID += OCPI::Util::UUID::binaryToHex (implUuid);
    ezxml_set_attr_d (implNode, "id", dceImplUUID.c_str());
  }

  ezxml_set_attr (implNode, "aepcompliance", "aep_compliant");

  /*
   * Create code element.
   */

  insertNode (implNode,
              createCodeElement (m_exeFileName, entrypoint),
              implTxt, implOff, 8);

  /*
   * Create runtime element.
   */

  ezxml_t runtimeNode = ezxml_new ("runtime");
  ezxml_set_attr (runtimeNode, "name", s_ocpiRuntimeRccName);

  insertNode (implNode,
              runtimeNode,
              implTxt, implOff, 8);

  /*
   * OS.
   */

  if (m_os.length()) {
    insertNode (implNode,
                createOsElement (m_os),
                implTxt, implOff, 8);
  }

  /*
   * Processor.
   */

  if (m_processor.length()) {
    insertNode (implNode,
                createProcessorElement (m_processor),
                implTxt, implOff, 8);
  }

  /*
   * OCPI Container Type Dependency.
   */

  insertNode (implNode,
              createOCPIContainerDependencyElement (),
              implTxt, implOff, 8);

  /*
   * OCPI Device Id Dependency.
   */

  if (m_ocpiDeviceId != static_cast<unsigned int> (-1)) {
    insertNode (implNode,
                createOCPIDeviceIdDependencyElement (m_ocpiDeviceId),
                implTxt, implOff, 8);
  }

  /*
   * Attach implementation node.
   */

  ezxml_set_txt_d (implNode, implTxt.c_str());
  insertNode (root, implNode,
              rootTxt, rootOff, 4);

  /*
   * Wrap up.
   */

  ezxml_set_txt_d (root, rootTxt.c_str());
  return root;
}

ezxml_t
OcpiRccSpdUpdater::
findRccImplementation (ezxml_t root)
  throw ()
{
  std::string osName, osVersion;
  std::string::size_type colon;

  if ((colon = m_os.find (':')) != std::string::npos) {
    osName = m_os.substr (0, colon);
    osVersion = m_os.substr (colon + 1);
  }
  else {
    osName = m_os;
  }

  /*
   * Look for an implementation that we might want to update; it should at
   * least have a matching requirement for OCPIContainerType.  If OCPIDeviceId,
   * OS or processor was specified, then they should match as well.
   */

  unsigned int implDeviceId;
  bool haveContainerType;
  bool haveOS, haveProcessor;

  ezxml_t implNode = ezxml_child (root, "implementation");

  while (implNode) {
    implDeviceId = static_cast<unsigned int> (-1);
    haveContainerType = false;

    if (m_os.length()) {
      ezxml_t osNode = ezxml_child (implNode, "os");
      const char * spdOsName = osNode ? ezxml_attr (osNode, "name") : 0;
      const char * spdOsVersion = osNode ? ezxml_attr (osNode, "version") : 0;

      if (spdOsName && osName.length() && osName != spdOsName) {
        haveOS = false;
      }
      else if (spdOsVersion && osVersion.length() && osVersion != spdOsVersion) {
        haveOS = false;
      }
      else {
        haveOS = true;
      }
    }
    else {
      haveOS = true;
    }

    if (m_processor.length()) {
      ezxml_t procNode = ezxml_child (implNode, "processor");
      const char * spdProcName = procNode ? ezxml_attr (procNode, "name") : 0;

      if (spdProcName && m_processor != spdProcName) {
        haveProcessor = false;
      }
      else {
        haveProcessor = true;
      }
    }
    else {
      haveProcessor = true;
    }

    ezxml_t depNode = ezxml_child (implNode, "dependency");

    while (depNode) {
      ezxml_t prefNode = ezxml_child (depNode, "propertyref");

      if (prefNode) {
        const char * refid = ezxml_attr (prefNode, "refid");
        const char * value = ezxml_attr (prefNode, "value");

        if (!refid || !value) {
          continue;
        }

        if (std::strcmp (refid, s_ocpiRuntimePropertyId) == 0 &&
            std::strcmp (value, s_ocpiRuntimeRccName) == 0) {
          haveContainerType = true;
        }
        else if (std::strcmp (refid, s_ocpiDeviceIdPropertyId) == 0) {
          try {
            implDeviceId = OCPI::Util::Misc::stringToUnsigned (value);
          }
          catch (...) {
          }
        }
      }

      depNode = ezxml_next (depNode);
    }

    if (haveContainerType && haveOS && haveProcessor &&
        (implDeviceId == static_cast<unsigned int> (-1) ||
         implDeviceId == m_ocpiDeviceId)) {
      break;
    }

    implNode = ezxml_next (implNode);
  }

  return implNode;
}

ezxml_t
OcpiRccSpdUpdater::
findImplementation (ezxml_t root, const std::string & implId)
  throw ()
{
  ezxml_t implNode = ezxml_child (root, "implementation");

  while (implNode) {
    const char * id = ezxml_attr (implNode, "id");

    if (id && implId == id) {
      return implNode;
    }

    implNode = ezxml_next (implNode);
  }

  return 0;
}

void
OcpiRccSpdUpdater::
insertNode (ezxml_t newParent, ezxml_t newNode,
            std::string & parentTxt, size_t & parentOff,
            unsigned int indent)
  throw ()
{
  std::string sindent (indent, ' ');
  sindent += '\n';

  ezxml_insert (newNode, newParent, parentOff+indent);
  parentTxt.insert (parentOff, sindent);
  parentOff += indent+1;
}

void
OcpiRccSpdUpdater::
moveNode (ezxml_t newParent, ezxml_t oldParent, const char * name,
          std::string & parentTxt, size_t & parentOff,
          unsigned int indent)
  throw ()
{
  ezxml_t oldNode = ezxml_child (oldParent, name);

  if (oldNode) {
    std::string sindent (indent, ' ');
    sindent += '\n';

    while (oldNode) {
      ezxml_move (oldNode, newParent, parentOff+indent);
      parentTxt.insert (parentOff, sindent);
      parentOff += indent+1;
      oldNode = ezxml_child (oldParent, name);
    }
  }
}

ezxml_t
OcpiRccSpdUpdater::
createCodeElement (const std::string & exeFileName,
                   const std::string & entrypoint)
  throw ()
{
  ezxml_t codeNode = ezxml_new ("code");
  ezxml_set_attr (codeNode, "type", "SharedLibrary");

  std::string codeTxt = "\n        ";
  size_t codeOff = 1;

  ezxml_t lfNode = ezxml_add_child (codeNode, "localfile", codeOff+12);
  codeTxt.insert (codeOff, "            \n");
  codeOff += 13;

  ezxml_set_attr_d (lfNode, "name", exeFileName.c_str());

  ezxml_t epNode = ezxml_add_child (codeNode, "entrypoint", codeOff+12);
  codeTxt.insert (codeOff, "            \n");
  codeOff += 13;

  ezxml_set_txt_d (epNode, entrypoint.c_str());
  ezxml_set_txt_d (codeNode, codeTxt.c_str());

  return codeNode;
}

ezxml_t
OcpiRccSpdUpdater::
createOsElement (const std::string & os)
  throw ()
{
  std::string osName, osVersion;
  std::string::size_type colon;

  if ((colon = os.find (':')) != std::string::npos) {
    osName = os.substr (0, colon);
    osVersion = os.substr (colon + 1);
  }
  else {
    osName = os;
  }

  ezxml_t osNode = ezxml_new ("os");
  ezxml_set_attr_d (osNode, "name", osName.c_str());

  if (osVersion.length()) {
    ezxml_set_attr_d (osNode, "version", osVersion.c_str());
  }

  return osNode;
}

ezxml_t
OcpiRccSpdUpdater::
createProcessorElement (const std::string & processor)
  throw ()
{
  ezxml_t procNode = ezxml_new ("processor");
  ezxml_set_attr_d (procNode, "name", processor.c_str());
  return procNode;
}

ezxml_t
OcpiRccSpdUpdater::
createOCPIContainerDependencyElement ()
  throw ()
{
  ezxml_t ctdNode = ezxml_new ("dependency");

  ezxml_set_attr (ctdNode, "type", "OCPI Runtime");
  ezxml_set_txt (ctdNode, "\n            \n        ");
  ezxml_t ctdPrefNode = ezxml_add_child (ctdNode, "propertyref", 13);
  ezxml_set_attr (ctdPrefNode, "refid", s_ocpiRuntimePropertyId);
  ezxml_set_attr (ctdPrefNode, "value", s_ocpiRuntimeRccName);

  return ctdNode;
}

ezxml_t
OcpiRccSpdUpdater::
createOCPIDeviceIdDependencyElement (unsigned int deviceId)
  throw ()
{
  ezxml_t didNode = ezxml_new ("dependency");

  ezxml_set_attr (didNode, "type", "OCPI Device Id");
  ezxml_set_txt (didNode, "\n            \n        ");
  ezxml_t didPrefNode = ezxml_add_child (didNode, "propertyref", 13);
  ezxml_set_attr (didPrefNode, "refid", s_ocpiDeviceIdPropertyId);

  std::string tmp = OCPI::Util::Misc::unsignedToString (deviceId);
  ezxml_set_attr_d (didPrefNode, "value", tmp.c_str());

  return didNode;
}

/*
 * ----------------------------------------------------------------------
 * Helpers.
 * ----------------------------------------------------------------------
 */

bool
findImplementation (ezxml_t root, const std::string & implId)
  throw ()
{
  ezxml_t implNode = ezxml_child (root, "implementation");

  while (implNode) {
    const char * id = ezxml_attr (implNode, "id");

    if (id && implId == id) {
      return true;
    }

    implNode = ezxml_next (implNode);
  }

  return false;
}

/*
 * ----------------------------------------------------------------------
 * Main.
 * ----------------------------------------------------------------------
 */

static
bool
ocpiRccBinderInt (int argc, char * argv[])
{
  OcpiRccBinderConfigurator config;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: " << oops << std::endl;
    return false;
  }

  if (config.help || argc != 1) {
    printUsage (config, argv[0]);
    return false;
  }

  if (!config.workerDllName.length() ||
      !config.outputFileName.length() ||
      !config.workerScdMap.size()) {
    printUsage (config, argv[0]);
    return false;
  }

  /*
   * Construct a FileFs for file access.
   */

  OCPI::Util::FileFs::FileFs fileFs ("/");

  /*
   * Collect executable information.
   */

  ExecutableInfo executableInfo;

  /*
   * Read all worker SCD files.
   */

  OCPI::Util::CommandLineConfiguration::MultiNameValue::iterator wit;

  for (wit = config.workerScdMap.begin();
       wit != config.workerScdMap.end();
       wit++) {
    /*
     * Worker entrypoint.
     */

    const std::string & workerEntrypoint = (*wit).first;

    /*
     * Collect worker information here.
     */

    WorkerInfo & workerInfo = executableInfo.workerInfo[workerEntrypoint];

    /*
     * The value is file[:uuid].  If file is an SPD file, then the UUID may
     * identify an implementation.  We want to process that implementation's
     * property file, and later this is the implementation that we want to
     * update.
     */

    const std::string & value = (*wit).second;
    std::string::size_type colon;

    if ((colon = value.find (':')) != std::string::npos) {
      workerInfo.relFileName = value.substr (0, colon);
      workerInfo.implementation = value.substr (colon + 1);
    }
    else {
      workerInfo.relFileName = value;
    }

    if (config.verbose) {
      std::cout << "Determining file type of \""
                << workerInfo.relFileName
                << "\" ... "
                << std::flush;
    }

    /*
     * Figure out the name of the file in the file system.
     */

    try {
      workerInfo.fileName = fileFs.fromNativeName (workerInfo.relFileName);
    }
    catch (const std::string & oops) {
      if (config.verbose) {
        std::cout << "failed." << std::endl;
      }
      std::cerr << "Oops: \"" << workerInfo.relFileName << "\": " << oops << "." << std::endl;
      return false;
    }

    /*
     * Parse file.
     */

    ezxml_t root;

    try {
      root = workerInfo.doc->parse (fileFs, workerInfo.fileName);
    }
    catch (const std::string & oops) {
      if (config.verbose) {
        std::cout << "failed." << std::endl;
      }
      std::cerr << "Oops: " << oops << "." << std::endl;
      return false;
    }

    const char * type = ezxml_name (root);
    ocpiAssert (root && type);

    if (config.verbose) {
      if (std::strcmp (type, "softpkg") == 0) {
        std::cout << "SPD." << std::endl;
      }
      else if (std::strcmp (type, "softwarecomponent") == 0) {
        std::cout << "SCD." << std::endl;
      }
      else if (std::strcmp (type, "properties") == 0) {
        std::cout << "PRF." << std::endl;
      }
      else {
        std::cout << type << std::endl;
      }
    }

    if (config.verbose) {
      std::cout << "Processing \""
                << workerInfo.relFileName
                << "\" ... "
                << std::flush;
    }

    OCPI::SCA::PropertyParser props;

    try {
      if (std::strcmp (type, "softpkg") == 0) {
        std::string implUUID = workerInfo.implementation;

        if (implUUID.length()) {
          if (config.updateSPD && !findImplementation (root, workerInfo.implementation)) {
            // we will create this implementation later
            implUUID.clear ();
          }
          else {
            std::string errMsg = "Implementation \"";
            errMsg += implUUID;
            errMsg += "\" not found in \"";
            errMsg += workerInfo.relFileName;
            errMsg += "\"";
          }
        }

        props.processSPD (fileFs, workerInfo.fileName, implUUID, root);
      }
      else if (std::strcmp (type, "softwarecomponent") == 0) {
        if (workerInfo.implementation.length() && !config.updateSPD) {
          std::cerr << "Warning: SCD file provided.  Ignoring implementation identifier."
                    << std::endl;
        }

        props.processSCD (fileFs, workerInfo.fileName, root);
      }
      else if (std::strcmp (type, "properties") == 0) {
        if (workerInfo.implementation.length()) {
          std::cerr << "Warning: SCD file provided.  Ignoring implementation identifier."
                    << std::endl;
        }

        props.processPRF (root);
      }
      else {
        std::string errMsg = "Input file \"";
        errMsg += workerInfo.relFileName;
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

    workerInfo.propertyMagic = props.encode ();
  }

  /*
   * Check executable file name for existence before creating the output file.
   * (It might still disappear between now and below, of course.)
   */

  std::string executableFileName;
  std::string executableTail;

  try {
    executableFileName = fileFs.fromNativeName (config.workerDllName);
    executableTail = OCPI::Util::Vfs::relativeName (executableFileName);

    if (!fileFs.exists (executableFileName)) {
      throw std::string ("File not found");
    }
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: \"" << config.workerDllName << "\": " << oops << "." << std::endl;
    return false;
  }

  /*
   * Check dependent DLLs for existence also.
   */

  OCPI::Util::CommandLineConfiguration::MultiString::iterator dit;

  for (dit = config.dependentDlls.begin();
       dit != config.dependentDlls.end();
       dit++) {
    std::string depFileName;
    std::string depTail;

    try {
      depFileName = fileFs.fromNativeName (*dit);
      depTail = OCPI::Util::Vfs::relativeName (depFileName);

      if (!fileFs.exists (depFileName)) {
        throw std::string ("File not found");
      }

      executableInfo.dependency.push_back (DependencyInfos::value_type (depTail, depFileName));
    }
    catch (const std::string & oops) {
      std::cerr << "Oops: \"" << (*dit) << "\": " << oops << "." << std::endl;
      return false;
    }
  }

  /*
   * Create output ZIP file.
   */

  OCPI::Util::ZipFs::ZipFs outputFile;
  std::string zipFileName;

  try {
    zipFileName = fileFs.fromNativeName (config.outputFileName);
    outputFile.openZip (&fileFs, zipFileName,
                        std::ios_base::out | std::ios_base::trunc,
                        false, true);
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: \"" << config.outputFileName << "\": " << oops << "." << std::endl;
    return false;
  }

  try {
    /*
     * Create ocpi-meta.inf file.
     */

    if (config.verbose) {
      std::cout << "Writing meta-data file ... " << std::flush;
    }

    std::ostream * meta = outputFile.openWriteonly ("/ocpi-meta.inf");

    *meta << "<?xml version='1.0' encoding='ISO-8859-1'?>" << std::endl;
    *meta << "<cpx:rcc-executable>" << std::endl;
    *meta << "  <file>" << executableTail << "</file>" << std::endl;

    for (WorkerInfos::iterator wit = executableInfo.workerInfo.begin ();
         wit != executableInfo.workerInfo.end(); wit++) {
      const std::string & entrypointName = (*wit).first;
      const WorkerInfo & workerInfo = (*wit).second;
      *meta << "  <worker>" << std::endl;
      *meta << "    <entrypoint>" << entrypointName << "</entrypoint>" << std::endl;
      *meta << "    <properties>" << workerInfo.propertyMagic << "</properties>" << std::endl;
      *meta << "  </worker>" << std::endl;
    }

    /*
     * List of DLLs.
     */

    for (DependencyInfos::iterator dit = executableInfo.dependency.begin ();
         dit != executableInfo.dependency.end (); dit++) {
      const std::string & dllTail = (*dit).first;
      *meta << "  <dependency>" << dllTail << "</dependency>" << std::endl;
    }

    *meta << "</cpx:rcc-executable>" << std::endl;

    if (!meta->good()) {
      try {
        outputFile.close (meta);
      }
      catch (...) {
      }

      throw std::string ("Error writing meta-data.");
    }

    outputFile.close (meta);

    if (config.verbose) {
      std::cout << "done." << std::endl;
    }

    /*
     * Copy executable file into the Zip.
     */

    if (config.verbose) {
      std::cout << "Copying executable file \""
                << config.workerDllName
                << "\" ... "
                << std::flush;
    }

    fileFs.copy (executableFileName,
                 &outputFile,
                 executableTail);

    if (config.verbose) {
      std::cout << "done." << std::endl;
    }

    /*
     * Copy DLL files into the Zip.
     */

    for (DependencyInfos::iterator dit = executableInfo.dependency.begin ();
         dit != executableInfo.dependency.end (); dit++) {
      const std::string & dllTail = (*dit).first;
      const std::string & dllName = (*dit).second;

      if (config.verbose) {
        std::cout << "Copying DLL file \""
                  << dllName
                  << "\" ... "
                  << std::flush;
      }

      fileFs.copy (dllName,
                   &outputFile,
                   dllTail);

      if (config.verbose) {
        std::cout << "done." << std::endl;
      }
    }
  }
  catch (const std::string & oops) {
    if (config.verbose) {
      std::cout << "failed." << std::endl;
    }

    std::cerr << "Oops: " << oops << "." << std::endl;

    try {
      outputFile.closeZip ();
    }
    catch (...) {
    }

    try {
      fileFs.remove (zipFileName);
    }
    catch (...) {
    }

    return false;
  }

  /*
   * Flush output file.
   */

  try {
    outputFile.closeZip ();
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: " << oops << "." << std::endl;

    try {
      fileFs.remove (zipFileName);
    }
    catch (...) {
    }

    return false;
  }

  /*
   * Update SPD files, if desired.
   */

  if (config.updateSPD) {
    OcpiRccSpdUpdater crsu (config.outputFileName,
                           config.os,
                           config.processor,
                           static_cast<unsigned int> (config.ocpiDeviceId));

    for (WorkerInfos::iterator wit = executableInfo.workerInfo.begin ();
         wit != executableInfo.workerInfo.end(); wit++) {
      const std::string & workerEntrypoint = (*wit).first;
      const WorkerInfo & workerInfo = (*wit).second;

      /*
       * Our action depends on the type of file that the user specified.
       */

      ezxml_t root = workerInfo.doc->getRootNode ();
      const char * type = ezxml_name (root);
      ocpiAssert (root && type);

      if (std::strcmp (type, "softpkg") == 0) {
        /*
         * We are processing an existing SPD.  Update it.
         */

        if (config.verbose) {
          std::cout << "Updating \""
                    << workerInfo.relFileName
                    << "\" ... "
                    << std::flush;
        }

        try {
          crsu.updateSPD (root, workerEntrypoint, workerInfo.implementation);

          /*
           * Write the new SPD file.
           */

          std::ostream * out =
            fileFs.openWriteonly (workerInfo.fileName,
                                  std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

          try {
            char * newSPD = ezxml_toxml (root);
            *out << "<?xml version=\"1.0\" encoding=\"us-ascii\"?>" << std::endl;
            *out << "<!DOCTYPE softpkg SYSTEM \"" << config.spdDtdName << "\">" << std::endl;
            *out << newSPD << std::endl;
            free (newSPD);
          }
          catch (...) {
            try {
              fileFs.close (out);
            }
            catch (...) {
            }
            throw;
          }

          fileFs.close (out);
        }
        catch (const std::string & oops) {
          if (config.verbose) {
            std::cout << "failed." << std::endl;
          }

          std::cerr << "Oops: \""
                    << workerInfo.fileName
                    << "\": "
                    << oops
                    << "."
                    << std::endl;

          return false;
        }

        if (config.verbose) {
          std::cout << "done." << std::endl;
        }
      }
      else if (std::strcmp (type, "softwarecomponent") == 0) {
        /*
         * We are processing an SCD.  Create a new SPD file using the SCD
         * file's base name.  It should have the ".scd.xml" extension.
         */

        std::string relFileName;
        std::string fileName;
        std::string::size_type rfnl = workerInfo.relFileName.length ();
        std::string::size_type fnl = workerInfo.fileName.length ();

        if (rfnl > 8 && workerInfo.relFileName.substr (rfnl-8) == ".scd.xml") {
          ocpiAssert (fnl > 8);
          ocpiAssert (workerInfo.fileName.substr (fnl-8) == ".scd.xml");

          relFileName  = workerInfo.relFileName.substr (0, rfnl-8);
          relFileName += ".spd.xml";

          fileName  = workerInfo.fileName.substr (0, fnl-8);
          fileName += ".spd.xml";
        }
        else if (rfnl > 4 &&
                 (workerInfo.relFileName.substr (rfnl-4) == ".scd" ||
                  workerInfo.relFileName.substr (rfnl-4) == ".xml")) {
          ocpiAssert (fnl > 4);
          ocpiAssert (workerInfo.fileName.substr (fnl-4) == workerInfo.relFileName.substr (rfnl-4));

          relFileName  = workerInfo.relFileName.substr (0, rfnl-4);
          relFileName += ".spd.xml";

          fileName  = workerInfo.fileName.substr (0, fnl-4);
          fileName += ".spd.xml";
        }

        if (config.verbose) {
          std::cout << "Creating \""
                    << relFileName
                    << "\" ... "
                    << std::flush;
        }

        try {
          ezxml_t spdRoot =
            crsu.createSPDfromSCD (workerInfo.relFileName,
                                   workerEntrypoint,
                                   workerInfo.implementation);

          try {
            /*
             * Write the new SPD file.
             */

            std::ostream * out =
              fileFs.openWriteonly (fileName,
                                    std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

            try {
              char * newSPD = ezxml_toxml (spdRoot);
              *out << "<?xml version=\"1.0\" encoding=\"us-ascii\"?>" << std::endl;
              *out << "<!DOCTYPE softpkg SYSTEM \"" << config.spdDtdName << "\">" << std::endl;
              *out << newSPD << std::endl;
              free (newSPD);
            }
            catch (...) {
              try {
                fileFs.close (out);
              }
              catch (...) {
              }
              throw;
            }

            fileFs.close (out);
          }
          catch (...) {
            ezxml_free (spdRoot);
            throw;
          }

          ezxml_free (spdRoot);
        }
        catch (const std::string & oops) {
          if (config.verbose) {
            std::cout << "failed." << std::endl;
          }

          std::cerr << "Oops: \""
                    << workerInfo.fileName
                    << "\": "
                    << oops
                    << "."
                    << std::endl;

          return false;
        }

        if (config.verbose) {
          std::cout << "done." << std::endl;
        }
      }
      else if (std::strcmp (type, "properties") == 0) {
        /*
         * We are processing a property file.
         *
         * We can't create an SPD without an SCD, so we complain.
         *
         * We could create a vanilla SCD (describing a component that
         * implements CF::Resource and has no ports), but that's probably
         * a bad idea.
         */

        std::cerr << "Oops: \""
                  << workerInfo.relFileName
                  << "\" is a PRF file.  Can not update SPD."
                  << std::endl;

        return false;
      }
      else {
        /*
         * Should not get here due to code above.
         */

        ocpiAssert (0);
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
  ocpiRccBinder (int argc, char * argv[])
  {
    return ocpiRccBinderInt (argc, argv) ? 0 : -1;
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

  return ocpiRccBinderInt (argc, argv) ? 0 : 1;
}

