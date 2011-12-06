
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

#include <string.h>
#include "OcpiOsAssert.h"
#include "OcpiUtilImplementation.h"
#include "OcpiUtilEzxml.h"

namespace OCPI {
  namespace Util {

    Implementation::Implementation()
      : m_properties(0), m_ports(0), m_tests(0), m_memories(0),
	m_nProperties(0), m_nPorts(0), m_nTests(0), m_nMemories(0),// size(0),
        m_totalPropertySize(0), m_xml(NULL)
    {}

    Implementation::~Implementation() {
      delete [] m_ports;
      delete [] m_properties;
      delete [] m_tests;
      delete [] m_memories;
    }
    unsigned Implementation::whichProperty(const char *id) const {
      Property *p = m_properties;
      for (unsigned n=0; n < m_nProperties; n++, p++)
        if (p->m_name == id)
          return n;
      throw Error("Unknown property: \"%s\"", id);
    }
    Property &Implementation::findProperty(const char *id) const {
      return *(m_properties + whichProperty(id));
    }
    Port *Implementation::findPort(const char *id) const {
      Port *p = m_ports;
      for (unsigned int n = m_nPorts; n; n--, p++)
        if (m_name == id)
          return p;
      return 0;
    }
    Test &Implementation::findTest(unsigned int testId) const {
       (void)testId;
       ocpiAssert(0); return *m_tests;
    }

    const char *Implementation::parse(ezxml_t xml) {
      ezxml_t x;
      // First pass - just count for allocation
      for (x = ezxml_cchild(xml, "property"); x; x = ezxml_next(x))
        m_nProperties++;
      for (x = ezxml_cchild(xml, "port"); x; x = ezxml_next(x))
        m_nPorts++;
      for (x = ezxml_cchild(xml, "localMemory"); x; x = ezxml_next(x))
        m_nMemories++;
      for (x = ezxml_cchild(xml, "memory"); x; x = ezxml_next(x))
        m_nMemories++;
      if (m_nProperties)
	m_properties = new Property[m_nProperties];
      if (m_nPorts)
	m_ports = new Port[m_nPorts];
     if (m_nMemories)
        m_memories = new Memory[m_nMemories];
      const char *err;
      // Second pass - decode all information
      Property *prop = m_properties;
      unsigned offset = 0;
      bool readableConfigs, writableConfigs, sub32Configs; // all unused
      for (x = ezxml_cchild(xml, "property"); x; x = ezxml_next(x), prop++) {
        if ((err = prop->parse(x, offset, readableConfigs, writableConfigs,
			       sub32Configs, true, prop - m_properties)))
          return esprintf("Invalid xml property description: %s", err);
        m_totalPropertySize += prop->m_nBytes;
      }
      // Ports at this level are unidirectional? Or do we support the pairing at this point?
      unsigned n = 0;
      Port *p = m_ports;
      for (x = ezxml_cchild(xml, "port"); x; x = ezxml_next(x), p++, n++)
        if ((err = p->parse(x, n)))
          return esprintf("Invalid xml port description: %s", err);
      Memory* m = m_memories;
      for (x = ezxml_cchild(xml, "memory"); x; x = ezxml_next(x), m++ )
        if ((err = m->parse(x)))
          return esprintf("Invalid xml local memory description: %s", err);
      for (x = ezxml_cchild(xml, "memory"); x; x = ezxml_next(x), m++ )
        if ((err = m->parse(x)))
          return esprintf("Invalid xml local memory description: %s", err);
      m_xml = xml;
      return NULL;
    }
  }
}

