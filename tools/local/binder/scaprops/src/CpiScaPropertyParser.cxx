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
 * Parse SCA SPD, SCD and PRF files.
 *
 * Revision History:
 *
 *     06/25/2009 - Frank Pilhofer
 *                  Bugfix when handling sequences.
 *
 *     05/06/2009 - Frank Pilhofer
 *                  RCC updates (interpretation of max_string_size).
 *
 *     02/20/1009 - Frank Pilhofer
 *                  Parse test properties.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <cstring>
#include <cstdlib>
#include <string>
#include <ezxml.h>
#include <CpiOsAssert.h>
#include <CpiUtilUri.h>
#include <CpiUtilVfs.h>
#include <CpiUtilEzxml.h>
#include <sca_props.h>
#include "CpiScaPropertyParser.h"

CPI::SCA::PropertyParser::
PropertyParser ()
  throw ()
  : m_magicString (0),
    m_sizeOfPropertySpace (0),
    m_numProperties (0),
    m_properties (0),
    m_numPorts (0),
    m_ports (0),
    m_numTests (0),
    m_tests (0)
{
}

CPI::SCA::PropertyParser::
PropertyParser (CPI::Util::Vfs::Vfs & fs,
                const std::string & fileName)
  throw (std::string)
  : m_magicString (0),
    m_sizeOfPropertySpace (0),
    m_numProperties (0),
    m_properties (0),
    m_numPorts (0),
    m_ports (0),
    m_numTests (0),
    m_tests (0)
{
  try {
    parse (fs, fileName);
  }
  catch (...) {
    cleanup ();
    throw;
  }
}

CPI::SCA::PropertyParser::
~PropertyParser ()
  throw ()
{
  cleanup ();
}

const char *
CPI::SCA::PropertyParser::
encode ()
  throw ()
{
  if (!m_magicString) {
    m_magicString = CPI::SCA::encode_props (m_properties, m_numProperties,
                                            m_sizeOfPropertySpace,
                                            m_ports, m_numPorts,
                                            m_tests, m_numTests);
  }

  return m_magicString;
}

void
CPI::SCA::PropertyParser::
cleanup ()
  throw ()
{
  if (m_properties) {
    for (unsigned int pi=0; pi<m_numProperties; pi++) {
      delete [] m_properties[pi].name;
      delete [] m_properties[pi].types;
    }

    delete [] m_properties;
  }

  if (m_ports) {
    for (unsigned int pi=0; pi<m_numPorts; pi++) {
      delete [] m_ports[pi].name;
    }

    delete [] m_ports;
  }

  if (m_tests) {
    for (unsigned int ti=0; ti<m_numTests; ti++) {
      delete [] m_tests[ti].inputValues;
      delete [] m_tests[ti].resultValues;
    }

    delete [] m_tests;
  }

  if (m_magicString) {
    free (m_magicString);
  }
}

void
CPI::SCA::PropertyParser::
parse (CPI::Util::Vfs::Vfs & fs,
       const std::string & fileName)
  throw (std::string)
{
  /*
   * Parse that file.
   */

  CPI::Util::EzXml::Doc doc;
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
  cpiAssert (root && type);

  if (std::strcmp (type, "softpkg") == 0) {
    processSPD (fs, fileName, std::string(), root);
  }
  else if (std::strcmp (type, "softwarecomponent") == 0) {
    processSCD (fs, fileName, root);
  }
  else if (std::strcmp (type, "properties") == 0) {
    processPRF (root);
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

void
CPI::SCA::PropertyParser::
processSPD (CPI::Util::Vfs::Vfs & fs,
            const std::string & spdFileName,
            const std::string & implId,
            ezxml_t spdRoot)
  throw (std::string)
{
  cpiAssert (spdRoot && ezxml_name (spdRoot));
  cpiAssert (std::strcmp (ezxml_name (spdRoot), "softpkg") == 0);

  /*
   * The SCA says that "the SCD file is optional, since some SCA
   * components are non-CORBA components".  However, those are not
   * components that we support.
   */

  ezxml_t descriptorNode = ezxml_child (spdRoot, "descriptor");

  if (!descriptorNode) {
    throw std::string ("\"descriptor\" element not found in root node");
  }

  ezxml_t scdLocalFileNode = ezxml_child (descriptorNode, "localfile");

  if (!scdLocalFileNode) {
    throw std::string ("Missing \"localfile\" element");
  }

  const char * scdRelName = ezxml_attr (scdLocalFileNode, "name");

  if (!scdRelName) {
    throw std::string ("No \"name\" attribute in \"localfile\" element");
  }

  /*
   * Resolve SCD file against the SPD file.  If that doesn't work,
   * try looking for the SCD file in the same directory as the SPD
   * file, in case we messed up the directory structure by copying
   * files around and not updating the path.
   */

  std::string scdFileName;

  try {
    CPI::Util::Uri scdUri (fs.nameToURI (spdFileName));
    scdUri += scdRelName;

    scdFileName = fs.URIToName (scdUri.get());
  }
  catch (...) {
  }

  if (!scdFileName.length() || !fs.exists (scdFileName)) {
    const char * scdTail = std::strrchr (scdRelName, '/');

    if (scdTail) {
      scdTail++;
    }
    else {
      scdTail = scdRelName;
    }

    std::string spdDirName = CPI::Util::Vfs::directoryName (spdFileName);
    scdFileName = CPI::Util::Vfs::joinNames (spdDirName, scdTail);
  }

  if (!fs.exists (scdFileName)) {
    std::string errMsg = "Failed to find descriptor file \"";
    errMsg += scdRelName;
    errMsg += "\"";
    throw errMsg;
  }

  CPI::Util::EzXml::Doc scdDoc;
  ezxml_t scdRoot;

  try {
    scdRoot = scdDoc.parse (fs, scdFileName);
  }
  catch (const std::string & oops) {
    std::string errMsg = "Failed to parse property file \"";
    errMsg += scdFileName;
    errMsg += "\": ";
    errMsg += oops;
    throw errMsg;
  }

  const char * scdType = ezxml_name (scdRoot);
  cpiAssert (scdRoot && scdType);

  if (std::strcmp (scdType, "softwarecomponent") != 0) {
    std::string errMsg = "\"";
    errMsg += scdFileName;
    errMsg += "\" is not a descriptor file; expected \"softwarecomponent\" but got \"";
    errMsg += scdType;
    errMsg += "\" instead";
    throw errMsg;
  }

  processSCD (fs, scdFileName, scdRoot);

  /*
   * Now process the SPD's own PRF.  This element is optional.
   */

  ezxml_t propertyFileNode = ezxml_child (spdRoot, "propertyfile");

  if (propertyFileNode) {
    unsigned int oldNumProperties = m_numProperties;
    unsigned int oldNumTests = m_numTests;

    parsePropertyfile (fs, spdFileName,
                       propertyFileNode);

    if (m_numProperties != oldNumProperties ||
        m_numTests != oldNumTests) {
      /*
       * We picked up new properties and/or tests from this property file.
       * Update the component type: use the SPD's name.  The name attribute
       * is mandatory.
       */

      const char * spdName = ezxml_attr (spdRoot, "name");

      if (!spdName) {
        throw std::string ("root node lacks \"name\" attribute");
      }

      m_componentType = spdName;
    }
  }

  /*
   * If an implementation UUID was specified, parse this implementation's
   * PRF file, if present.
   */

  if (implId.length()) {
    ezxml_t implNode = ezxml_child (spdRoot, "implementation");

    while (implNode) {
      const char * id = ezxml_attr (implNode, "id");

      if (id && implId == id) {
        break;
      }

      implNode = ezxml_next (implNode);
    }

    if (!implNode) {
      std::string errMsg = "Implementation \"";
      errMsg += implId;
      errMsg += "\" not found";
      throw errMsg;
    }

    /*
     * Process this implementation's PRF, if present.
     */

    ezxml_t implPrfNode = ezxml_child (implNode, "propertyfile");

    if (implPrfNode) {
      unsigned int oldNumProperties = m_numProperties;
      unsigned int oldNumTests = m_numTests;

      parsePropertyfile (fs, spdFileName,
                         implPrfNode);

      if (m_numProperties != oldNumProperties ||
          m_numTests != oldNumTests) {
        /*
         * We picked up new properties and/or tests from this property file.
         * Update the component type: it is specific to this implementation.
         */

        m_componentType = implId;
      }
    }
  }
}

void
CPI::SCA::PropertyParser::
processSCD (CPI::Util::Vfs::Vfs & fs,
            const std::string & scdFileName,
            ezxml_t scdRoot)
  throw (std::string)
{
  cpiAssert (scdRoot && ezxml_name (scdRoot));
  cpiAssert (std::strcmp (ezxml_name (scdRoot), "softwarecomponent") == 0);

  ezxml_t scdFeaturesNode = ezxml_child (scdRoot, "componentfeatures");

  if (!scdFeaturesNode) {
    throw std::string ("\"componentfeatures\" element not found in root node");
  }

  ezxml_t scdPortsNode = ezxml_child (scdFeaturesNode, "ports");

  if (!scdPortsNode) {
    throw std::string ("\"ports\" element not found in \"componentfeatures\" node");
  }

  /*
   * Set the component "type".  Unfortunately, the SCA does not make this
   * easy, as an SCD has neither a name nor an ID.  The best we can do is
   * to use the file name, without its path or the ".scd.xml" suffix.
   */

  std::string relScdName = CPI::Util::Vfs::relativeName (scdFileName);
  std::string::size_type rsnl = relScdName.length ();

  if (rsnl > 8 && relScdName.substr (rsnl - 8) == ".scd.xml") {
    m_componentType = relScdName.substr (0, rsnl - 8);
  }
  else {
    m_componentType = relScdName;
  }

  /*
   * Count the number of ports.
   */

  unsigned int numPorts = 0;
  ezxml_t portsNode = scdPortsNode->child;

  while (portsNode) {
    const char * portType = ezxml_name (portsNode);

    if (portType &&
        (std::strcmp (portType, "uses") == 0 ||
         std::strcmp (portType, "provides") == 0)) {
      numPorts++;
    }

    portsNode = portsNode->ordered;
  }

  /*
   * Allocate ports.  Append to the set, if one exists already.
   */

  /*
   * if (!numPorts) goto readPropertyFile;
   */

  if (numPorts) {
    CPI::SCA::Port * portData = new CPI::SCA::Port [m_numPorts + numPorts];

    if (m_numPorts) {
      std::memcpy (portData, m_ports, m_numPorts * sizeof (CPI::SCA::Port));
      delete [] m_ports;
    }

    m_ports = portData;
    portData = &m_ports[m_numPorts];
    std::memset (m_ports, 0, numPorts * sizeof (CPI::SCA::Port));
    m_numPorts += numPorts;

    /*
     * Fill in port information.
     */

    portsNode = scdPortsNode->child;

    while (portsNode) {
      const char * portType = ezxml_name (portsNode);

      if (portType && std::strcmp (portType, "uses") == 0) {
        const char * portName = ezxml_attr (portsNode, "usesname");

        if (!portName) {
          throw std::string ("Missing \"usesname\" attribute for uses port");
        }

        portData->name = strdup (portName);
        portData->provider = false;
        portData->twoway = false;
        portData++;
      }
      else if (portType && std::strcmp (portType, "provides") == 0) {
        const char * portName = ezxml_attr (portsNode, "providesname");

        if (!portName) {
          throw std::string ("Missing \"providesname\" attribute for provides port");
        }

        portData->name = strdup (portName);
        portData->provider = true;
        portData->twoway = false;
        portData++;
      }

      portsNode = portsNode->ordered;
    }
  }

  /*
   * Find property file.  This element is optional.
   */

  ezxml_t propertyFileNode = ezxml_child (scdRoot, "propertyfile");

  if (propertyFileNode) {
    parsePropertyfile (fs, scdFileName,
                       propertyFileNode);
  }
}

void
CPI::SCA::PropertyParser::
parsePropertyfile (CPI::Util::Vfs::Vfs & fs,
                   const std::string & fileName,
                   ezxml_t propertyFileNode)
  throw (std::string)
{
  ezxml_t localFileNode = ezxml_child (propertyFileNode, "localfile");

  if (!propertyFileNode) {
    throw std::string ("Missing \"localfile\" element");
  }

  const char * prfRelName = ezxml_attr (localFileNode, "name");

  if (!prfRelName) {
    throw std::string ("No \"name\" attribute in \"localfile\" element");
  }

  /*
   * Resolve property file against the SPD file.  If that doesn't work,
   * try looking for the property file in the same directory as this
   * file, in case we messed up the directory structure by copying files
   * around and not updating the path.
   */

  std::string prfFileName;

  try {
    CPI::Util::Uri prfUri (fs.nameToURI (fileName));
    prfUri += prfRelName;

    prfFileName = fs.URIToName (prfUri.get());
  }
  catch (...) {
  }

  if (!prfFileName.length() || !fs.exists (prfFileName)) {
    const char * prfTail = std::strrchr (prfRelName, '/');

    if (prfTail) {
      prfTail++;
    }
    else {
      prfTail = prfRelName;
    }

    std::string scdDirName = CPI::Util::Vfs::directoryName (fileName);
    prfFileName = CPI::Util::Vfs::joinNames (scdDirName, prfTail);
  }

  if (!fs.exists (prfFileName)) {
    std::string errMsg = "Failed to find property file \"";
    errMsg += prfRelName;
    errMsg += "\"";
    throw errMsg;
  }

  CPI::Util::EzXml::Doc prfDoc;
  ezxml_t prfRoot;

  try {
    prfRoot = prfDoc.parse (fs, prfFileName);
  }
  catch (const std::string & oops) {
    std::string errMsg = "Failed to parse property file \"";
    errMsg += prfFileName;
    errMsg += "\": ";
    errMsg += oops;
    throw errMsg;
  }

  const char * prfType = ezxml_name (prfRoot);
  cpiAssert (prfRoot && prfType);

  if (std::strcmp (prfType, "properties") != 0) {
    std::string errMsg = "Property file \"";
    errMsg += prfFileName;
    errMsg += "\" is not a property file; expected \"properties\" but got \"";
    errMsg += prfType;
    errMsg += "\" instead";
    throw errMsg;
  }

  processPRF (prfRoot);
}

void
CPI::SCA::PropertyParser::
processPRF (ezxml_t prfRoot)
  throw (std::string)
{
  cpiAssert (prfRoot && ezxml_name (prfRoot));
  cpiAssert (std::strcmp (ezxml_name (prfRoot), "properties") == 0);

  /*
   * Count the number of properties, excluding duplicates.
   */

  unsigned int numProperties = 0;
  unsigned int numTests = 0;
  ezxml_t propertyNode = prfRoot->child;

  while (propertyNode) {
    const char * propertyType = ezxml_name (propertyNode);

    if (propertyType &&
        (std::strcmp (propertyType, "simple") == 0 ||
         std::strcmp (propertyType, "simplesequence") == 0) &&
        isConfigurableProperty (propertyNode)) {
      const char * propName = getNameOrId (propertyNode);

      if (!propName) {
        throw std::string ("Missing property name");
      }

      if (haveProperty (propName)) {
        continue;
      }

      numProperties++;
    }

    if (propertyType && std::strcmp (propertyType, "test") == 0) {
      if (!m_numTests && !numTests && !haveProperty ("testId")) {
        // testId property
        numProperties++;
      }

      numTests++;

      ezxml_t ivNode = ezxml_child (propertyNode, "inputvalue");

      if (ivNode) {
        ezxml_t ivProp = ezxml_child (ivNode, "simple");

        while (ivProp) {
          const char * propName = getNameOrId (ivProp);

          if (!propName) {
            throw std::string ("Missing property name");
          }

          if (haveProperty (propName)) {
            continue;
          }

          ivProp = ezxml_next (ivProp);
          numProperties++;
        }
      }

      ezxml_t ovNode = ezxml_child (propertyNode, "resultvalue");

      if (ovNode) {
        ezxml_t ovProp = ezxml_child (ovNode, "simple");

        while (ovProp) {
          const char * propName = getNameOrId (ovProp);

          if (!propName) {
            throw std::string ("Missing property name");
          }

          if (haveProperty (propName)) {
            continue;
          }

          ovProp = ezxml_next (ovProp);
          numProperties++;
        }
      }
    }

    propertyNode = propertyNode->ordered;
  }

  if (!numProperties) {
    return;
  }

  /*
   * Allocate properties.  Append to the set, if one exists already.
   */

  CPI::SCA::Property * propData = new CPI::SCA::Property [m_numProperties + numProperties];

  if (m_numProperties) {
    std::memcpy (propData, m_properties, m_numProperties * sizeof (CPI::SCA::Property));
    delete [] m_properties;
  }

  m_properties = propData;
  propData = &m_properties[m_numProperties];
  std::memset (propData, 0, numProperties * sizeof (CPI::SCA::Property));

  /*
   * Same for tests.
   */

  CPI::SCA::Test * testData = new CPI::SCA::Test [m_numTests + numTests];

  if (m_numTests) {
    std::memcpy (testData, m_tests, m_numTests * sizeof (CPI::SCA::Test));
    delete [] m_tests;
  }

  m_tests = testData;
  testData = &m_tests[m_numTests];
  std::memset (testData, 0, numTests * sizeof (CPI::SCA::Test));

  /*
   * Fill in property information.
   */

  unsigned int propIdx = m_numProperties;
  unsigned int offset = m_sizeOfPropertySpace;
  propertyNode = prfRoot->child;

  while (propertyNode) {
    const char * propertyType = ezxml_name (propertyNode);

    if (propertyType &&
        std::strcmp (propertyType, "simple") == 0 &&
        isConfigurableProperty (propertyNode)) {
      const char * propName = getNameOrId (propertyNode);

      if (!haveProperty (propName)) {
        processSimpleProperty (propertyNode, propData, offset, false, false);
        m_nameToIdxMap[propName] = propIdx++;
        propData++;
      }
      else {
        adjustSimpleProperty (propertyNode,
                              &m_properties[m_nameToIdxMap[propName]],
                              false, false);
      }
    }
    else if (propertyType &&
             std::strcmp (propertyType, "simplesequence") == 0 &&
             isConfigurableProperty (propertyNode)) {
      const char * propName = getNameOrId (propertyNode);

      if (!haveProperty (propName)) {
        processSimpleProperty (propertyNode, propData, offset, true, false);
        m_nameToIdxMap[propName] = propIdx++;
        propData++;
      }
      else {
        adjustSimpleProperty (propertyNode,
                              &m_properties[m_nameToIdxMap[propName]],
                              true, false);
      }
    }
    else if (propertyType && std::strcmp (propertyType, "test") == 0) {
      if (!haveProperty ("testId")) {
        propData->name = strdup ("testId");
        propData->is_sequence = false;
        propData->is_struct = false;
        propData->read_sync = false;
        propData->write_sync = false;
        propData->is_writable = true;
        propData->is_readable = false;
        propData->is_test = true;
        propData->num_members = 1;
        propData->types = new CPI::SCA::SimpleType [1];
        propData->types[0].data_type = CPI::SCA::SCA_ulong;
        propData->types[0].size = 0;
        propData->offset = roundUp (offset, 4);
        offset = propData->offset + 4;
        m_nameToIdxMap["testId"] = propIdx++;
        propData++;
      }

      const char * testIdStr = ezxml_attr (propertyNode, "id");
      char * endPtr;

      if (!testIdStr) {
        throw std::string ("Missing \"id\" attribute for test element");
      }

      testData->testId = std::strtoul (testIdStr, &endPtr, 10);

      if (*endPtr) {
        std::string errMsg = "Invalid test id \"";
        errMsg += testIdStr;
        errMsg += "\"";
        throw errMsg;
      }

      /*
       * Process input values.
       */

      ezxml_t ivNode = ezxml_child (propertyNode, "inputvalue");

      if (ivNode) {
        ezxml_t ivProp = ezxml_child (ivNode, "simple");
        unsigned int numInputValues = 0;

        while (ivProp) {
          const char * propName = getNameOrId (ivProp);

          if (!haveProperty (propName)) {
            processSimpleProperty (ivProp, propData, offset, false, true);
            m_nameToIdxMap[propName] = propIdx++;
            propData->is_writable = true;
            propData++;
          }

          ivProp = ezxml_next (ivProp);
          numInputValues++;
        }

        /*
         * Cross-reference input values.
         */

        testData->numInputs = numInputValues;
        testData->inputValues = new unsigned int [numInputValues];

        ivProp = ezxml_child (ivNode, "simple");
        unsigned int inputValueIdx = 0;

        while (ivProp) {
          const char * propName = getNameOrId (ivProp);
          testData->inputValues[inputValueIdx++] = m_nameToIdxMap[propName];
          ivProp = ezxml_next (ivProp);
        }
      }

      /*
       * Process result values.
       */

      ezxml_t ovNode = ezxml_child (propertyNode, "resultvalue");

      if (ovNode) {
        ezxml_t ovProp = ezxml_child (ovNode, "simple");
        unsigned int numResultValues = 0;

        while (ovProp) {
          const char * propName = getNameOrId (ovProp);

          if (!haveProperty (propName)) {
            processSimpleProperty (ovProp, propData, offset, false, true);
            m_nameToIdxMap[propName] = propIdx++;
            propData->is_readable = true;
            propData++;
          }

          ovProp = ezxml_next (ovProp);
          numResultValues++;
        }

        /*
         * Cross-reference result values.
         */

        testData->numResults = numResultValues;
        testData->resultValues = new unsigned int [numResultValues];

        ovProp = ezxml_child (ovNode, "simple");
        unsigned int resultValueIdx = 0;

        while (ovProp) {
          const char * propName = getNameOrId (ovProp);
          testData->resultValues[resultValueIdx++] = m_nameToIdxMap[propName];
          ovProp = ezxml_next (ovProp);
        }
      }

      testData++;
    }

    propertyNode = propertyNode->ordered;
  }

  m_numProperties = propIdx;
  m_sizeOfPropertySpace = offset;
  m_numTests += numTests;
}

void
CPI::SCA::PropertyParser::
processSimpleProperty (ezxml_t simplePropertyNode,
                       CPI::SCA::Property * propData,
                       unsigned int & offset,
                       bool isSequence,
                       bool isTest)
  throw (std::string)
{
  const char * propId = ezxml_attr (simplePropertyNode, "id");
  const char * propMode = ezxml_attr (simplePropertyNode, "mode");
  const char * propName = ezxml_attr (simplePropertyNode, "name");
  const char * propType = ezxml_attr (simplePropertyNode, "type");

  if (!propId) {
    throw std::string ("property lacks \"id\" attribute");
  }

  if (!propType) {
    throw std::string ("property lacks \"type\" attribute");
  }

  propData->name = strdup (propName ? propName : propId);
  propData->is_sequence = isSequence;
  propData->is_struct = false;
  propData->is_readable = false;
  propData->is_writable = false;
  propData->read_sync = isTest ? false : true;
  propData->write_sync = isTest ? false : true;
  propData->is_test = isTest;

  if (!isTest && (!propMode ||
                  std::strcmp (propMode, "readwrite") == 0 ||
                  std::strcmp (propMode, "writeonly") == 0)) {
    propData->is_writable = true;
  }

  if (!isTest && (!propMode ||
                  std::strcmp (propMode, "readwrite") == 0 ||
                  std::strcmp (propMode, "readonly")  == 0)) {
    propData->is_readable = true;
  }

  propData->num_members = 1;
  propData->types = new CPI::SCA::SimpleType [1];
  CPI::SCA::SimpleType & pt = propData->types[0];

  if (std::strcmp (propType, "string") == 0) {
    if (isSequence) {
      throw std::string ("sequences of strings not supported");
    }

    /*
     * CP289 proposed a "max_string_size" attribute.  But since CP289
     * was never officially adopted, this attribute can not exist in a
     * compliant property file.
     *
     * As a workaround, we accept "max_string_size=<nn>" as part of
     * the description.
     *
     * If that is missing as well, use a hard-coded default.
     */

    const char * propMaxSize = ezxml_attr (simplePropertyNode, "max_string_size");

    if (!propMaxSize) {
      ezxml_t descNode = ezxml_child (simplePropertyNode, "description");
      const char * description = descNode ? ezxml_txt (descNode) : 0;
      propMaxSize = description ? std::strstr (description, "max_string_size=") : 0;
      propMaxSize = propMaxSize ? propMaxSize+16 : 0;
    }

    if (propMaxSize) {
      char * endPtr;
      pt.size = std::strtoul (propMaxSize, &endPtr, 10);

      if (!propData->types[0].size || *endPtr) {
        std::string errMsg = "Invalid value \"";
        errMsg += propMaxSize;
        errMsg += "\" for \"max_string_size\" property";
        throw errMsg;
      }
    }
    else {
      pt.size = 255; /* Hardcoded default. */
    }

    pt.data_type = CPI::SCA::SCA_string;
    propData->offset = roundUp (offset, 4);
    offset = propData->offset + propData->types[0].size + 1;
  }
  else {
    pt.data_type = mapPropertyType (propType);
    pt.size = 0;

    unsigned int size = propertySize (pt.data_type);
    unsigned int align = propertyAlign (pt.data_type);

    if (isSequence) {
      /*
       * CP289 proposed a "max_sequence_size" attribute.  But since CP289
       * was never officially adopted, this attribute can not exist in a
       * compliant property file.
       *
       * As a workaround, we accept "max_sequence_size=<nn>" as part of
       * the description.
       *
       * If that is missing as well, use a hard-coded default.
       */

      const char * propSeqSize = ezxml_attr (simplePropertyNode, "max_sequence_size");

      if (!propSeqSize) {
        ezxml_t descNode = ezxml_child (simplePropertyNode, "description");
        const char * description = descNode ? ezxml_txt (descNode) : 0;
        propSeqSize = description ? std::strstr (description, "max_sequence_size=") : 0;
        propSeqSize = propSeqSize ? propSeqSize+18 : 0;
      }

      if (propSeqSize) {
        char * endPtr;
        propData->sequence_size = std::strtoul (propSeqSize, &endPtr, 10);

        if (!propData->sequence_size || *endPtr) {
          std::string errMsg = "Invalid value \"";
          errMsg += propSeqSize;
          errMsg += "\" for \"max_sequence_size\" property";
          throw errMsg;
        }
      }
      else {
        propData->sequence_size = 256; /* Hardcoded default. */
      }

      propData->offset = roundUp (offset, 4);
      propData->data_offset = roundUp (propData->offset + 4, align);
      offset = propData->data_offset + size * propData->sequence_size;
    }
    else {
      propData->offset = roundUp (offset, align);
      offset = propData->offset + size;
    }
  }
}

void
CPI::SCA::PropertyParser::
adjustSimpleProperty (ezxml_t propertyNode,
                      CPI::SCA::Property * propData,
                      bool isSequence,
                      bool isTest)
  throw (std::string)
{
  const char * propMode = ezxml_attr (propertyNode, "mode");

  if (!isTest) {
    propData->read_sync = true;
    propData->write_sync = true;
    propData->is_test = false;

    if (!propMode ||
        std::strcmp (propMode, "readwrite") == 0 ||
        std::strcmp (propMode, "writeonly") == 0) {
      propData->is_writable = true;
    }

    if (!propMode ||
        std::strcmp (propMode, "readwrite") == 0 ||
        std::strcmp (propMode, "readonly")  == 0) {
      propData->is_readable = true;
    }
  }
}

const char *
CPI::SCA::PropertyParser::
getNameOrId (ezxml_t node)
  throw ()
{
  const char * propName = ezxml_attr (node, "name");

  if (!propName) {
    propName = ezxml_attr (node, "id");
  }

  return propName;
}

bool
CPI::SCA::PropertyParser::
haveProperty (const char * name)
  throw ()
{
  return (m_nameToIdxMap.find (name) != m_nameToIdxMap.end());
}

bool
CPI::SCA::PropertyParser::
isConfigurableProperty (ezxml_t propertyNode)
  throw ()
{
  bool isConfigurable = false;
  ezxml_t kindNode = ezxml_child (propertyNode, "kind");

  while (kindNode && !isConfigurable) {
    const char * kindType = ezxml_attr (kindNode, "kindtype");

    if (!kindType || std::strcmp (kindType, "configure") == 0) {
      isConfigurable = true;
    }
  }

  return isConfigurable;
}

CPI::SCA::DataType
CPI::SCA::PropertyParser::
mapPropertyType (const char * type)
  throw (std::string)
{
  if (std::strcmp (type, "boolean") == 0) {
    return CPI::SCA::SCA_boolean;
  }
  else if (std::strcmp (type, "char") == 0) {
    return CPI::SCA::SCA_char;
  }
  else if (std::strcmp (type, "double") == 0) {
    return CPI::SCA::SCA_double;
  }
  else if (std::strcmp (type, "float") == 0) {
    return CPI::SCA::SCA_float;
  }
  else if (std::strcmp (type, "short") == 0) {
    return CPI::SCA::SCA_short;
  }
  else if (std::strcmp (type, "long") == 0) {
    return CPI::SCA::SCA_long;
  }
  else if (std::strcmp (type, "octet") == 0) {
    return CPI::SCA::SCA_octet;
  }
  else if (std::strcmp (type, "ulong") == 0) {
    return CPI::SCA::SCA_ulong;
  }
  else if (std::strcmp (type, "ushort") == 0) {
    return CPI::SCA::SCA_ushort;
  }

  /* string handled separately by caller */

  std::string errMsg = "Invalid type \"";
  errMsg += type;
  errMsg += "\"";
  throw errMsg;
}

unsigned int
CPI::SCA::PropertyParser::
propertySize (CPI::SCA::DataType type)
  throw ()
{
  unsigned int size;

  switch (type) {
  case CPI::SCA::SCA_boolean: size = 1; break;
  case CPI::SCA::SCA_char:    size = 1; break;
  case CPI::SCA::SCA_double:  size = 8; break;
  case CPI::SCA::SCA_float:   size = 4; break;
  case CPI::SCA::SCA_short:   size = 2; break;
  case CPI::SCA::SCA_long:    size = 4; break;
  case CPI::SCA::SCA_octet:   size = 1; break;
  case CPI::SCA::SCA_ulong:   size = 4; break;
  case CPI::SCA::SCA_ushort:  size = 2; break;
  default: cpiAssert (0); break;
  }

  return size;
}

unsigned int
CPI::SCA::PropertyParser::
propertyAlign (CPI::SCA::DataType type)
  throw ()
{
  unsigned int align;

  switch (type) {
  case CPI::SCA::SCA_boolean: align = 1; break;
  case CPI::SCA::SCA_char:    align = 1; break;
  case CPI::SCA::SCA_double:  align = 8; break;
  case CPI::SCA::SCA_float:   align = 4; break;
  case CPI::SCA::SCA_short:   align = 2; break;
  case CPI::SCA::SCA_long:    align = 4; break;
  case CPI::SCA::SCA_octet:   align = 1; break;
  case CPI::SCA::SCA_ulong:   align = 4; break;
  case CPI::SCA::SCA_ushort:  align = 2; break;
  default: cpiAssert (0); break;
  }

  return align;
}

unsigned int
CPI::SCA::PropertyParser::
roundUp (unsigned int value, unsigned int align)
  throw ()
{
  return ((value + (align - 1)) / align) * align;
}

char *
CPI::SCA::PropertyParser::
strdup (const char * string)
  throw ()
{
  unsigned int len = std::strlen (string);
  char * dup = new char [len+1];
  std::memcpy (dup, string, len);
  dup[len] = 0;
  return dup;
}

