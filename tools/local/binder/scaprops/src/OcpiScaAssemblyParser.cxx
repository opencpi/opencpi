
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
 * Parse SCA SAD files.
 *
 * Revision History:
 *
 *     03/02/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <cstring>
#include <cstdlib>
#include <string>
#include <ezxml.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilUri.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilMisc.h>
#include <OcpiUtilEzxml.h>
#include <sca_props.h>
#include "OcpiScaAssemblyParser.h"

OCPI::SCA::AssemblyParser::
AssemblyParser ()
  throw ()
{
}

OCPI::SCA::AssemblyParser::
AssemblyParser (OCPI::Util::Vfs::Vfs & fs,
                const std::string & fileName)
  throw (std::string)
{
  parse (fs, fileName);
}

OCPI::SCA::AssemblyParser::
~AssemblyParser ()
  throw ()
{
}

void
OCPI::SCA::AssemblyParser::
parse (OCPI::Util::Vfs::Vfs & fs,
       const std::string & fileName)
  throw (std::string)
{
  /*
   * Parse that file.
   */

  OCPI::Util::EzXml::Doc doc;
  ezxml_t root;

  try {
    root = doc.parse (fs, fileName);
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

  if (std::strcmp (type, "softwareassembly") == 0) {
    processSAD (fs, fileName, root);
  }
  else {
    std::string errMsg = "Input file \"";
    errMsg += fileName;
    errMsg += "\" is not a Software Assembly Descriptor, but a \"";
    errMsg += type;
    errMsg += "\" file";
    throw errMsg;
  }
}

void
OCPI::SCA::AssemblyParser::
processSAD (OCPI::Util::Vfs::Vfs & fs,
            const std::string & scdFileName,
            ezxml_t sadRoot)
  throw (std::string)
{
  ( void ) fs;
  ( void ) scdFileName;
  ocpiAssert (sadRoot && ezxml_name (sadRoot));
  ocpiAssert (std::strcmp (sadRoot->name, "softwareassembly") == 0);

  const char * idAttr = ezxml_attr (sadRoot, "id");

  if (!idAttr || !*idAttr) {
    throw std::string ("Missing \"id\" attribute");
  }

  m_assembly.id = idAttr;

  const char * nameAttr = ezxml_attr (sadRoot, "name");
  m_assembly.name = nameAttr ? nameAttr : "";

  const char * versionAttr = ezxml_attr (sadRoot, "version");
  m_assembly.version = versionAttr ? versionAttr : "";

  /*
   * Parse "description".
   */

  ezxml_t descNode = ezxml_child (sadRoot, "description");

  if (descNode) {
    const char * description = ezxml_txt (descNode);
    m_assembly.description = description ? description : "";
  }

  /*
   * Parse "componentfiles".
   */

  {
    ezxml_t cfsNode = ezxml_child (sadRoot, "componentfiles");

    if (!cfsNode) {
      throw std::string ("Missing \"componentfiles\" element");
    }

    /*
     * There must be at least one "componentfile" element.
     */

    ezxml_t cfNode = ezxml_child (cfsNode, "componentfile");

    if (!cfNode) {
      throw std::string ("Missing \"componentfile\" element");
    }

    while (cfNode) {
      const char * cfId = ezxml_attr (cfNode, "id");

      if (!cfId || !*cfId) {
        throw std::string ("Missing \"id\" attribute for \"componentfile\" element");
      }

      try {
        parseComponentFileNode (cfNode, cfId);
      }
      catch (const std::string & oops) {
        std::string msg = oops;
        msg += " while parsing componentfile \"";
        msg + cfId;
        msg += "\"";
        throw msg;
      }

      cfNode = ezxml_next (cfNode);
    }
  }

  /*
   * Parse "partitioning".
   */

  {
    ezxml_t partNode = ezxml_child (sadRoot, "partitioning");

    if (!partNode) {
      throw std::string ("Missing \"partitioning\" element");
    }

    ezxml_t cpNode = ezxml_child (partNode, "componentplacement");

    while (cpNode) {
      try {
        parseComponentPlacement (cpNode, m_assembly.componentPlacement);
      }
      catch (const std::string & oops) {
        std::string msg = oops;
        msg += " while parsing componentplacement";
        throw msg;
      }

      cpNode = ezxml_next (cpNode);
    }

    /*
     * Count number of hostcollocation elements.
     */

    unsigned int hcCount = 0;
    ezxml_t hcNode;

    for (hcNode = ezxml_child (partNode, "hostcollocation");
         hcNode; hcNode = ezxml_next (hcNode)) {
      hcCount++;
    }

    m_assembly.hostCollocation.resize (hcCount);

    /*
     * Process hostcollocation elements.
     */

    hcNode = ezxml_child (partNode, "hostcollocation");
    unsigned int hcIdx = 0;

    while (hcNode) {
      ocpiAssert (hcIdx < hcCount);

      try {
        parseHostCollocation (hcNode, hcIdx);
      }
      catch (const std::string & oops) {
        std::string msg = oops;
        msg += " while parsing hostcollocation[";
        msg + OCPI::Util::Misc::unsignedToString (hcIdx);
        msg += "]";
        throw msg;
      }

      hcNode = ezxml_next (hcNode);
      hcIdx++;
    }
  }

  /*
   * Parse "assemblycontroller".
   */

  {
    ezxml_t acNode = ezxml_child (sadRoot, "assemblycontroller");

    if (!acNode) {
      throw std::string ("Missing \"assemblycontroller\" element");
    }

    ezxml_t acCidRef = ezxml_child (acNode, "componentinstantiationref");

    if (!acCidRef) {
      throw std::string ("Missing \"componentinstantiationref\" element for assembly controller");
    }

    const char * refId = ezxml_attr (acCidRef, "refid");

    if (!refId || !*refId) {
      throw std::string ("Missing \"refid\" attribute for assembly controller");
    }

    if (m_assembly.allComponents.find (refId) == m_assembly.allComponents.end()) {
      std::string msg = "Assembly controller \"";
      msg += refId;
      msg += "\" not found";
      throw msg;
    }

    m_assembly.assemblyController = refId;
  }

  /*
   * Parse "connections".
   */

  ezxml_t connectionsNode = ezxml_child (sadRoot, "connections");

  if (connectionsNode) {
    /*
     * Count number of connection elements.
     */

    unsigned int ciCount = 0;
    ezxml_t ciNode;

    for (ciNode = ezxml_child (connectionsNode, "connectinterface");
         ciNode; ciNode = ezxml_next (ciNode)) {
      ciCount++;
    }

    m_assembly.connection.resize (ciCount);

    /*
     * Process connection elements.
     */

    ciNode = ezxml_child (connectionsNode, "connectinterface");
    unsigned int ciIdx = 0;

    while (ciNode) {
      ocpiAssert (ciIdx < ciCount);

      try {
        parseConnection (ciNode, ciIdx);
      }
      catch (const std::string & oops) {
        std::string msg = "While parsing connection[";
        msg += OCPI::Util::Misc::unsignedToString (ciIdx);
        msg += "]";
        throw msg;
      }

      ciNode = ezxml_next (ciNode);
      ciIdx++;
    }
  }

  /*
   * Parse "externalports".
   */

  ezxml_t epsNode = ezxml_child (sadRoot, "externalports");

  if (epsNode) {
    /*
     * Count number of external ports.
     */

    unsigned int epCount = 0;
    ezxml_t epNode;

    for (epNode = ezxml_child (epsNode, "port");
         epNode; epNode = ezxml_next (epNode)) {
      epCount++;
    }

    m_assembly.externalPort.resize (epCount);

    /*
     * Process port elements.
     */

    epNode = ezxml_child (epsNode, "port");
    unsigned int epIdx = 0;

    while (epNode) {
      ocpiAssert (epIdx < epCount);

      try {
        parseExternalPort (epNode, epIdx);
      }
      catch (const std::string & oops) {
        std::string msg = "While parsing port[";
        msg += OCPI::Util::Misc::unsignedToString (epIdx);
        msg += "]";
        throw msg;
      }

      epNode = ezxml_next (epNode);
      epIdx++;
    }
  }
}

void
OCPI::SCA::AssemblyParser::
parseComponentFileNode (ezxml_t cfNode, const char * id)
  throw (std::string)
{
  ezxml_t lfNode = ezxml_child (cfNode, "localfile");

  if (!lfNode) {
    throw std::string ("\"localfile\" element missing");
  }

  const char * lfName = ezxml_attr (lfNode, "name");

  if (!lfName || !*lfName) {
    throw std::string ("\"localfile\" element lacks \"name\" attribute");
  }

  StringStringMap::iterator it = m_assembly.componentFiles.find (id);

  if (it != m_assembly.componentFiles.end()) {
    throw std::string ("Duplicate \"componentfile\" identifier");
  }

  m_assembly.componentFiles[id] = lfName;
}

void
OCPI::SCA::AssemblyParser::
parseHostCollocation (ezxml_t hcNode, unsigned int hcIdx)
  throw (std::string)
{
  HostCollocation & hc = m_assembly.hostCollocation[hcIdx];

  const char * id = ezxml_attr (hcNode, "id");
  hc.id = id ? id : "";

  const char * name = ezxml_attr (hcNode, "name");
  hc.name = name ? name : "";

  ezxml_t cpNode = ezxml_child (hcNode, "componentplacement");

  if (!cpNode) {
    throw std::string ("Missing \"componentplacement\" element");
  }

  unsigned int cpIdx = 0;

  while (cpNode) {
    try {
      parseComponentPlacement (cpNode,
                               hc.componentPlacement,
                               hcIdx);
    }
    catch (const std::string & oops) {
      std::string msg = oops;
      msg += " while parsing componentplacement[";
      msg += OCPI::Util::Misc::unsignedToString (cpIdx);
      msg += "]";
      throw msg;
    }

    cpNode = ezxml_next (cpNode);
    cpIdx++;
  }
}

void
OCPI::SCA::AssemblyParser::
parseComponentPlacement (ezxml_t cpNode,
                         ComponentPlacements & cps,
                         unsigned int hcIdx)
  throw (std::string)
{
  ezxml_t cfrNode = ezxml_child (cpNode, "componentfileref");

  if (!cfrNode) {
    throw std::string ("Missing \"componentfileref\" element");
  }

  const char * refId = ezxml_attr (cfrNode, "refid");

  if (!refId || !*refId) {
    throw std::string ("Missing \"componentfileref\" attribute");
  }

  std::string fileRef = refId;
  StringStringMap::const_iterator cfit = m_assembly.componentFiles.find (fileRef);

  if (cfit == m_assembly.componentFiles.end()) {
    std::string msg = "Component file reference \"";
    msg += fileRef;
    msg += "\" not found";
    throw msg;
  }

  const std::string & spdFileName = (*cfit).second;

  ezxml_t ciNode = ezxml_child (cpNode, "componentinstantiation");

  if (!ciNode) {
    throw std::string ("Missing \"componentinstantiation\" element");
  }

  while (ciNode) {
    const char * ciId = ezxml_attr (ciNode, "id");

    if (!ciId || !*ciId) {
      throw std::string ("Missing \"id\" attribute for \"componentinstantiation\" element");
    }

    std::string instanceId = ciId;
    StringUIntMap::iterator acit = m_assembly.allComponents.find (instanceId);

    if (acit != m_assembly.allComponents.end()) {
      std::string msg = "Duplicate component instantiation id \"";
      msg += instanceId;
      msg += "\"";
      throw msg;
    }

    ComponentPlacement & cp = cps[instanceId];
    cp.fileRef = fileRef;
    cp.spdFileName = spdFileName;

    ezxml_t usageNameNode = ezxml_child (ciNode, "usagename");

    if (usageNameNode) {
      const char * usageName = ezxml_txt (usageNameNode);

      if (usageName) {
        cp.usageName = usageName;
      }
    }

    m_assembly.allComponents[instanceId] = hcIdx;
    ciNode = ezxml_next (ciNode);
  }
}

void
OCPI::SCA::AssemblyParser::
parseConnection (ezxml_t ciNode, unsigned int ciIdx)
  throw (std::string)
{
  Connection & conn = m_assembly.connection[ciIdx];

  const char * connId = ezxml_attr (ciNode, "id");
  conn.id = connId ? connId : "";

  ezxml_t upNode = ezxml_child (ciNode, "usesport");

  if (!upNode) {
    throw std::string ("Missing \"usesport\" element");
  }

  try {
    parsePortRef (upNode, conn.usesPort, false);
  }
  catch (const std::string & oops) {
    std::string msg = oops;
    msg += " while parsing usesport element";
    throw msg;
  }

  ezxml_t subNode;

  if ((subNode = ezxml_child (ciNode, "providesport"))) {
    conn.type = Connection::PROVIDESPORT;

    try {
      parsePortRef (subNode, conn.providesPort, true);
    }
    catch (const std::string & oops) {
      std::string msg = oops;
      msg += " while parsing providesport element";
      throw msg;
    }
  }
  else if ((subNode = ezxml_child (ciNode, "componentsupportedinterface"))) {
    conn.type = Connection::COMPONENTSUPPORTEDINTERFACE;

    try {
      parseSupportedInterfaceRef (subNode, conn.supportedInterface);
    }
    catch (const std::string & oops) {
      std::string msg = oops;
      msg += " while parsing supportedinterface element";
      throw msg;
    }
  }
  else if ((subNode = ezxml_child (ciNode, "findby"))) {
    conn.type = Connection::FINDBY;

    try {
      parseFindBy (subNode, conn.findBy);
    }
    catch (const std::string & oops) {
      std::string msg = oops;
      msg += " while parsing findby element";
      throw msg;
    }
  }
  else {
    throw std::string ("Missing provider");
  }
}

void
OCPI::SCA::AssemblyParser::
parsePortRef (ezxml_t portNode,
              PortRef & portRef,
              bool isProvides)
  throw (std::string)
{
  const char * pnnName = isProvides ? "providesidentifier" : "usesidentifier";
  ezxml_t portNameNode = ezxml_child (portNode, pnnName);

  if (!portNameNode) {
    std::string msg = "Missing \"";
    msg += pnnName;
    msg += "\" element";
    throw msg;
  }

  const char * portName = ezxml_txt (portNameNode);

  if (!portName || !*portName) {
    std::string msg = "Empty \"";
    msg += pnnName;
    msg += "\" element";
    throw msg;
  }

  portRef.portName = portName;

  ezxml_t subNode;

  if ((subNode = ezxml_child (portNode, "componentinstantiationref"))) {
    const char * refId = ezxml_attr (subNode, "refid");

    if (!refId || !*refId) {
      throw std::string ("Missing \"refid\" attribute for component instantiation reference");
    }

    if (m_assembly.allComponents.find (refId) == m_assembly.allComponents.end()) {
      std::string msg = "Component instantiation \"";
      msg += refId;
      msg += "\" not found";
      throw msg;
    }

    portRef.type = PortRef::COMPONENTINSTANTIATIONREF;
    portRef.refId = refId;
  }
  else if ((subNode = ezxml_child (portNode, "devicethatloadedthiscomponentref"))) {
    const char * refId = ezxml_attr (subNode, "refid");

    if (!refId || !*refId) {
      throw std::string ("Missing \"refid\" attribute for device that loaded this component reference");
    }

    if (m_assembly.allComponents.find (refId) == m_assembly.allComponents.end()) {
      std::string msg = "Component instantiation \"";
      msg += refId;
      msg += "\" not found";
      throw msg;
    }

    portRef.type = PortRef::DEVICETHATLOADEDTHISCOMPONENTREF;
    portRef.refId = refId;
  }
  else if ((subNode = ezxml_child (portNode, "deviceusedbythiscomponentref"))) {
    const char * refId = ezxml_attr (subNode, "refid");

    if (!refId || !*refId) {
      throw std::string ("Missing \"refid\" attribute for device used by this component reference");
    }

    if (m_assembly.allComponents.find (refId) == m_assembly.allComponents.end()) {
      std::string msg = "Component instantiation \"";
      msg += refId;
      msg += "\" not found";
      throw msg;
    }

    const char * usesRefId = ezxml_attr (subNode, "usesrefid");

    if (!usesRefId || !*usesRefId) {
      throw std::string ("Missing \"usesrefid\" attribute for device used by this component reference");
    }

    portRef.type = PortRef::DEVICEUSEDBYTHISCOMPONENTREF;
    portRef.refId = refId;
    portRef.usesRefId = usesRefId;
  }
  else if ((subNode = ezxml_child (portNode, "findby"))) {
    portRef.type = PortRef::FINDBY;

    try {
      parseFindBy (subNode, portRef.findBy);
    }
    catch (const std::string & oops) {
      std::string msg = oops;
      msg += " while parsing \"findby\" element";
      throw msg;
    }
  }
  else {
    throw std::string ("Missing component reference");
  }
}

void
OCPI::SCA::AssemblyParser::
parseSupportedInterfaceRef (ezxml_t csiNode,
                            SupportedInterfaceRef & supportedInterface)
  throw (std::string)
{
  ezxml_t siNode = ezxml_child (csiNode, "supportedidentifier");

  if (!siNode) {
    throw std::string ("Missing \"supportedidentifier\" element");
  }

  const char * supportedIdentifier = ezxml_txt (siNode);

  if (!supportedIdentifier || !*supportedIdentifier) {
    throw std::string ("Empty \"supportedidentifier\" element");
  }

  supportedInterface.supportedIdentifier = supportedIdentifier;

  ezxml_t subNode;

  if ((subNode = ezxml_child (csiNode, "componentinstantiationref"))) {
    const char * refId = ezxml_attr (subNode, "refid");

    if (!refId || !*refId) {
      throw std::string ("Missing \"refid\" attribute for component instantiation reference");
    }

    if (m_assembly.allComponents.find (refId) == m_assembly.allComponents.end()) {
      std::string msg = "Component instantiation \"";
      msg += refId;
      msg += "\" not found";
      throw msg;
    }

    supportedInterface.type = SupportedInterfaceRef::COMPONENTINSTANTIATIONREF;
    supportedInterface.refId = refId;
  }
  else if ((subNode = ezxml_child (csiNode, "findby"))) {
    supportedInterface.type = SupportedInterfaceRef::FINDBY;

    try {
      parseFindBy (subNode, supportedInterface.findBy);
    }
    catch (const std::string & oops) {
      std::string msg = oops;
      msg += " while parsing \"findby\" element";
      throw msg;
    }
  }
  else {
    throw std::string ("Missing component reference");
  }
}

void
OCPI::SCA::AssemblyParser::
parseFindBy (ezxml_t findByNode, FindBy & findBy)
  throw (std::string)
{
  ezxml_t subNode;

  if ((subNode = ezxml_child (findByNode, "namingservice"))) {
    const char * name = ezxml_attr (subNode, "name");

    if (!name || !*name) {
      throw std::string ("Empty naming service name");
    }

    findBy.type = FindBy::NAMINGSERVICE;
    findBy.name = name;
  }
  else if ((subNode = ezxml_child (findByNode, "domainfinder"))) {
    const char * type = ezxml_attr (subNode, "type");

    if (!type || !*type) {
      throw std::string ("Missing domainfinder type");
    }

    const char * name = ezxml_attr (subNode, "name");

    findBy.type = FindBy::DOMAINFINDER;
    findBy.name = name ? name : "";

    if (std::strcmp (type, "filemanager") == 0) {
      findBy.domainFinderType = FindBy::DF_FILEMANAGER;
    }
    else if (std::strcmp (type, "log") == 0) {
      findBy.domainFinderType = FindBy::DF_LOG;
    }
    else if (std::strcmp (type, "eventchannel") == 0) {
      findBy.domainFinderType = FindBy::DF_EVENTCHANNEL;
    }
    else if (std::strcmp (type, "namingservice") == 0) {
      findBy.domainFinderType = FindBy::DF_NAMINGSERVICE;
    }
    else {
      std::string msg = "Invalid domain finder type \"";
      msg += type;
      msg += "\"";
      throw msg;
    }
  }
  else {
    throw std::string ("Empty or invalid findby element");
  }
}

void
OCPI::SCA::AssemblyParser::
parseExternalPort (ezxml_t epNode, unsigned int epIdx)
  throw (std::string)
{
  ExternalPort & ep = m_assembly.externalPort[epIdx];

  ezxml_t descNode = ezxml_child (epNode, "description");

  if (descNode) {
    const char * description = ezxml_txt (descNode);
    ep.description = description ? description : "";
  }

  ezxml_t subNode;

  if ((subNode = ezxml_child (epNode, "usesidentifier"))) {
    const char * portName = ezxml_txt (subNode);

    if (!portName || !*portName) {
      throw std::string ("Empty usesidentifier");
    }

    ep.type = ExternalPort::USESIDENTIFIER;
    ep.portName = portName;
  }
  else if ((subNode = ezxml_child (epNode, "providesidentifier"))) {
    const char * portName = ezxml_txt (subNode);

    if (!portName || !*portName) {
      throw std::string ("Empty providesidentifier");
    }

    ep.type = ExternalPort::PROVIDESIDENTIFIER;
    ep.portName = portName;
  }
  else if ((subNode = ezxml_child (epNode, "supportedidentifier"))) {
    const char * portName = ezxml_txt (subNode);

    if (!portName || !*portName) {
      throw std::string ("Empty supportedidentifier");
    }

    ep.type = ExternalPort::SUPPORTEDIDENTIFIER;
    ep.portName = portName;
  }

  ezxml_t ciRefNode = ezxml_child (epNode, "componentinstantiationref");

  if (!ciRefNode) {
    throw std::string ("Missing \"componentinstantiationref\" element");
  }

  const char * refId = ezxml_attr (ciRefNode, "refid");

  if (!refId || !*refId) {
    throw std::string ("Missing \"refid\" attribute for component instantiation reference");
  }

  if (m_assembly.allComponents.find (refId) == m_assembly.allComponents.end()) {
    std::string msg = "Component instantiation \"";
    msg += refId;
    msg += "\" not found";
    throw msg;
  }

  ep.refId = refId;
}
