
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef OCPI_WORKER_H
#define OCPI_WORKER_H

#include "ezxml.h"
#include "OcpiParentChild.h"
#include "OcpiOsMutex.h"
#include "OcpiContainerDataTypes.h"
#include "OcpiMetadataWorker.h"
#include "OcpiContainerApi.h"

namespace OCPI {

  namespace Util {
    class EmbeddedException;
  }
  namespace Container {
    // This class is a small module of behavior used by workers, but available for other uses
    // Unfortunately, it is virtually inheritable (see HDL container's use of it).
    class Controllable {
    public:
      inline OCPI::Metadata::Worker::ControlState getState() { return m_state; }
      inline uint32_t getControlMask() { return m_controlMask; }
      inline void setControlMask(uint32_t mask) { m_controlMask = mask; }
      inline void setControlState(OCPI::Metadata::Worker::ControlState state) {
	m_state = state;
      }	
      inline OCPI::Metadata::Worker::ControlState getControlState() {
	return m_state;
      }	
    protected:
      Controllable();
      void setControlOperations(const char *controlOperations);
    private:
      OCPI::Metadata::Worker::ControlState m_state;
      uint32_t m_controlMask;
    };

    class Application;
    class Port;
    class Artifact;
    // This is the base class for all workers
    // It supports the API, and is a child of the Worker template class inherited by 
    // concrete workers
    class Worker
      : public OCPI::API::Worker,
	public OCPI::Metadata::Worker,  virtual public Controllable {

      friend class OCPI::API::Property;
      friend class Artifact;
      friend class Application;
      Artifact *m_artifact;
      ezxml_t m_xml;
      std::string m_implTag, m_instTag;
      // Our thread safe mutex for the worker itself
      OCPI::OS::Mutex m_workerMutex;
      void controlOp(OCPI::Metadata::Worker::ControlOperation);
    protected:
      inline OCPI::OS::Mutex &mutex() { return m_workerMutex; }
      virtual Port *findPort(const char *name) = 0;
      inline const std::string &instTag() const { return m_instTag; }
      inline const std::string &implTag() const { return m_implTag; }
      inline ezxml_t myXml() const { return m_xml; }
      Worker(Artifact *art, ezxml_t impl, ezxml_t inst, const OCPI::Util::PValue *props);
      void setupProperty(const char *name, OCPI::API::Property &prop);
      virtual void prepareProperty(OCPI::Util::Prop::Property &p, OCPI::API::Property &) = 0;
      virtual Port &createPort(OCPI::Metadata::Port &metaport, const OCPI::Util::PValue *props = NULL) = 0;

    public:
      virtual Application &application() = 0;
      void setProperty(const char *name, const char *value);

        /**
           @brief
           getLastControlError

           This method is used to get the last error that occured during a control
           operation.

           @param [ in ] workerId
           Container worker id.

           @retval std::string - last control error

           ****************************************************************** */
#if 0
      virtual std::string getLastControlError()
        throw ( OCPI::Util::EmbeddedException )=0;
#endif

      bool hasImplTag(const char *tag);
      bool hasInstTag(const char *tag);
      typedef unsigned Ordinal;
      // Generic setting method

      virtual ~Worker();
      OCPI::API::Port &getPort(const char *name, const OCPI::API::PValue *props);

      virtual Port & createOutputPort(OCPI::Metadata::PortOrdinal portId,
                                     OCPI::OS::uint32_t bufferCount,
                                     OCPI::OS::uint32_t bufferSize, 
                                      const OCPI::Util::PValue* props=NULL) 
        throw ( OCPI::Util::EmbeddedException ) = 0;

      virtual Port & createInputPort(OCPI::Metadata::PortOrdinal portId,
                                     OCPI::OS::uint32_t bufferCount,
                                     OCPI::OS::uint32_t bufferSize, 
                                     const OCPI::Util::PValue* props=NULL) 
        throw ( OCPI::Util::EmbeddedException ) = 0;
      virtual void read(uint32_t offset, uint32_t size, void *data) = 0;
      virtual void write(uint32_t offset, uint32_t size, const void *data) = 0;
#define CONTROL_OP(x, c, t, s1, s2, s3) \
      void x();
    OCPI_CONTROL_OPS
#undef CONTROL_OP
      virtual void controlOperation(OCPI::Metadata::Worker::ControlOperation) = 0;
    };
  }
}
#endif



