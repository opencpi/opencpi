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

#include <iostream>
#include <string>

namespace OCPI {
  namespace Util {
    namespace EzXml {
      // Object that holds other "stuff" callers might care about an artifact file:
      class extra_t {
      public:
        extra_t() { init(); }
        off_t filesize; // size of entire artifact file (including metadata)
        struct timespec timestamp; // timestamp of artifact file
        struct { off_t start, length; } metadata; // location within, and length of, metadata
        void init() { timestamp = {0,0}; filesize = metadata.start = metadata.length = -1; }
      private:
        extra_t( const extra_t& );
        extra_t& operator=( const extra_t& );
      };

      // Adds XML metadata string to end of artifact file:
      void artifact_addXML(const std::string &fname, const std::string &xml);
      // Will query XML metadata string from end of artifact file:
      bool artifact_getXML(const std::string &fname, std::string &xml, extra_t * = NULL);
      // Will remove XML metadata from end of artifact file on filesystem:
      bool artifact_stripXML(const std::string &fname);
      // Will remove XML metadata from end of artifact file and stream payload:
      void artifact_getPayload(const std::string &fname, std::ostream &os = std::cout);
    }
  }
}

#endif
