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

#ifndef HdlContainer_H
#define HdlContainer_H
#include "Container.h"
#include "HdlDriver.h"

namespace OCPI {
  namespace HDL {
    class Application;
    class Artifact;
    // We inherit Access to simply ensure that control accesses are as fast as possible.
    class Container
      : public OCPI::Container::ContainerBase<Driver, Container, Application, Artifact>,
	private Access, public OCPI::Time::Emit::TimeSource {
      HDL::Device &m_device;        // the underlying device that we own
      std::string m_part, m_esn, m_position, m_loadParams;
      OCPI::Time::Emit m_hwEvents;
      uint64_t m_lastTick;
      friend class WciControl;
      friend class Worker;
      friend class Driver;
      friend class Port;
      friend class Artifact;
    protected:
      Container(OCPI::HDL::Device &device, ezxml_t config = NULL, const OCPI::API::PValue *params = NULL);
    public:
      virtual ~Container();
      bool portsInProcess() { return false; }
      inline uint64_t getMyTicks() {
	return
	  m_device.isAlive() && !m_device.isFailed() && m_device.timeServer() ? 
	  (m_lastTick =
	   swap32(m_device.timeServer()->get64RegisterOffset(0)) +
	   hdlDevice().m_timeCorrection) :
	  m_lastTick;
      }
    protected:
      inline HDL::Device &hdlDevice() const { return m_device; }
    public:
      bool connectInside(OCPI::Container::BasicPort &in, OCPI::Container::BasicPort &out);
      //      void start();
      // void stop();
      OCPI::Container::Artifact &
	createArtifact(OCPI::Library::Artifact &lart, const OCPI::API::PValue *artifactParams);
      OCPI::API::ContainerApplication *
	createApplication(const char *name, const OCPI::Util::PValue *props)
	throw ( OCPI::Util::EmbeddedException );
      bool needThread();
      Container::DispatchRetCode dispatch(DataTransfer::EventManager*);
      void dump(bool before, bool hex);
    };
  }
}
#endif
