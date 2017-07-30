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

#include <set>
#include "OcpiUtilUUID.h"
#include "OcpiApplication.h"
#include "cdkutils.h"
#include "utils.h"

namespace OCPI {
  namespace API {
    namespace OU = OCPI::Util;
    namespace SCA = OCPI::SCA;
    namespace OC = OCPI::Container;
    namespace OX = OCPI::Util::EzXml;

static const char *repid(std::string &s, const char *name, const char *module = NULL) {
  return OU::format(s, "IDL:%s/%s:1.0", module ? module : "CF", name);
}


static 
const char *mapType(OCPI::API::BaseType &bt) {
  const char *typeName = "Unknown";
  switch( bt ) {
  case OCPI::API::OCPI_Short:
    typeName = "dataShort";
    break;
  case OCPI::API::OCPI_Char:
    typeName = "dataChar";
    break;
  case OCPI::API::OCPI_Double:
    typeName = "dataDouble";
    break;
  case OCPI::API::OCPI_Float:
    typeName = "dataFloat";
    break;
  case OCPI::API::OCPI_Long:
    typeName = "dataLong";
    break;
  case OCPI::API::OCPI_ULong:
    typeName = "dataULong";
    break;
  case OCPI::API::OCPI_UShort:
    typeName = "dataUShort";
    break;
  case OCPI::API::OCPI_LongLong:
    typeName = "dataLongLong";
    break;
  case OCPI::API::OCPI_ULongLong:
    typeName = "dataULongLong";
    break;

    case OCPI::API::OCPI_UChar:
    case OCPI::API::OCPI_scalar_type_limit:
    case OCPI::API::OCPI_none:
    case OCPI::API::OCPI_Bool:
    case OCPI::API::OCPI_Enum:
    case OCPI::API::OCPI_Type:
    case OCPI::API::OCPI_String:
    case OCPI::API::OCPI_Struct:
      assert(0);
      break;
    }
  return typeName;
}



void ApplicationI::
genScaScd(const char *outDir) const {
  FILE *f;
  std::string path;
  const char *err = openOutput(m_assembly.name().c_str(), outDir, "", ".scd", ".xml", NULL, f,
			       &path);
  if (err)
    throw OU::Error("Error opening SCD output file: %s", err);
  std::string s;
  ezxml_t root = ezxml_new("softwarecomponent");
  OX::addChild(root, "corbaversion", 1, "2.2");
  OX::addChild(root, "componentrepid", 1, NULL, "repid", repid(s, "Resource"));
  OX::addChild(root, "componenttype", 1, "resource");
  const char *supports[] =
    { "Resource", "LifeCycle", "PortSupplier", "PropertySet", "TestableObject", NULL};
  ezxml_t cfx = OX::addChild(root, "componentfeatures", 1);
  for (const char **p = supports; *p; p++)
    OX::addChild(cfx, "supportsinterface", 2, NULL, "repid", repid(s, *p), "supportsname", *p);
  ezxml_t isx = OX::addChild(root, "interfaces", 1); // ditto
  for (const char **p = supports; *p; p++) {
    ezxml_t ix =
      OX::addChild(isx, "interface", 2, NULL, "name", *p, "repid", repid(s, *p));
    if (!strcmp(*p, "Device"))
      OX::addChild(ix, "inheritsinterface", 3, NULL, "repid", repid(s, "Resource"));
    if (!strcmp(*p, "Resource"))
      for (const char **p1 = supports + 1; *p1; p1++)
	OX::addChild(ix, "inheritsinterface", 3, NULL, "repid", repid(s, *p1));
  }  
  ezxml_t psx = OX::addChild(cfx, "ports", 2); // this is required per the DTD even if empty
  std::set<std::string> portInterfaces;
  for (OU::Assembly::ConnectionsIter ci = m_assembly.m_connections.begin();
       ci != m_assembly.m_connections.end(); ci++)
    for (OU::Assembly::ExternalsIter ei = (*ci).m_externals.begin();
	 ei != (*ci).m_externals.end(); ei++)
      for (OU::Assembly::Connection::PortsIter pi = (*ci).m_ports.begin();
	   pi != (*ci).m_ports.end(); pi++) {
	OU::Assembly::Role &r = (*pi).m_role;
	assert(r.m_knownRole && !r.m_bidirectional);
	const char *type;
	OU::Port *p;
	for (unsigned n=0; (p=getMetaPort(n)); n++) {
	  size_t nOps = p->nOperations();
	  // For now we only support 1 operation
	  // For now we just use the first operation
	  assert( nOps == 1 );
	  OU::Operation & op = p->operations()[0];
	  //	  printf("Member name = %s\n",  op.args()[0].m_name.c_str() );
	  type = mapType(op.args()[0].m_baseType);
	  //	  printf("Type = %s\n",  type );
	}
	ezxml_t px = OX::addChild(psx, r.m_provider ? "provides" : "uses", 3, NULL,
				   "repid", repid(s, type, "BULKIO"),
				   r.m_provider ? "providesname" : "usesname",
				   pi->m_name.c_str());
	OX::addChild(px, "porttype", 4, NULL, "type", "data");
	if (portInterfaces.insert(type).second) {
	  if (portInterfaces.insert("ProvidesPortStatisticsProvider").second)
	    OX::addChild(isx, "interface", 2, NULL, "name", "ProvidesPortStatisticsProvider",
			  "repid", repid(s, "ProvidesPortStatisticsProvider", "BULKIO"));
	  if (portInterfaces.insert("updateSRI").second)
	    OX::addChild(isx, "interface", 2, NULL, "name", "updateSRI",
			  "repid", repid(s, "updateSRI", "BULKIO"));
	  ezxml_t ix = OX::addChild(isx, "interface", 2, NULL, "name", type,
				     "repid", repid(s, type, "BULKIO"));
	  OX::addChild(ix, "inheritsinterface", 3, NULL,
			"repid", repid(s, "ProvidesPortStatisticsProvider", "BULKIO"));
	  OX::addChild(ix, "inheritsinterface", 3, NULL,
			"repid", repid(s, "updateSRI", "BULKIO"));
	}
   }
#if 0
  const char *xml = ezxml_toxml(root);
  if (fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<!DOCTYPE softwarecomponent PUBLIC \"-//JTRS//DTD SCA V2.2.2 SCD//EN\" "
	    "\"softwarecomponent.dtd\">\n",
	    f) == EOF ||
      fputs(xml, f) == EOF || fclose(f))
    throw OU::Error("Error closing PRF output file: \"%s\"  No space?", path.c_str());
#else
  SCA::writeXml(root, f, "SCD", "softwarecomponent", path);
#endif
}
  }
}
