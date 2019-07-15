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
/**
  @file

  @brief
    This file contains the interface and data structures for RCC Worker
      classes (OCPI::RCC::Worker, etc.)

************************************************************************** */

#ifndef RCC_WORKER_H_
#define RCC_WORKER_H_

#include <cstdarg>
#include "OcpiOsSemaphore.h"

#ifndef WORKER_INTERNAL
#define WORKER_INTERNAL
#endif
#include "RCC_Worker.h"

#include "ContainerManager.h"

namespace OCPI {
  namespace RCC {

    class Controller;
    class Application;
    class Port;
    class Artifact;

    // Worker instance information structure
    class Worker
      : public OCPI::Container::WorkerBase<Application,Worker,Port>,
	public OCPI::Time::Emit
    {
      friend class Application;
      friend class Controller;
      friend class Port;
      friend class RCCUserPort;
      friend class RCCUserSlave;
      friend class RCCUserWorker;
      void run(bool &anyRun);
      void advanceAll();
      void portError(std::string&error);
      bool doEOF();
      // EOF propagation for V2+ RCC workers
      // If workereof is set on the first input, then all handling is by the worker
      // Otherwise it is per-output-port
      inline bool checkEOF() const {
	return m_version >= 2 && m_firstInput && !m_firstInput->metaPort->m_workerEOF &&
	  m_firstInput->current.data && m_firstInput->current.eof_;
      }
    public:
      RCCResult setError(const char *fmt, va_list ap);
      inline RCCWorker &context() const { return *m_context; }

      Worker(Application & app, Artifact *art, const char *name, ezxml_t impl, ezxml_t inst,
	     const OCPI::Container::Workers &slaves, bool hasMaster, size_t member,
	     size_t crewSize, const OCPI::Util::PValue *wParams);
      OCPI::Container::Port& createPort(const OCPI::Util::Port&, const OCPI::Util::PValue *props);
      void controlOperation(OCPI::Util::Worker::ControlOperation);

      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors.
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
      void set##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member &, \
				 size_t off, const run val, unsigned idx) const; \
      void set##pretty##SequenceProperty(const OCPI::API::PropertyInfo &p, const run *vals, \
					 size_t length) const;
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)	\
      void set##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member &, \
				 size_t offset, const run val, unsigned idx) const;     \
      void set##pretty##SequenceProperty(const OCPI::API::PropertyInfo &p, const run *vals, \
					 size_t length) const;
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)	             \
	run get##pretty##Property(const OCPI::API::PropertyInfo &info,	\
				  const OCPI::Util::Member &m, size_t offset, \
				  unsigned idx) const;			\
        unsigned get##pretty##SequenceProperty(const OCPI::API::PropertyInfo &p, \
					       run *vals, size_t length) const;
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)	      \
	void get##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member &, \
				   size_t off, char *cp, size_t length, unsigned idx) const; \
      unsigned get##pretty##SequenceProperty                                  \
	(const OCPI::API::PropertyInfo &p, char **vals, size_t length, char *buf, \
	 size_t space) const;

      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
      void setPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
			    const uint8_t *data, size_t nBytes, unsigned idx) const;
      void setProperty8(const OCPI::API::PropertyInfo &info, size_t offset, uint8_t data, unsigned idx) const;
      void setProperty16(const OCPI::API::PropertyInfo &info, size_t offset, uint16_t data, unsigned idx) const;
      void setProperty32(const OCPI::API::PropertyInfo &info, size_t offset, uint32_t data, unsigned idx) const;
      void setProperty64(const OCPI::API::PropertyInfo &info, size_t offset, uint64_t data, unsigned idx) const;
      void getPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
			    uint8_t *data, size_t nBytes, unsigned idx, bool string) const;
      uint8_t getProperty8(const OCPI::API::PropertyInfo &info, size_t offset, unsigned idx) const;
      uint16_t getProperty16(const OCPI::API::PropertyInfo &info, size_t offset, unsigned idx) const;
      uint32_t getProperty32(const OCPI::API::PropertyInfo &info, size_t offset, unsigned idx) const;
      uint64_t getProperty64(const OCPI::API::PropertyInfo &info, size_t offset, unsigned idx) const;
      void propertyWritten(unsigned ordinal) const;
      void propertyRead(unsigned ordinal) const;
      void prepareProperty(OCPI::Util::Property&,
			   volatile uint8_t *&writeVaddr,
			   const volatile uint8_t *&readVaddr) const;
      // backward compatibility for ctests
      OCPI::Container::Port
	&createInputPort(OCPI::Util::PortOrdinal portId, size_t bufferCount, size_t bufferSize,
			 const OCPI::Util::PValue *params),
	&createOutputPort(OCPI::Util::PortOrdinal portId, size_t bufferCount, size_t bufferSize,
			  const OCPI::Util::PValue *params),
	&createTestPort(OCPI::Util::PortOrdinal portId, size_t bufferCount, size_t bufferSize,
			bool isProvider, const OU::PValue *params);
      void read(size_t offset, size_t nBytes, void* p_data);
      void write(size_t offset, size_t nBytes, const void* p_data);
      // end backward compatibility for ctests

      // Get our transport
      inline OCPI::DataTransport::Transport &getTransport() { return m_transport; }

      virtual ~Worker();


      std::list<OCPI::Util::Port*> m_testPmds;
    private:
      void initializeContext();
      void checkError() const;
      inline void setRunCondition(const RunCondition &rc) {
        if (m_runCondition) // there is no RunCondition::deactivate()
          m_runCondition->m_inUse = false;
        m_runCondition = &rc;
        m_runCondition->activate(m_runTimer, m_nPorts);
      }
      // Our dispatch table
      RCCEntryTable   *m_entry;    // our entry in the entry table of the artifact
      RCCUserWorker   *m_user;     // for C++, the user's worker object
      RCCDispatch     *m_dispatch; // For C-language, the dispatch
      RCCWorkerInfo    m_info;     // set from c dispatch or c++ entry point
      unsigned         m_portInit; // This counts up as C++ user ports are constructed
      // Our context
      RCCWorker       *m_context;
      RCCPort         *m_firstInput; // easy way to find first input port
      RCCPortMask     m_eofSent;     // record EOF propagation
      OCPI::OS::Mutex &m_mutex;
      RunCondition     m_defaultRunCondition; // run condition we create
      RunCondition     m_cRunCondition;       // run condition we use when C-language RC changes
      const RunCondition *m_runCondition;        // current active run condition used in dispatching

      // Mutable since this is a side effect of clearing the worker-set error when reported
      mutable char     *m_errorString;         // error string set via "setError"
    protected:
      OCPI::Container::Worker &getSlave(unsigned i);
      RCCPort &portInit() { return m_context->ports[m_portInit++]; }
      inline uint8_t * getPropertyVaddr() const { return  (uint8_t*)m_context->properties; }

      bool enabled;                // Worker enabled flag
      bool hasRun;                 // Has the worker ever run so far?

      uint32_t sourcePortCount;
      uint32_t targetPortCount;
      unsigned m_nPorts;

      // Last time that the worker was run
      OCPI::OS::Timer m_runTimer;
      OCPI::OS::Time  m_lastRun;

      // Debug/stats
      uint64_t worker_run_count;

      // Pointer into actual RCC worker binary for its dispatch struct
      OCPI::DataTransport::Transport &m_transport;

      // Task semaphore
      OCPI::OS::Semaphore m_taskSem;

      // Update a ports information (as a result of a connection)
      void portIsConnected(unsigned ordinal);
    };


    /**********************************
     *  This function is used as the factory for an artifact.
     *********************************/
    extern "C" {
      RCCDispatch* createWorkerTemplate(uint32_t type);
    }
  }
}

#endif
