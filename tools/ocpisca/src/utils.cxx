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

#include <string>
#include <cstdio>
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "utils.h"

namespace OU = OCPI::Util;
namespace OCPI {
  namespace SCA {
    void
    writeXml(ezxml_t root, FILE *f, const char *type, const char *dtd, const std::string &path) {
      const char *xml = ezxml_toxml(root);
      if (fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		  "<!DOCTYPE %s PUBLIC \"-//JTRS//DTD SCA V2.2.2 %s//EN\" "
		  "\"%s.dtd\">\n", dtd, type, dtd) == EOF ||
	  fputs(xml, f) == EOF || fputc('\n', f) == EOF || fclose(f))
	throw OU::Error("Error closing %s output file: \"%s\"  No space?", type, path.c_str());
    }
  }
}
