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


/*
 * Abstact:
 *   This file contains the interface and data structures for the JTRS DSP Worker.
 *
 * Revision History: 
 *
 *    06/23/09  John Miller
 *    Added code to handle RCC_ERROR and RCC_FATAL return codes.
 * 
 *    06/01/05  John Miller
 *    Initial revision
 *
 */

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
    public:
      RCCResult setError(const char *fmt, va_list ap);
      inline RCCWorker &context() const { return *m_context; }

      Worker(Application & app, Artifact *art, const char *name, ezxml_t impl, ezxml_t inst,
	     OCPI::Container::Worker *slave, size_t member, size_t crewSize,
	     const OCPI::Util::PValue *wParams);
      OCPI::Container::Port& createPort(const OCPI::Util::Port&, const OCPI::Util::PValue *props);
      void controlOperation(OCPI::Util::Worker::ControlOperation);

      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors. 
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                         \
      void set##pretty##Property(unsigned ordinal, const run val) const;   \
      void set##pretty##SequenceProperty(const OCPI::API::Property &p,const run *vals, \
					 size_t length) const;
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                       \
      void set##pretty##Property(unsigned ordinal, const run val) const;   \
      void set##pretty##SequenceProperty(const OCPI::API::Property &p,const run *vals, \
					 size_t length) const;
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)	             \
	run get##pretty##Property(unsigned ordinal) const;       \
        unsigned get##pretty##SequenceProperty(const OCPI::API::Property &p, \
					     run *vals,			     \
					       size_t length) const;
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)		\
	void get##pretty##Property(unsigned ordinal, char *cp,      \
				   size_t length) const;                      \
      unsigned get##pretty##SequenceProperty                                    \
	(const OCPI::API::Property &p, char **vals, size_t length, char *buf, \
	 size_t space) const;

      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
      void setPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
			    const uint8_t *data, size_t nBytes) const;
      void setProperty8(const OCPI::API::PropertyInfo &info, uint8_t data) const;
      void setProperty16(const OCPI::API::PropertyInfo &info, uint16_t data) const;
      void setProperty32(const OCPI::API::PropertyInfo &info, uint32_t data) const;
      void setProperty64(const OCPI::API::PropertyInfo &info, uint64_t data) const;
      void getPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
				    uint8_t *data, size_t nBytes) const;
      uint8_t getProperty8(const OCPI::API::PropertyInfo &info) const;
      uint16_t getProperty16(const OCPI::API::PropertyInfo &info) const;
      uint32_t getProperty32(const OCPI::API::PropertyInfo &info) const;
      uint64_t getProperty64(const OCPI::API::PropertyInfo &info) const;


        virtual void prepareProperty(OCPI::Util::Property&,
				     volatile void *&writeVaddr,
				     const volatile void *&readVaddr);

      OCPI::Container::Port &  createInputPort(
					       OCPI::Util::PortOrdinal   portId,      
                                              size_t   bufferCount,
                                              size_t   bufferSize, 
                                              const OCPI::Util::PValue*  props=NULL
                                              )
        throw ( OCPI::Util::EmbeddedException );


      OCPI::Container::Port &  createOutputPort( 
						OCPI::Util::PortOrdinal   portId,     
                                               size_t             bufferCount,
                                               size_t             bufferSize, 
                                               const OCPI::Util::PValue*               props=NULL
                                               )
        throw ( OCPI::Util::EmbeddedException );

        void read (size_t offset,
                         size_t nBytes,
                         void* p_data );
        void write ( size_t offset,
                          size_t nBytes,
                          const void* p_data );
     
      // Get our transport
      inline OCPI::DataTransport::Transport & getTransport() { return m_transport; }

      virtual ~Worker();


      std::list<OCPI::Util::Port*> m_testPmds;
    private:
      

      void initializeContext();
      inline void setRunCondition(const RunCondition &rc) {
	m_runCondition = &rc;
	if (rc.m_timeout)
	  m_runTimer.reset(rc.m_usecs / 1000000, (rc.m_usecs % 1000000) * 1000);
      }

      // Our dispatch table
      RCCEntryTable   *m_entry;    // our entry in the entry table of the artifact
      RCCUserWorker   *m_user;     // for C++, the user's worker object
      RCCDispatch     *m_dispatch; // For C-language, the dispatch
      RCCWorkerInfo    m_info;     // set from c dispatch or c++ entry point
      unsigned         m_portInit; // This counts up as C++ user ports are constructed
      // Our context
      RCCWorker       *m_context;
      OCPI::OS::Mutex &m_mutex;
      RunCondition     m_defaultRunCondition; // run condition we create
      RunCondition     m_cRunCondition;       // run condition we use when C-language RC changes
      const RunCondition *m_runCondition;        // current active run condition used in dispatching
      
      char            *m_errorString;         // error string set via "setError"
      OCPI::Container::Worker      *m_slave;
    protected:
      OCPI::Container::Worker &getSlave();
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

      // Override the port data based on hardcoded requirements from the worker
      void overRidePortInfo( OCPI::Util::Port & portData );

      // Update a ports information (as a result of a connection)
      void portIsConnected( unsigned ordinal );
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

