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

void ApplicationI::
genScaSpd(const char *outDir, const char *pkg) const {
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
  std::string name;
  if (pkg) {
    name = pkg;
    name += ".";
  }
  name += m_assembly.name();
  ezxml_set_attr_d(root, "name", name.c_str());
  ezxml_set_attr(root, "type", "sca_compliant");
  OX::addChild(root, "author", 1);
  ezxml_t x = OX::addChild(root, "propertyfile", 1, NULL, "type", "PRF");
  OX::addChild(x, "localfile", 2, NULL, "name", (m_assembly.name() + ".prf.xml").c_str());
  x = OX::addChild(root, "descriptor", 1);
  OX::addChild(x, "localfile", 2, NULL, "name", (m_assembly.name() + ".scd.xml").c_str());
#if 0
  id = "DCE:";
  uuid = OU::UUID::produceRandomUUID();
  id += OU::UUID::binaryToHex(uuid);
#else
  id = "cpp";
#endif
  x = OX::addChild(root, "implementation", 1, NULL, "id", id.c_str());
  ezxml_t cx = OX::addChild(x, "code", 2, NULL, "type", "Executable");
  OX::addChild(cx, "localfile", 3, NULL, "name", ("cpp/" + m_assembly.name()).c_str());
  //  OU::format(s, "artifacts/%s.xml", m_assembly.name().c_str());
  OX::addChild(cx, "entrypoint", 3, ("cpp/" + m_assembly.name()).c_str());
  OX::addChild(x, "os", 2, NULL, "name", "Linux");
  ezxml_t c = 
    OX::addChild(x, "compiler", 2, NULL, "name", "/usr/bin/gcc");
  ezxml_set_attr(c, "version", "4.4.7");
  OX::addChild(x, "programminglanguage", 2, NULL, "name", "C++");
  OX::addChild(x, "processor", 2, NULL, "name", "x86_64");
#if 0
  cx = OX::addChild(x, "dependency", 2, NULL, "type", "runtime_requirements");
  cx = OX::addChild(cx, "softpkgref", 3, NULL, NULL, NULL);
  OX::addChild(cx, "localfile", 4, NULL, "name", "libs/libs.spd.xml");
  OX::addChild(cx, "implref", 4, NULL, "refid", "cpp");
  const char *xml = ezxml_toxml(root);
  if (fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<!DOCTYPE softpkg PUBLIC \"-//JTRS//DTD SCA V2.2.2 SPD//EN\" \"softpkg.dtd\">\n",
	    f) == EOF ||
      fputs(xml, f) == EOF || fclose(f))
    throw OU::Error("Error closing PRF output file: \"%s\"  No space?", path.c_str());
#else
  SCA::writeXml(root, f, "SPD", "softpkg", path);
#endif
}
  }
}
