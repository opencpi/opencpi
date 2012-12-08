
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
#include <list>
#include "ezxml.h"
#include "OcpiPValue.h"

namespace OCPI {
  namespace Util {
    class Assembly {
    public:
      // This class is overloaded both for property values for individual instances
      // as well as top level properties that are mapped to workers.
      struct Instance;
      struct MappedProperty {
	std::string m_name;
	std::string m_instPropName; // non-empty for top level
	unsigned m_instance;        // if m_instPropName is nonempty this is valid
	const char *parse(ezxml_t x, Assembly &a);
      };
      typedef std::vector<MappedProperty> MappedProperties;
      struct Property {
	std::string m_name;
	std::string m_value;
	const char *parse(ezxml_t x);
	const char *setValue(ezxml_t px);
      };
      typedef std::vector<Property> Properties;
      struct Port;
      struct Instance {
	std::string
	  m_name,                  // name of the instance within the assembly
	  m_specName,              // name of component being instantiated
	  m_selection;             // the selection expression
	unsigned m_ordinal;
	Properties m_properties;
	PValueList m_parameters;
	std::list<Port*> m_ports; // attachments to connections
	const char *parse(ezxml_t ix, ezxml_t ax, unsigned ordinal);
	const char *addProperty(const char *name, ezxml_t px);
	const char *parseConnection(ezxml_t ix, Assembly &a);
      };
      // The attachment of a connection to external or port
      struct External {
	std::string m_name; // the name
	std::string m_url;  // the URL that this external attachment has
	bool m_provider;    // is this external attachment acting as a provider to the world?
	PValueList m_parameters;
	const char *parse(ezxml_t, unsigned&, const PValue *pvl);
      };
      struct Connection;
      struct Port {
	// This mutable is because this name might be resolved when an application
	// uses this assembly (and has access to impl metadata).
	// Then this assembly is reused, this resolution will still be valid.
	mutable std::string m_name;
	unsigned m_instance;
	PValueList m_parameters;
	bool m_input; // if no name
	Port *m_connectedPort;
	const char *parse(ezxml_t x, Assembly &a, const PValue *pvl);
	void init(const char *name, unsigned instance, bool isInput);
      };
      struct Connection {
	std::string m_name;
	std::vector<External> m_externals;
	std::vector<Port> m_ports;
	PValueList m_parameters;
	const char *parse(ezxml_t x, Assembly &a, unsigned &ord);
	Port &addPort(unsigned instance, const char *port, bool isInput);
	void addExternal(const char *name);
      };
      // Potentially specified in the assembly, what policy should be used
      // to spread workers to containers?
      enum CMapPolicy {
	RoundRobin,
	MinProcessors,
	MaxProcessors
      };
    private:
      ezxml_t m_xml;
      char *m_copy;
      const char *parse();
    public:
      static unsigned s_count;
      std::string m_name;
      int m_doneInstance; // -1 for none
      std::vector<Instance> m_instances;
      std::vector<Connection> m_connections;
      CMapPolicy m_cMapPolicy;
      uint32_t   m_processors;
      MappedProperties m_mappedProperties; // top level mapped to instance properties.
      // Provide a file name.
      explicit Assembly(const char *file);
      // Provide a string containing the xml
      explicit Assembly(const std::string &string);
      ~Assembly();
      const char
        *getInstance(const char *name, unsigned &),
	*addConnection(const char *name, Connection *&c),
        *addPortConnection(unsigned from, const char *name, unsigned to, const char *toPort),
        *addExternalConnection(unsigned instance, const char *port);

    };
  }
}
#endif
