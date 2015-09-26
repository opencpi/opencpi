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
#ifndef CONTAINER_WORKER_H
#define CONTAINER_WORKER_H

#include "ezxml.h"
#include "OcpiContainerApi.h"
#include "OcpiOsMutex.h"
#include "OcpiOsTimer.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilWorker.h"
#include "OcpiPValue.h"

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

      OCPI::Util::Worker::ControlState getControlState() {
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
				    const uint8_t *data, size_t nBytes,
				    unsigned idx = 0) const = 0;
      virtual void setProperty8(const OCPI::API::PropertyInfo &info, uint8_t data,
				unsigned idx = 0) const = 0;
      virtual void setProperty16(const OCPI::API::PropertyInfo &info, uint16_t data,
				 unsigned idx = 0) const = 0;
      virtual void setProperty32(const OCPI::API::PropertyInfo &info, uint32_t data,
				 unsigned idx = 0) const = 0;
      virtual void setProperty64(const OCPI::API::PropertyInfo &info, uint64_t data,
				 unsigned idx = 0) const = 0;
      virtual void getPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
				    uint8_t *data, size_t nBytes, unsigned idx = 0) const = 0;
      virtual uint8_t getProperty8(const OCPI::API::PropertyInfo &info, unsigned idx = 0)
	const = 0;
      virtual uint16_t getProperty16(const OCPI::API::PropertyInfo &info, unsigned idx = 0)
	const = 0;
      virtual uint32_t getProperty32(const OCPI::API::PropertyInfo &info, unsigned idx = 0)
	const = 0;
      virtual uint64_t getProperty64(const OCPI::API::PropertyInfo &info, unsigned idx = 0)
	const = 0;
      virtual void controlOperation(OCPI::Util::Worker::ControlOperation) = 0;
    };
    typedef uint32_t PortMask;
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
      OCPI::OS::Mutex m_workerMutex; // Our thread safe mutex for the worker itself
      OCPI::OS::Mutex m_controlMutex; // HACK since sched_yield is busted with SCHED_OTHER
      bool m_controlOpPending;
      Worker *m_slave;
      bool m_hasMaster;
      size_t m_member, m_crewSize;
      PortMask m_connectedPorts, m_optionalPorts; // spcm?
      bool beforeStart();
    protected:
      void connectPort(OCPI::Util::PortOrdinal ordinal);
      PortMask &connectedPorts() { return m_connectedPorts; }
      PortMask &optionalPorts() { return m_optionalPorts; }
      virtual void portIsConnected(OCPI::Util::PortOrdinal /*ordinal*/) {};
      void checkControl();
      inline OCPI::OS::Mutex &mutex() { return m_workerMutex; }
      virtual Port *findPort(const char *name) = 0;
      inline const std::string &instTag() const { return m_instTag; }
      inline const std::string &implTag() const { return m_implTag; }
      inline const Artifact *artifact() const { return m_artifact; }
      inline ezxml_t myXml() const { return m_xml; }
      inline ezxml_t myInstXml() const { return m_instXml; }
      inline bool hasMaster() const { return m_hasMaster; }
      inline Worker *slave() const { return m_slave; }
      inline size_t member() const { return m_member; }
      inline size_t crewSize() const { return m_crewSize; }
      Worker(Artifact *art, ezxml_t impl, ezxml_t inst, Worker *slave, bool hasMaster,
	     size_t member, size_t crewSize, const OCPI::Util::PValue *params = NULL);
      OCPI::API::PropertyInfo &setupProperty(const char *name,
					     volatile void *&m_writeVaddr,
					     const volatile void *&m_readVaddr);
      OCPI::API::PropertyInfo &setupProperty(unsigned n,
					     volatile void *&m_writeVaddr,
					     const volatile void *&m_readVaddr);
      virtual Port &createPort(const OCPI::Util::Port &metaport,
			       const OCPI::Util::PValue *props) = 0;
      virtual Worker *nextWorker() = 0;
      virtual void setPropertyValue(const OCPI::Util::Property &p, const OCPI::Util::Value &v);


    public:
      // Return true when ignored due to "ignored due to existing state"
      bool controlOp(OCPI::Util::Worker::ControlOperation);
      void setPropertyValue(const OCPI::Util::Property &p, const std::string &v);
      virtual const std::string &name() const = 0;
      // This class is actually used in some contexts (e.g. ocpihdl),
      // Where it is not a child of an application, hence this method
      // is allowed to return NULL in this case, hence it returns a pointer.
      virtual Application *application() = 0;
      void setProperty(const char *name, const char *value);
      void setProperty(unsigned ordinal, OCPI::Util::Value &value);
      void setProperties(const char *props[][2]);
      void setProperties(const OCPI::API::PValue *props);
      virtual void getPropertyValue(const OCPI::Util::Property &p, std::string &value, bool hex,
				    bool add = false);
      bool getProperty(unsigned ordinal, std::string &name, std::string &value,
		       bool *unreadablep = NULL, bool hex = false);
      bool hasImplTag(const char *tag);
      bool hasInstTag(const char *tag);
      typedef unsigned Ordinal;
      // Generic setting method

      virtual ~Worker();
      OCPI::API::Port &getPort(const char *name, const OCPI::API::PValue *params = NULL);
      Port &getPort(const char *name, size_t nOthers, const OCPI::API::PValue *params = NULL);

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
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      run get##pretty##Parameter(unsigned ordinal, unsigned idx) const;
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) \
      void get##pretty##Parameter(unsigned ordinal, char *, size_t length, unsigned idx) const;
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
    };
  }
}
#endif
