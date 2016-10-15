#include <unordered_set>
#include "OcpiUtilUUID.h"
#include "OcpiApplication.h"
#include "cdkutils.h"
#include "utils.h"

namespace OCPI {
  namespace API {
    namespace OU = OCPI::Util;
    namespace SCA = OCPI::SCA;
    namespace OC = OCPI::Container;

static const char *repid(std::string &s, const char *name, const char *module = NULL) {
  return OU::format(s, "IDL:%s/%s:1.0", module ? module : "CF", name);
}

void ApplicationI::
genScaScd(const char *outDir) {
  FILE *f;
  std::string path;
  const char *err = openOutput(m_assembly.name().c_str(), outDir, "", ".scd", ".xml", NULL, f,
			       &path);
  if (err)
    throw OU::Error("Error opening SCD output file: %s", err);
  std::string s;
  ezxml_t root = ezxml_new("softwarecomponent");
  SCA::addChild(root, "corbaversion", 1, "2.2");
  SCA::addChild(root, "componentrepid", 1, NULL, "repid", repid(s, "Resource"));
  SCA::addChild(root, "componenttype", 1, "resource");
  const char *supports[] =
    { "Resource", "LifeCycle", "PortSupplier", "PortSet", "PropertySet", "TestableObject", NULL};
  ezxml_t cfx = SCA::addChild(root, "componentfeatures", 1);
  for (const char **p = supports; *p; p++)
    SCA::addChild(cfx, "supportsinterface", 2, NULL, "repid", repid(s, *p), "supportsname", *p);
  ezxml_t isx = SCA::addChild(root, "interfaces", 1); // ditto
  for (const char **p = supports; *p; p++) {
    ezxml_t ix =
      SCA::addChild(isx, "interface", 2, NULL, "name", *p, "repid", repid(s, *p));
    if (!strcmp(*p, "Device"))
      SCA::addChild(ix, "inheritsinterface", 3, NULL, "repid", repid(s, "Resource"));
    if (!strcmp(*p, "Resource"))
      for (const char **p1 = supports + 2; *p1; p1++)
	SCA::addChild(ix, "inheritsinterface", 3, NULL, "repid", repid(s, *p1));
  }  
  ezxml_t psx = SCA::addChild(cfx, "ports", 2); // this is required per the DTD even if empty
  std::set<std::string> portInterfaces;
  for (OU::Assembly::ConnectionsIter ci = m_assembly.m_connections.begin();
       ci != m_assembly.m_connections.end(); ci++)
    for (OU::Assembly::ExternalsIter ei = (*ci).m_externals.begin();
	 ei != (*ci).m_externals.end(); ei++)
      for (OU::Assembly::Connection::PortsIter pi = (*ci).m_ports.begin();
	   pi != (*ci).m_ports.end(); pi++) {
	OU::Assembly::Role &r = (*pi).m_role;
	assert(r.m_knownRole && !r.m_bidirectional);
	const char *type = "dataDouble";
	ezxml_t px = SCA::addChild(psx, r.m_provider ? "provides" : "uses", 3, NULL,
				   "repid", repid(s, type, "BULKIO"),
				   r.m_provider ? "providesname" : "usesname",
				   pi->m_name.c_str());
	SCA::addChild(px, "porttype", 4, NULL, "type", "data");
	if (portInterfaces.insert(type).second) {
	  if (portInterfaces.insert("ProvidesPortStatisticsProvider").second)
	    SCA::addChild(isx, "interface", 2, NULL, "name", "ProvidesPortStatisticsProvider",
			  "repid", repid(s, "ProvidesPortStatisticsProvider", "BULKIO"));
	  if (portInterfaces.insert("updateSRI").second)
	    SCA::addChild(isx, "interface", 2, NULL, "name", "updateSRI",
			  "repid", repid(s, type, "BULKIO"));
	  ezxml_t ix = SCA::addChild(isx, "interface", 2, NULL, "name", type,
				     "repid", repid(s, type, "BULKIO"));
	  SCA::addChild(ix, "inheritsinterface", 3, NULL,
			"repid", repid(s, "ProvidesPortStatisticsProvider", "BULKIO"));
	  SCA::addChild(ix, "inheritsinterface", 3, NULL,
			"repid", repid(s, "updateSRI", "BULKIO"));
	}
      }
  const char *xml = ezxml_toxml(root);
  if (fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<!DOCTYPE softwarecomponent PUBLIC \"-//JTRS//DTD SCA V2.2.2 SCD//EN\" "
	    "\"softwarecomponent.dtd\">\n",
	    f) == EOF ||
      fputs(xml, f) == EOF || fclose(f))
    throw OU::Error("Error closing PRF output file: \"%s\"  No space?", path.c_str());
}
  }
}
