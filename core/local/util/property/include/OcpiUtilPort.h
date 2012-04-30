
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

#ifndef OCPI_UTIL_PORT_H
#define OCPI_UTIL_PORT_H

#include <string>
#include "OcpiUtilPort.h"
#include "OcpiUtilProtocol.h"
#include "ezxml.h"

namespace OCPI {
  namespace Util {
    typedef uint32_t PortOrdinal;
    // FIXME:  use a pointer to a protocol, and share protocols in the artifact xml
    class Implementation;
    class Port : public Protocol {
    public:
      typedef uint32_t Mask;
      static const Mask c_maxPorts = sizeof(Mask)*8;
      std::string m_name;
      PortOrdinal m_ordinal;
      bool        m_provider;
      bool        m_optional;
      bool        m_bidirectional;   // implementation-defined value
      uint32_t    m_minBufferCount;  // implementation-defined value
      uint32_t    m_bufferSize;      // metadata protocol override, if non-zero
      ezxml_t     m_xml;
      //      Port       *m_connected;       // static connection of this port.
      Implementation *m_implementation;
      Port(bool provider = true);
      const char *parse(ezxml_t x, PortOrdinal ord);
    };

  }
}
#endif
