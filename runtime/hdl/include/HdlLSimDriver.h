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

#ifndef HDLLSIMDRIVER_H
#define HDLLSIMDRIVER_H
#include "HdlNetDriver.h"

namespace OCPI {
  namespace HDL {
    namespace LSim {
      const char
	TMPDIR[] = "/tmp",
	SIMDIR[] = "ocpi",
	SIMPREF[] =  "sim";
      class Device;
      class Driver {
	friend class Device;
      protected:
	virtual ~Driver();
      private:
	Device *createDevice(const std::string &name, const std::string &platform,
			     uint8_t spinCount, unsigned sleepUsecs, unsigned simTicks,
			     const OCPI::Util::PValue *params, bool dump, const char *dir, std::string &error);
      public:
	unsigned
	search(const OCPI::Util::PValue *props, const char **exclude, bool discoveryOnly,
	       std::string &error);
	OCPI::HDL::Device *
	open(const char *name, const OCPI::API::PValue *params, std::string &err);
	// Callback when found
	virtual bool found(OCPI::HDL::Device &dev, const char **excludes, bool discoveryOnly,
			   std::string &error) = 0;
      };
    }
  }
}
#endif
