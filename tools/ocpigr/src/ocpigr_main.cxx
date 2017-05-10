#include <cstdio>
#include <cstdlib>
#include <cstring>
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
  "Usage syntax is: ocpigr [options] <xml-worker-descriptor-file>\n" \
  "After options, the single argument is the name of the XML file to parse as input.\n" \
  "This argument can exclude the .xml suffix, which will be assumed.\n" \
  "XML file can be a worker, an assembly, a platform configuration, or a container.\n"
#define OCPI_OPTIONS \
  CMD_OPTION  (verbose,   v,    Bool,   NULL, "Be verbose") \
  CMD_OPTION  (directory, D,    String, NULL, "Specify the directory in which to put output generated files") \
  /**/
#include "CmdOption.h"
static const char *bad(OU::Worker &w, const char *tag, const char *val) {
  fprintf(stderr, "Bad worker %s: can't map %s %s\n", w.cname(), tag, val);
  return "raw";
}
StringSet specs;
struct Category {
  std::string name;
  StringSet blocks;
  std::vector<Category> categories;
  void addWorker(const char *path, std::string &name) {
    if (!path || !*path) {
      blocks.insert(name);
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
    cat->addWorker(cp, name);
  }
  ezxml_t getXml(ezxml_t x = NULL, unsigned level = 0) {
    if (!x)
      x = ezxml_new("cat");
    OX::addChild(x, "name", level + 1, name.c_str());
    for (StringSetIter it = blocks.begin(); it != blocks.end(); ++it)
      OX::addChild(x, "block", level + 1, it->c_str());
    for (unsigned n = 0; n < categories.size(); n++)
      categories[n].getXml(OX::addChild(x, "cat", level + 1), level + 1);
    return x;
  }
} top;
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
    import("from opencpi import " + path);
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
  //  OX::addChild(root, "make", 1, "WTFIT");
  unsigned np;
  OU::Property *p = w.properties(np);
  for (unsigned n = 0; n < np; n++, p++) {
    ezxml_t px = OX::addChild(root, "param", 1);
    OX::addChild(px, "name", 2, p->pretty());
    OX::addChild(px, "key", 2, p->cname());
    const char *type;
    bool isVector = p->m_arrayRank || p->m_isSequence;
    switch (p->m_baseType) {
    case OA::OCPI_Bool:  type = isVector ? bad(w, "property", p->cname()) : "bool"; break;
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
    case OA::OCPI_Enum:  type = "enum"; break;
    case OA::OCPI_Struct:
    case OA::OCPI_Type:
    default:
      type = bad(w, "property", p->cname()); break;
    }
    OX::addChild(px, "type", 2, type);
    if (p->m_baseType == OA::OCPI_Enum)
      for (const char **ep = p->m_enums; *ep; ep++) {
	ezxml_t ox = OX::addChild(px, "option", 3);
	OX::addChild(ox, "name", 4, *ep);
	OX::addChild(ox, "key", 4, *ep);
      }
  }
  OU::Port *ports = w.ports(np);
  for (unsigned n = 0; n < np; n++, ports++) {
    OU::Port &p = *ports;
    ezxml_t px = OX::addChild(root, p.m_provider ? "sink" : "source", 1);
    OX::addChild(px, "name", 2, p.cname());
    if (p.nOperations() != 1 || p.operations()[0].nArgs() != 1) {
      bad(w, "port", p.cname());
      continue;
    }
    OU::Member &arg = p.operations()[0].args()[0];
    if ((arg.m_isSequence || arg.m_arrayRank) && arg.m_baseType != OA::OCPI_Struct &&
	arg.m_baseType != OA::OCPI_Type && arg.m_baseType != OA::OCPI_String) {
      const char *type;
      switch (arg.m_baseType) {
      case OA::OCPI_Bool:  type = "bool"; break;
      case OA::OCPI_Double:  type = "real"; break;
      case OA::OCPI_Float:  type = "float"; break;
      case OA::OCPI_Char:
      case OA::OCPI_Short:
      case OA::OCPI_Long: 
      case OA::OCPI_UChar:
      case OA::OCPI_ULong:
      case OA::OCPI_UShort:
      case OA::OCPI_LongLong:
      case OA::OCPI_ULongLong:
	type = "int"; break;
      case OA::OCPI_String:  type = "string"; break;
      case OA::OCPI_Enum:  type = "enum"; break;
      case OA::OCPI_Struct:
      case OA::OCPI_Type:
      default:
	type = bad(w, "port", p.cname()); break;
      }
      OX::addChild(px, "type", 2, type);
    } else
      bad(w, "port", p.cname());
  }
  std::string
    xml = ezxml_toxml(root),
    file;
  OU::format(file, "%s/%s-grc.xml", options.directory(), key.c_str());
  xml += "\n";
  if ((err = OU::string2File(xml, file)))
    throw OU::Error("error writing GRC XML file:  %s", err);
}
static int mymain(const char **/*ap*/) {
  setenv("OCPI_SYSTEM_CONFIG", "", 1);
  OCPI::Driver::ManagerManager::suppressDiscovery();
  OL::getManager().enableDiscovery();
  top.name = "[OpenCPI]";
  OL::getManager().doWorkers(doWorker);
  std::string file;
  OU::format(file, "%s/block_tree.xml", options.directory());
  std::string xml = "<?xml version=\"1.0\"?>\n";
  xml += ezxml_toxml(top.getXml());
  xml += "\n";
  OU::string2File(xml, file);
  return 0;
}
