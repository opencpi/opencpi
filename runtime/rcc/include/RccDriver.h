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

#ifndef RCC_DRIVER_H
#define RCC_DRIVER_H
#include <pthread.h>
#include "ContainerManager.h"

namespace OCPI {
  namespace RCC {
    extern const char *rcc;
    class Container;
    class Driver : public OCPI::Container::DriverBase<Driver, Container, rcc> {
      friend class Container;
    protected:
      std::string m_platform;
    public:
      static pthread_key_t s_threadKey;
      Driver() throw();
      ~Driver() throw();
      OCPI::Container::Container *
	probeContainer(const char *which, std::string &error, const OCPI::API::PValue *props)
	throw ( OCPI::Util::EmbeddedException );
      // Per driver discovery routine to create devices
      unsigned search(const OCPI::API::PValue*, const char **exclude, bool discoveryOnly);
      void configure(ezxml_t x);
    };
  }
}
#endif
