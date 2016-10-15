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

void ApplicationI::
genScaSpd(const char *outDir) {
  FILE *f;
  std::string path;
  const char *err = openOutput(m_assembly.name().c_str(), outDir, "", ".spd", ".xml", NULL, f,
			       &path);
  if (err)
    throw OU::Error("Error opening SCD output file: %s", err);
  std::string s;
  std::string id = "DCE:";
  OU::UUID::BinaryUUID uuid = OU::UUID::produceRandomUUID();
  id += OU::UUID::binaryToHex(uuid);
  ezxml_t root = ezxml_new("softpkg");
  ezxml_set_attr_d(root, "id", id.c_str());
  ezxml_set_attr_d(root, "name", m_assembly.name().c_str());
  ezxml_set_attr(root, "type", "sca_compliant");
  SCA::addChild(root, "author", 1);
  ezxml_t x = SCA::addChild(root, "propertyfile", 1, NULL, "type", "PRF");
  SCA::addChild(x, "localfile", 2, NULL, "name", (m_assembly.name() + ".prf.xml").c_str());
  x = SCA::addChild(root, "descriptor", 1);
  SCA::addChild(x, "localfile", 2, NULL, "name", (m_assembly.name() + ".scd.xml").c_str());
  id = "DCE:";
  uuid = OU::UUID::produceRandomUUID();
  id += OU::UUID::binaryToHex(uuid);
  x = SCA::addChild(root, "implementation", 1, NULL, "id", id.c_str());
  ezxml_t cx = SCA::addChild(x, "code", 2, NULL, "type", "SharedLibrary");
  SCA::addChild(cx, "localfile", 3, NULL, "name", "artifacts");
  OU::format(s, "artifacts/%s.xml", m_assembly.name().c_str());
  SCA::addChild(cx, "entrypoint", 3, s.c_str());
  SCA::addChild(x, "os", 2, NULL, "name", "Linux");
  ezxml_t c = 
    SCA::addChild(x, "compiler", 2, NULL, "name", "/usr/bin/gcc");
  ezxml_set_attr(c, "version", "4.4.7");
  SCA::addChild(x, "programminglanguage", 2, NULL, "name", "C++");

  SCA::addChild(x, "processor", 2, NULL, "name", "x86_64");
  const char *xml = ezxml_toxml(root);
  if (fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<!DOCTYPE softpkg PUBLIC \"-//JTRS//DTD SCA V2.2.2 SPD//EN\" \"softpkg.dtd\">\n",
	    f) == EOF ||
      fputs(xml, f) == EOF || fclose(f))
    throw OU::Error("Error closing PRF output file: \"%s\"  No space?", path.c_str());
}
  }
}
