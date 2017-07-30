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

// This file is for corba-related utility functions exposed in the API
#ifndef _OCPICORBAAPI_H__
#define _OCPICORBAAPI_H__

namespace OCPI {
  namespace API {
    // Convert the stringified object reference to the corbaloc: format.
    // If already corbaloc, then return that
    void ior2corbaloc(const char *ior, std::string &corbaloc) throw (std::string);
    inline void ior2corbaloc(const std::string &ior, std::string &corbaloc) {
      ior2corbaloc(ior.c_str(), corbaloc);
    }
  }
}

#endif
