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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include "OcpiOsFileSystem.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "OcpiLibraryManager.h"
#include "OcpiDriverManager.h"
#include "cdkutils.h"
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OL = OCPI::Library;
namespace OS = OCPI::OS;
namespace OX = OCPI::Util::EzXml;
// OpenCPI GNU Radio utility
// Generate blocks corresponding to available artifacts
// After options args are platforms to engage or "all"
#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpigr [options]\n" \
  "At least one option must be specified\n" \

#define OCPI_OPTIONS \
  CMD_OPTION  (verbose,   v,    Bool,   NULL, "Be verbose") \
  CMD_OPTION  (directory, D,    String, ".", "Specify the directory in which to put output generated files") \
  /**/
#include "CmdOption.h"
static const char *bad(OU::Worker &w, const char *tag, const char *val) {
  fprintf(stderr, "Bad worker %s: can't map %s %s\n", w.cname(), tag, val);
  return "raw";
}

#define AUTO_PLATFORM_COLOR "#afaf75"

// "Converts" an arbitrary string to some arbitrary color via hashing.
//
// Note: The total set of platforms encountered during parsing on a given system
// is dynamic as it is based on what the workers have been built for, however,
// we do want the colors to be consistent from one user to the other, from one run
// to the other. Indexing into a static list of colors would cause each user to have
// different colors for their platforms, or even different for a single user after
// running this script after building for a new platform. We could use a static
// mapping of all known platforms to colors, but it would be difficult to maintain.
// This hash function maps the ID of the platform to a calculated color, so it is the
// same every time for every run while supporting new platforms with minimal chances of
// collision.
static std::string stringToColorHash(std::string str) {
  unsigned int hash = 0;
  for(size_t i = 0; i < str.size(); i++) {
    hash = str[i] + ((hash << 5) - hash);
  }
  std::string color;
  OU::format(color, "#%02x%02x%02x", (hash & 0x0000FF), (hash & 0x00FF00) >> 8, (hash & 0xFF0000) >> 16);
  return color;
}

// For each spec, which platforms are implemented by found workers in artifacts?
std::map<std::string, StringSet> specToPlatforms;
// For each platform, which model does it support
std::map<std::string, std::string> platformToModel;
// For each worker, add worker specific params
std::map<std::string, std::map<std::string, std::set<OU::Property *>>> workerSpecificProperties;
StringSet specs;
struct Category {
  std::string name;
  StringSet blocks;
  std::vector<Category> categories;
  void addWorker(const char *path, std::string &a_name) {
    if (!path || !*path) {
      blocks.insert(a_name);
      return;
    }
    std::string cname = path;
    const char *cp = strchr(path, '/');
    if (cp)
      cname.assign(path, cp++ - path);
    Category *cat = NULL;
    for (unsigned n = 0; n < categories.size(); n++)
      if (categories[n].name == cname) {
	cat = &categories[n];
	break;
      }
    if (!cat) {
      categories.resize(categories.size() + 1);
      cat = &categories.back();
      cat->name = cname;
    }
    cat->addWorker(cp, a_name);
  }
  ezxml_t getXml(ezxml_t x = NULL, unsigned level = 0) {
    if (!x)
      x = ezxml_new("cat");
    OX::addChild(x, "name", level + 1, name.c_str());
    for (unsigned n = 0; n < categories.size(); n++)
      categories[n].getXml(OX::addChild(x, "cat", level + 1), level + 1);
    for (StringSetIter it = blocks.begin(); it != blocks.end(); ++it)
      OX::addChild(x, "block", level + 1, it->c_str());
    if (name == "ocpi")
      OX::addChild(x, "block", level + 1, "variable_ocpi_container");
    return x;
  }
} top;

static void addProperty(OU::Worker &w, ezxml_t & root, OU::Property *p, __attribute__((unused)) std::string workerName="") {
    ezxml_t px = OX::addChild(root, "param", 1);
    OX::addChild(px, "name", 2, p->pretty());
    OX::addChild(px, "key", 2, p->cname());
    std::string strval;
    if (p->m_default)
      p->m_default->unparse(strval);
    const char *type;
    bool isVector = p->m_arrayRank || p->m_isSequence;
    switch (p->m_baseType) {
    case OA::OCPI_Bool:
      type = isVector ? bad(w, "property", p->cname()) : "bool";
      if (strval.length() > 1)
	strval[0] = (char)toupper(strval[0]);
      break;
    case OA::OCPI_Double:  type = isVector ? "real_vector" : "real"; break;
    case OA::OCPI_Float:  type = isVector ? "float_vector" : "float"; break;
    case OA::OCPI_Char:
    case OA::OCPI_Short:
    case OA::OCPI_Long:
    case OA::OCPI_UChar:
    case OA::OCPI_ULong:
    case OA::OCPI_UShort:
    case OA::OCPI_LongLong:
    case OA::OCPI_ULongLong:
      type = isVector ? "int_vector" : "int"; break;
    case OA::OCPI_String:  type = isVector ? bad(w, "property", p->cname()) : "string"; break;
    case OA::OCPI_Enum:
      type = "enum";
      break;
    case OA::OCPI_Struct:
    case OA::OCPI_Type:
    default:
      type = bad(w, "property", p->cname()); break;
    }
    if (strval.size())
      OX::addChild(px, "value", 2, strval.c_str());
    OX::addChild(px, "type", 2, type);

    if ((!p->m_isWritable && !p->m_isInitial) || p->m_isParameter) {
        OX::addChild(px, "hide", 2, "none");
        OX::addChild(px, "build_param", 2, "True");
    }

    if (p->m_baseType == OA::OCPI_Enum) {
      for (const char **ep = p->m_enums; *ep; ep++) {
        ezxml_t ox = OX::addChild(px, "option", 3);
        OX::addChild(ox, "name", 4, *ep);
        OX::addChild(ox, "key", 4, *ep);
      }
    }

    // Due to the XML schema, this has to be here and not above where
    // hide/build_param is set
    if (p->m_isParameter) {
      OX::addChild(px, "tab", 2, "Parameters");
    } else if (!p->m_isWritable && !p->m_isInitial) {
      OX::addChild(px, "tab", 2, "Read Only");
    }

    if (p->m_isInitial) {
      OX::addChild(px, "required", 2, "True");
    }
}

static void doWorker(OU::Worker &w) {
  const char *err;
  //  printf("spec:  %-30s  worker:  %s\n", w.specName().c_str(), w.name().c_str());
  if (!specs.insert(w.specName()).second)
    return;

  ezxml_t root = ezxml_new("block");
  const char *cp = strrchr(w.specName().c_str(), '.');
  OX::addChild(root, "name", 1, cp ? cp + 1 : w.specName().c_str());
  std::string
    path(w.specName().c_str(), cp - w.specName().c_str()),
    cat("[OpenCPI]/" + path),
    key(w.specName()),
    import("import ocpi");
  for (unsigned n = 0; n < key.size(); n++)
    if (key[n] == '.')
      key[n] = '_';
  for (unsigned n = 0; n < path.size(); n++)
    if (path[n] == '.')
      path[n] = '/';
  top.addWorker(path.c_str(), key);
  OX::addChild(root, "key", 1, key.c_str());
  OX::addChild(root, "category", 1, cat.c_str());
  OX::addChild(root, "import", 1, import.c_str());
  OX::addChild(root, "make", 1, "");
  {
    unsigned np;
    OU::Property *p = w.properties(np);
    for (unsigned n = 0; n < np; n++, p++) {
      if(p->m_isWritable && !p->m_isInitial) {
        OX::addChild(root, "callback", 1, ("self._ocpi_application_internal_black_box_0.set_property(\"$(id)\", \"" +
                           p->m_name + "\", str($" + p->m_name + "))").c_str());
      }
    }
  }
  // Add ocpi package as a partially hidden parameter
  ezxml_t _px = OX::addChild(root, "param", 1);
  OX::addChild(_px, "name", 2, "ocpi_spec");
  OX::addChild(_px, "key", 2, "ocpi_spec");
  OX::addChild(_px, "value", 2, w.specName().c_str());
  OX::addChild(_px, "type", 2, "string");
  OX::addChild(_px, "hide", 2, "part");

  _px = OX::addChild(root, "param", 1);
  OX::addChild(_px, "name", 2, "Container");
  OX::addChild(_px, "key", 2, "container");
  OX::addChild(_px, "value", 2, "auto");
  OX::addChild(_px, "type", 2, "enum");
  OX::addChild(_px, "hide", 2, "none");

  ezxml_t _ox = OX::addChild(_px, "option", 3);
  OX::addChild(_ox, "name", 4, "auto");
  OX::addChild(_ox, "key", 4, "auto");
  StringSet &platforms = specToPlatforms[w.specName()];
  for (StringSetIter it = platforms.begin(); it != platforms.end(); ++it) {
    _ox = OX::addChild(_px, "option", 3);
    OX::addChild(_ox, "name", 4, it->c_str());
    OX::addChild(_ox, "key", 4, it->c_str());
  }

  if (!w.slaves().empty()) {
    _px = OX::addChild(root, "param", 1);
    OX::addChild(_px, "name", 2, "Slave");
    OX::addChild(_px, "key", 2, "slave");
    OX::addChild(_px, "value", 2, "");
    OX::addChild(_px, "type", 2, "string");
    OX::addChild(_px, "hide", 2, "none");
    OX::addChild(_px, "required", 2, "True");
  }

  // Add all of the Component specific properties
  unsigned np;
  OU::Property *p = w.properties(np);
  for (unsigned n = 0; n < np; n++, p++) {
    // Worker specific properties will be added later so skip them
    // for now so they don't get added twice.
    if (!p->m_isImpl) {
      addProperty(w, root, p);
    }
  }

  // Add all of the properties specific to a particular worker
  std::map<std::string, std::set<OU::Property *>>::const_iterator wsp = workerSpecificProperties[w.specName()].begin();
  while (wsp != workerSpecificProperties[w.specName()].end()) {
    std::set<OU::Property *>::const_iterator prop = wsp->second.begin();
    while (prop != wsp->second.end()) {
      addProperty(w, root, *prop, wsp->first);
      prop++;
    }
    wsp++;
  }

  OU::Port *ports = w.ports(np);
  for (unsigned n = 0; n < np; n++, ports++) {
    OU::Port &port = *ports;
    ezxml_t px = OX::addChild(root, port.m_provider ? "sink" : "source", 1);
    OX::addChild(px, "name", 2, port.cname());
    OX::addChild(px, "type", 2, "ocpi");
    OX::addChild(px, "domain", 2, "$platform");
    if (port.m_isOptional) {
      OX::addChild(px, "optional", 2, "1");
    }
    OX::addChild(px, "protocol", 2, port.OU::Protocol::cname());
  }

  std::string
    xml = ezxml_toxml(root),
    file;
  OU::format(file, "%s/%s-grc.xml", options.directory(), key.c_str());
  xml += "\n";
  if ((err = OU::string2File(xml, file)))
    throw OU::Error("error writing GRC XML file:  %s", err);
}

static void containerBlock() {
  std::string file;
  OU::format(file, "%s/ocpi_container.xml", options.directory());

  ezxml_t root = ezxml_new("block");
  OX::addChild(root, "name", 1, "OpenCPI Container");
  OX::addChild(root, "key", 1, "variable_ocpi_container");
  OX::addChild(root, "make", 1, "");

  ezxml_t px = OX::addChild(root, "param", 1);
  OX::addChild(px, "name", 2, "ocpi_spec");
  OX::addChild(px, "key", 2, "ocpi_spec");
  OX::addChild(px, "value", 2, "");
  OX::addChild(px, "type", 2, "string");
  OX::addChild(px, "hide", 2, "all");

  px = OX::addChild(root, "param", 1);
  OX::addChild(px, "name", 2, "Platform");
  OX::addChild(px, "key", 2, "value");
  OX::addChild(px, "type", 2, "enum");

  for (auto pi = platformToModel.begin(); pi != platformToModel.end(); ++pi) {
    ezxml_t ox = OX::addChild(px, "option", 2);
    OX::addChild(ox, "name", 1, pi->first.c_str());
    OX::addChild(ox, "key", 1, pi->first.c_str());
  }

  px = OX::addChild(root, "param", 1);
  OX::addChild(px, "name", 2, "value");
  OX::addChild(px, "key", 2, "container");
  OX::addChild(px, "value", 2, "$value");
  OX::addChild(px, "type", 2, "string");
  OX::addChild(px, "hide", 2, "all");

  std::string xml = "<?xml version=\"1.0\"?>";
  xml += ezxml_toxml(root);
  const char *err;
  if ((err = OU::string2File(xml, file)))
	  throw OU::Error("error writing GRC XML file:  %s", err);
}

static void doWorkerPlatform(OU::Worker &w) {
  assert(w.attributes().platform().length());
  specToPlatforms[w.specName()].insert(w.attributes().platform());
  platformToModel[w.attributes().platform()] = w.model();

  unsigned np;
  OU::Property *p = w.properties(np);
  for (unsigned n = 0; n < np; n++, p++) {
    if (p->m_isImpl) {
      // TODO. This isn't working.
      workerSpecificProperties[w.specName()][w.cname()].insert(p);
    }
  }
}

static int
mymain(const char ** /*ap*/) {
  setenv("OCPI_SYSTEM_CONFIG", "", 1);
  OCPI::Driver::ManagerManager::suppressDiscovery();
  OL::getManager().enableDiscovery();
  OS::FileSystem::mkdir(options.directory(), true);
  top.name = "[OpenCPI]";
  platformToModel["auto"] = "auto";
  OL::getManager().doWorkers(doWorkerPlatform);
  OL::getManager().doWorkers(doWorker);
  std::string file;
  OU::format(file, "%s/ocpi_block_tree.xml", options.directory());
  std::string xml = "<?xml version=\"1.0\"?>\n";
  xml += ezxml_toxml(top.getXml());
  xml += "\n";
  OU::string2File(xml, file);

  for (auto pi = platformToModel.begin(); pi != platformToModel.end(); ++pi) {
    file = "";
    OU::format(file, "%s/ocpi_%s_domain.xml", options.directory(), pi->first.c_str());
    xml = "<?xml version=\"1.0\"?>\n";
    ezxml_t root = ezxml_new("domain");
    OX::addChild(root, "name", 1, pi->first.c_str());
    OX::addChild(root, "key", 1, pi->first.c_str());
    OX::addChild(root, "model", 1, pi->second.c_str());
    OX::addChild(root, "color", 1,
		 pi->first == "auto" ? AUTO_PLATFORM_COLOR :
		 stringToColorHash(pi->first).c_str());
    OX::addChild(root, "multiple_sinks", 1, "False");
    OX::addChild(root, "multiple_sources", 1, "False");
    for (auto other = platformToModel.begin(); other != platformToModel.end(); ++other) {
      ezxml_t px = OX::addChild(root, "connection", 1);
      OX::addChild(px, "source_domain", 2, pi->first.c_str());
      OX::addChild(px, "sink_domain", 2, other->first.c_str());
      OX::addChild(px, "make", 2, "");
    }
    ezxml_t px = OX::addChild(root, "connection", 1);
    OX::addChild(px, "source_domain", 2, pi->first.c_str());
    OX::addChild(px, "sink_domain", 2, "gr_message");
    OX::addChild(px, "make", 2, "");
    px = OX::addChild(root, "connection", 1);
    OX::addChild(px, "source_domain", 2, "gr_message");
    OX::addChild(px, "sink_domain", 2, pi->first.c_str());
    OX::addChild(px, "make", 2, "");
    xml += ezxml_toxml(root);
    xml += "\n";
    OU::string2File(xml, file);
  }
  containerBlock();
  return 0;
}
