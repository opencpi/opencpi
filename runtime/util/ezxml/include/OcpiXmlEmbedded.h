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

#ifndef OCPI_XML_EMBEDDED_H
#define OCPI_XML_EMBEDDED_H

#include <string>

namespace OCPI {
  namespace Util {
    namespace EzXml {
      // Adds XML metadata string to end of artifact file:
      void artifact_addXML(const std::string &fname, const std::string &xml);
      // Will query XML metadata string from end of artifact file:
      void artifact_getXML(const std::string &fname, std::string &xml);
      // Will remove XML metadata from end of artifact file:
      bool artifact_stripXML(const std::string &fname);
    }
  }
}

#endif
