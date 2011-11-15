
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
 * Configure worker properties according to the property sheet.
 *
 * Revision History:
 *
 *     05/06/2009 - Frank Pilhofer
 *                  RCC updates (interpretation of maximum string size to
 *                  exclude the null character).
 *
 *     02/25/2009 - Frank Pilhofer
 *                  Add support for test properties.
 *                  Add accessor for list of ports.
 *                  Constify accessors.
 *
 *     11/13/2008 - Frank Pilhofer
 *                  Add missing getPort operation.
 *
 *     10/22/2008 - Frank Pilhofer
 *                  String size includes the null character.
 *
 *     10/17/2008 - Frank Pilhofer
 *                  Don't place code with side effects in assertions.
 *                  Integrate with WCI API updates.
 *
 *     10/02/2008 - Jim Kulp
 *                  Initial version.
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "OcpiOsAssert.h"
#include "OcpiMetadataWorker.h"
#include "OcpiContainerMisc.h"
#include "OcpiUtilEzxml.h"

namespace OCPI {
  namespace OU = OCPI::Util;
  namespace Metadata {

    Worker::~Worker() {
	if (myPorts)
	  delete [] myPorts;
	if (myProps)
	  delete [] myProps;
	if (myTests)
	  delete [] myTests;
      }
    unsigned Worker::whichProperty(const char *id) {
      Property *p = myProps;
      for (unsigned n=0; n < nProps; n++, p++)
        if (!strcmp(p->m_name.c_str(), id))
          return n;
      throw OU::ApiError("Unknown property: \"", id, "\"", 0);
    }
    Property &Worker::findProperty(const char *id) {
      return *(myProps + whichProperty(id));
    }
    Port *Worker::findMetaPort(const char *id) {
      Port *p = myPorts;
      for (unsigned int n=nPorts; n; n--, p++) {
        if ( strcmp(p->name,id)==0 )
          return p;
      }
      return 0;
    }
    Test &Worker::findTest(unsigned int testId) {
       (void)testId;
      assert(0); static Test *t; return *t;
    }

    // Decode based on XML, determining offsets
    Worker::Worker(ezxml_t xml)
      : myProps(0), myPorts(0), myTests(0), myLocalMemories(0), nProps(0),
        nPorts(0), nTests(0), nLocalMemories(0), size(0),
        totalPropertySize(0)
    {
      ezxml_t x;
      // First pass - just count for allocation
      for (x = ezxml_cchild(xml, "property"); x; x = ezxml_next(x))
        nProps++;
      for (x = ezxml_cchild(xml, "port"); x; x = ezxml_next(x))
        nPorts++;
      for (x = ezxml_cchild(xml, "localMemory"); x; x = ezxml_next(x))
        nLocalMemories++;
      if (nProps)
	myProps = new Property[nProps];
      if (nPorts)
	myPorts = new Port[nPorts];
     if (nLocalMemories)
        myLocalMemories = new LocalMemory[nLocalMemories];
      const char *err;
      // Second pass - decode all information
      Property *prop = myProps;
      unsigned offset = 0;
      bool readableConfigs, writableConfigs, sub32Configs; // all unused
      for (x = ezxml_cchild(xml, "property"); x; x = ezxml_next(x), prop++)
      {
        if ((err = prop->parse(x, offset, readableConfigs, writableConfigs,
			       sub32Configs, true, prop - myProps)))
          throw OU::ApiError("Invalid xml property description:", err, NULL);
        totalPropertySize += prop->m_nBytes;
      }
      // Ports at this level are unidirectional? Or do we support the pairing at this point?
      unsigned n = 0;
      Port *p = myPorts;
      for (x = ezxml_cchild(xml, "port"); x; x = ezxml_next(x), p++, n++)
        if (p->decode(x, n))
          throw OU::ApiError("Invalid xml port description", 0);
      LocalMemory* m = myLocalMemories;
      for (x = ezxml_cchild(xml, "localMemory"); x; x = ezxml_next(x), m++ )
        if (m->decode(x))
          throw OU::ApiError("Invalid xml local memory description", 0);
    }
  }
}

