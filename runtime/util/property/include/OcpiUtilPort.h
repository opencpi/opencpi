/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
    class Worker;
    class Port : public Protocol {
    public:
      typedef uint32_t Mask;
      static const Mask c_maxPorts = sizeof(Mask)*8;
      std::string m_name;
      PortOrdinal m_ordinal;
      bool        m_provider;
      bool        m_optional;
      bool        m_bidirectional;   // implementation-defined value
      size_t      m_minBufferCount;  // implementation-defined value
      size_t      m_bufferSize;      // metadata protocol override, if non-zero
      ezxml_t     m_xml;
      Worker     *m_worker;
      Port       *m_bufferSizePort;  // The port we should copy our buffer size from
      Port(bool provider = true);
      const char *cname() const { return m_name.c_str(); }
      const char *preParse(Worker &w, ezxml_t x, PortOrdinal ord);
      const char *parse(ezxml_t x);
      const char *postParse();
    };

  }
}
#endif
