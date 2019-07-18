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

#ifndef HDLDUMMYWORKER_H
#define HDLDUMMYWORKER_H
#include "ContainerPort.h"
#include "HdlWciControl.h"
#include "OcpiUtilEzxml.h"
namespace OCPI {
  namespace HDL {
    // We create a worker class that does not have to be part of anything else.
    class DummyWorker : public OCPI::Container::Worker, public OCPI::HDL::WciControl {
      std::string m_name, m_wName;
      Access m_wAccess;
    public:
      DummyWorker(Device &device, ezxml_t impl, ezxml_t inst, const char *idx);
      const char *status();
      OCPI::Container::Port *findPort(const char *) { return NULL; }
      const std::string &name() const { return m_name; }
      void prepareProperty(OCPI::Util::Property &, volatile uint8_t *&, const volatile uint8_t *&) const {}
      OCPI::Container::Port &createPort(const OCPI::Util::Port &, const OCPI::Util::PValue *) {
	assert("not called"==0);
	return *(OCPI::Container::Port*)this;
      }
      OCPI::Container::Port &createOutputPort(OCPI::Util::PortOrdinal, size_t, size_t,
					      const OCPI::Util::PValue*)
	throw (OCPI::Util::EmbeddedException)
      {
	assert("not called"==0);
	return *(OCPI::Container::Port*)this;
      }
      OCPI::Container::Port & createInputPort(OCPI::Util::PortOrdinal, size_t, size_t, const OCPI::Util::PValue*)
	throw (OCPI::Util::EmbeddedException)
      {
	assert("not called"==0);
	return *(OCPI::Container::Port*)this;
      }
      OCPI::Container::Application *application() { return NULL;}
      OCPI::Container::Worker *nextWorker() { return NULL; }
      void read(size_t, size_t, void *) {}
      void write(size_t, size_t, const void *) {}
    };
  }
}
#endif
