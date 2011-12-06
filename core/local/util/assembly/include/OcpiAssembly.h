
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
 Definitions for assembly metadata decoding, etc.
*/
#ifndef OCPI_ASSEMBLY_H
#define OCPI_ASSEMBLY_H
#include <string>
#include "ezxml.h"
#include "OcpiPValue.h"
#include "OcpiUtilDataTypes.h"

namespace OCPI {
  namespace Util {
    typedef const char *AssemblyProperty[2];
    class Instance {
    public:
      std::string m_name;              // The name of the instance
      const char *m_specName;          // The name of the component to be instantiated
      unsigned m_nProperties;
      AssemblyProperty *m_properties;
      const char *m_selection;         // The selection expression
      Instance();
      ~Instance();
      const char *parse(ezxml_t, unsigned);
    };
    class External {
    public:
      bool m_portIsProvider;
    };
    class Assembly;
    class Attachment {
    public:
      Instance *m_instance; // if null, port is external
      const char *m_port;
      const char *m_url;
      unsigned m_nProperties;
      AssemblyProperty *m_properties;
      Attachment();
      const char *parse(ezxml_t x, Assembly &a);
    };
    class Connection {
    public:
      std::string m_name;
      unsigned m_nAttachments;
      Attachment *m_attachments;
      Connection();
      ~Connection();
      const char *parse(ezxml_t x, unsigned ordinal, Assembly &a);
    };
    class Assembly {
    public:
      std::string m_name;
      unsigned m_nInstances;
      Instance *m_instances;        // An array
      unsigned m_nConnections;
      Connection *m_connections;
      ezxml_t m_xml;
      Assembly(const char *file);
      Assembly(const ezxml_t x);
      ~Assembly();
      const char *parse(ezxml_t a);
      Instance *getInstance(const char *name);
    };
  }
}
#endif
