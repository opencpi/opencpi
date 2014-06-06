
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

#include <string>
#include <ctype.h>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilPort.h"
#include "OcpiUtilImplementation.h"
#include "OcpiUtilMisc.h"

namespace OCPI {
  namespace Util {
  namespace OE = OCPI::Util::EzXml;

    Port::
    Port(bool prov)
      : m_ordinal(0), m_provider(prov), m_optional(false),
	m_bidirectional(false), m_minBufferCount(1), m_bufferSize(0), m_xml(NULL),
	m_worker(NULL), m_bufferSizePort(NULL) {
    }

    // First pass captures name
    const char *Port::
    preParse(Worker &w, ezxml_t x, PortOrdinal ordinal) {
      m_worker = &w;
      m_ordinal = ordinal;
      m_xml = x;
      return OE::getRequiredString(x, m_name, "name", "port");
    }

    // second pass can use names of other ports.
    const char *Port::
    parse(ezxml_t x) {
      const char *err;
      // Initialize everything from the protocol, then other attributes can override the protocol
      ezxml_t protocol = ezxml_cchild(x, "protocol");
      if (protocol &&
	  (err = Protocol::parse(protocol)))
	return err;
      if ((err = OE::getBoolean(x, "twoWay", &m_isTwoWay)) ||           // protocol override
	  (err = OE::getBoolean(x, "bidirectional", &m_bidirectional)) ||
	  (err = OE::getBoolean(x, "provider", &m_provider)) ||
	  (err = OE::getBoolean(x, "optional", &m_optional)) ||
	  (err = OE::getNumber(x, "minBufferCount", &m_minBufferCount, 0, 1)))
	return err;
      const char *bs = ezxml_cattr(x, "bufferSize");
      if (bs && !isdigit(bs[0])) {
	if (!(m_bufferSizePort = m_worker->findMetaPort(bs)))
	  return esprintf("Buffersize set to '%s', which is not a port name or a number",
			      bs);
      } else if ((err = OE::getNumber(x, "bufferSize", &m_bufferSize, 0, 0)))
	return err;
      if (m_bufferSize == 0)
	m_bufferSize = m_defaultBufferSize; // from protocol
	  
      // FIXME: do we need the separately overridable nOpcodes here?
      return NULL;
    }
    const char *Port::
    postParse() {
      if (m_bufferSizePort)
	m_bufferSize = m_bufferSizePort->m_bufferSize;
      return NULL;
    }
  }
}

