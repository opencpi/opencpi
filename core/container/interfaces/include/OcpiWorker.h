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
#include "OcpiOsTimer.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilImplementation.h"
#include "OcpiContainerDataTypes.h"
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
      inline OCPI::Util::Worker::ControlState getState() { return m_state; }
      inline uint32_t getControlMask() { return m_controlMask; }
      inline void setControlMask(uint32_t mask) { m_controlMask = mask; }
      inline void setControlState(OCPI::Util::Worker::ControlState state) {
	m_state = state;
      }	
      // Default is that no polling is done
      virtual void checkControlState() {}

      inline OCPI::Util::Worker::ControlState getControlState() {
	checkControlState();
	return m_state;
      }	
    protected:
      Controllable();
      void setControlOperations(const char *controlOperations);
      virtual ~Controllable(){}
    private:
      OCPI::Util::Worker::ControlState m_state;
      uint32_t m_controlMask;
    };

    class Application;
    class Port;
    class Artifact;
    // This is the base class for all workers
    // It supports the API, and is a child of the Worker template class inherited by 
    // concrete workers
    // These interfaces must be supplied for control purposes
    class WorkerControl {
    protected:
      virtual ~WorkerControl();
    public:
      //      virtual const std::string &name() const = 0;
      virtual void prepareProperty(OCPI::Util::Property &p,
				   volatile void *&m_writeVaddr,
				   const volatile void *&m_readVaddr) = 0;
      virtual void setPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
				    const uint8_t *data, size_t nBytes) const = 0;
      virtual void setProperty8(const OCPI::API::PropertyInfo &info, uint8_t data) const = 0;
      virtual void setProperty16(const OCPI::API::PropertyInfo &info, uint16_t data) const = 0;
      virtual void setProperty32(const OCPI::API::PropertyInfo &info, uint32_t data) const = 0;
      virtual void setProperty64(const OCPI::API::PropertyInfo &info, uint64_t data) const = 0;
      virtual void getPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
				    uint8_t *data, size_t nBytes) const = 0;
      virtual uint8_t getProperty8(const OCPI::API::PropertyInfo &info) const = 0;
      virtual uint16_t getProperty16(const OCPI::API::PropertyInfo &info) const = 0;
      virtual uint32_t getProperty32(const OCPI::API::PropertyInfo &info) const = 0;
      virtual uint64_t getProperty64(const OCPI::API::PropertyInfo &info) const = 0;
      virtual void controlOperation(OCPI::Util::Worker::ControlOperation) = 0;
    };
    class Worker
      : public OCPI::Util::Worker, public OCPI::API::Worker, virtual public Controllable,
	virtual public WorkerControl
    {

      friend class OCPI::API::Property;
      friend class Port;
      friend class Artifact;
      friend class Application;
      Artifact *m_artifact;
      ezxml_t m_xml, m_instXml;
      std::string m_implTag, m_instTag;
      // Our thread safe mutex for the worker itself
      OCPI::OS::Mutex m_workerMutex;
      bool beforeStart();
    protected:
      // Return true when ignored due to "ignored due to existing state"
      bool controlOp(OCPI::Util::Worker::ControlOperation);
      inline OCPI::OS::Mutex &mutex() { return m_workerMutex; }
      virtual Port *findPort(const char *name) = 0;
      inline const std::string &instTag() const { return m_instTag; }
      inline const std::string &implTag() const { return m_implTag; }
      virtual const std::string &name() const = 0;
      inline ezxml_t myXml() const { return m_xml; }
      inline ezxml_t myInstXml() const { return m_instXml; }
      Worker(Artifact *art, ezxml_t impl, ezxml_t inst, const OCPI::Util::PValue *props = NULL);
      OCPI::API::PropertyInfo &setupProperty(const char *name,
					     volatile void *&m_writeVaddr,
					     const volatile void *&m_readVaddr);
      OCPI::API::PropertyInfo &setupProperty(unsigned n,
					     volatile void *&m_writeVaddr,
					     const volatile void *&m_readVaddr);
#if 0
      virtual void prepareProperty(OCPI::Util::Property &p,
				   volatile void *&m_writeVaddr,
				   const volatile void *&m_readVaddr) = 0;
#endif
      virtual Port &createPort(const OCPI::Util::Port &metaport,
			       const OCPI::Util::PValue *props) = 0;
      virtual Worker *nextWorker() = 0;
      void setPropertyValue(const OCPI::API::Property &p, const OCPI::Util::Value &v);


    public:
      // This class is actually used in some contexts (e.g. ocpihdl),
      // Where it is not a child of an application, hence this method
      // is allowed to return NULL in this case, hence it returns a pointer.
      virtual Application *application() = 0;
      void setProperty(const char *name, const char *value);
      void setProperty(unsigned ordinal, OCPI::Util::Value &value);
      void setProperties(const char *props[][2]);
      void setProperties(const OCPI::API::PValue *props);
#if 0
      virtual void setPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
				    const uint8_t *data, size_t nBytes) const = 0;
      virtual void setProperty8(const OCPI::API::PropertyInfo &info, uint8_t data) const = 0;
      virtual void setProperty16(const OCPI::API::PropertyInfo &info, uint16_t data) const = 0;
      virtual void setProperty32(const OCPI::API::PropertyInfo &info, uint32_t data) const = 0;
      virtual void setProperty64(const OCPI::API::PropertyInfo &info, uint64_t data) const = 0;
      virtual void getPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
				    uint8_t *data, size_t nBytes) const = 0;
      virtual uint8_t getProperty8(const OCPI::API::PropertyInfo &info) const = 0;
      virtual uint16_t getProperty16(const OCPI::API::PropertyInfo &info) const = 0;
      virtual uint32_t getProperty32(const OCPI::API::PropertyInfo &info) const = 0;
      virtual uint64_t getProperty64(const OCPI::API::PropertyInfo &info) const = 0;
#endif
      bool getProperty(unsigned ordinal, std::string &name, std::string &value,
		       bool *unreadablep = NULL, bool hex = false);
      bool hasImplTag(const char *tag);
      bool hasInstTag(const char *tag);
      typedef unsigned Ordinal;
      // Generic setting method

      virtual ~Worker();
      OCPI::API::Port &getPort(const char *name, const OCPI::API::PValue *props = NULL);

      virtual Port & createOutputPort(OCPI::Util::PortOrdinal portId,
				      size_t bufferCount,
				      size_t bufferSize, 
                                      const OCPI::Util::PValue* props) 
        throw ( OCPI::Util::EmbeddedException ) = 0;

      virtual Port & createInputPort(OCPI::Util::PortOrdinal portId,
                                     size_t bufferCount,
                                     size_t bufferSize, 
                                     const OCPI::Util::PValue* props) 
        throw ( OCPI::Util::EmbeddedException ) = 0;
      virtual void read(size_t offset, size_t size, void *data) = 0;
      virtual void write(size_t offset, size_t size, const void *data) = 0;
#define CONTROL_OP(x, c, t, s1, s2, s3, s4)	\
      void x();
    OCPI_CONTROL_OPS
#undef CONTROL_OP
      virtual bool wait(OCPI::OS::Timer *t = NULL);
      bool isDone();
    };
  }
}
#endif



