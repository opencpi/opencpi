
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
 * Definitions for assembly metadata decoding, etc.
 * The result of parsing an xml assembly, in a vacuum (no access to impl metadata)
 *
 * After parsing this class does not depend on the existence of the xml from which it
 * was parsed.
 *
 * Example:
 * <assembly name="myapp">
 *  <instance name="fred" specName="fft1d"/>
 *  <instance name="i2" specName="sink" selection="select-expression">
 *    <property name="knob1" value="4"/>
 *  </instance>
 *  <connection name=helpin>
 *    <port name="input" instance="fred"/>
 *    <external name="globin" provider="true" url="ddstopic:"/>
 *  </connection>
 * </assembly>
 */
#ifndef OCPI_UTIL_ASSEMBLY_H
#define OCPI_UTIL_ASSEMBLY_H
#include <string>
#include <vector>
#include "ezxml.h"
#include "OcpiPValue.h"

namespace OCPI {
  namespace Util {
    class Assembly {
    public:
      struct Property {
	std::string m_name;
	std::string m_value;
	const char *parse(ezxml_t x);
      };
      typedef std::vector<Property> Properties;
      struct Instance {
	std::string
	  m_name,      // name of the instance within the assembly
	  m_specName,  // name of component being instantiated
	  m_selection; // the selection expression
	Properties m_properties;
	PValueList m_parameters;
	const char *parse(ezxml_t ix, ezxml_t ax);
      };
      struct External {
	std::string m_name; // the name
	std::string m_url;  // the URL that this external attachment has
	bool m_provider;    // is this external attachment acting as a provider to the world?
	PValueList m_parameters;
	const char *parse(ezxml_t, unsigned&, const PValue *pvl);
      };
      // The attachment of a connection to external
      struct Port {
	std::string m_name;
	unsigned m_instance;
	PValueList m_parameters;
	const char *parse(ezxml_t x, Assembly &a, const PValue *pvl);
      };
      struct Connection {
	std::string m_name;
	std::vector<External> m_externals;
	std::vector<Port> m_ports;
	PValueList m_parameters;
	const char *parse(ezxml_t x, Assembly &a, unsigned &ord);
      };
    private:
      ezxml_t m_xml;
      char *m_copy;
    public:
      static unsigned s_count;
      std::string m_name;
      std::vector<Instance> m_instances;
      std::vector<Connection> m_connections;
      // Provide a file name.
      explicit Assembly(const char *file);
      // Provide a string containing the xml
      explicit Assembly(const std::string &string);
      ~Assembly();
      const char *parse(ezxml_t a);
      const char *getInstance(const char *name, unsigned &);
    };
  }
}
#endif
