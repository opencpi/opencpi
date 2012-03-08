
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
  namespace CC = OCPI::Container;
  namespace OE = OCPI::Util::EzXml;
  namespace Metadata {

    //    const unsigned Port::DEFAULT_NBUFFERS = 1;
    // const unsigned Port::DEFAULT_BUFFER_SIZE = 2*1024;

    Port::Port(bool prov)
      : name(NULL), ordinal(0), provider(prov), optional(false),
	bidirectional(false), minBufferCount(1), bufferSize(0), myXml(NULL) {
    }

    bool Port::decode(ezxml_t x, PortOrdinal aOrdinal) {
      myXml = x;
      name = ezxml_cattr(x, "name");
      if (!name)
	return true;
      ordinal = aOrdinal;
      ocpiDebug("Port %s has ordinal = %d\n", name, ordinal );

      // Initialize everything from the protocol, then other attributes can override the protocol
      ezxml_t protocol = ezxml_cchild(x, "protocol");
      if (protocol)
	Protocol::parse(protocol);
      if (OE::getBoolean(x, "twoWay", &m_isTwoWay) ||           // protocol override
	  OE::getBoolean(x, "bidirectional", &bidirectional) ||
	  OE::getBoolean(x, "provider", &provider) ||
	  OE::getBoolean(x, "optional", &optional) ||
	  OE::getNumber(x, "minBufferCount", &minBufferCount, 0, 1) ||
	  OE::getNumber(x, "bufferSize", &bufferSize, 0, 0))
	return true;
      if (bufferSize == 0)
	bufferSize = m_defaultBufferSize; // from protocol
	  
      // FIXME: do we need the separately overridable nOpcodes here?
      return false;
    }
    Port::~Port(){}
  }
}

