
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


#ifndef OCPI_RCC_WORKER_H_
#define OCPI_RCC_WORKER_H_

#include <climits>
#include <OcpiOsTimer.h>
#include <OcpiTimeEmit.h>
#include <OcpiContainerManager.h>

namespace OCPI {
  namespace RCC {

    namespace CM = OCPI::Metadata;

    class Controller;
    class Application;
    class Port;
    class RDMAPort;
    class Artifact;

    // Worker instance information structure
    class Worker
      : public OCPI::Container::WorkerBase<Application,Worker,Port>,
	public OCPI::Time::Emit
    {

    protected:
      friend class Application;
      friend class Controller;
      friend class RDMAPort;
      friend class MessagePort;
      ::RCCPortMask getReadyPorts();
      void run(bool &anyRun);
      void checkDeadLock();
      void advanceAll();
    public:

      Worker( Application & app, Artifact *art, const char *name,
	      ezxml_t impl, ezxml_t inst, const OCPI::Util::PValue *wParams);
      OCPI::Container::Port& createPort(const OCPI::Metadata::Port&, const OCPI::Util::PValue *props);
      void controlOperation(OCPI::Metadata::Worker::ControlOperation);

      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors. 
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                         \
      void set##pretty##Property(const OCPI::API::Property &p, const run val) const;   \
      void set##pretty##SequenceProperty(const OCPI::API::Property &p,const run *vals, \
					 unsigned length) const;
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                       \
      void set##pretty##Property(const OCPI::API::Property &p, const run val) const;   \
      void set##pretty##SequenceProperty(const OCPI::API::Property &p,const run *vals, \
					 unsigned length) const;
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)	             \
	run get##pretty##Property(const OCPI::API::Property &p) const;       \
        unsigned get##pretty##SequenceProperty(const OCPI::API::Property &p, \
					     run *vals,			     \
					       unsigned length) const;
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)		\
	void get##pretty##Property(const OCPI::API::Property &p, char *cp,      \
				   unsigned length) const;                      \
      unsigned get##pretty##SequenceProperty                                    \
	(const OCPI::API::Property &p, char **vals, unsigned length, char *buf, \
	 unsigned space) const;

      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

        virtual void prepareProperty(OCPI::Metadata::Property&,
				     volatile void *&writeVaddr,
				     const volatile void *&readVaddr);

      OCPI::Container::Port &  createInputPort(
					       OCPI::Metadata::PortOrdinal   portId,      
                                              OCPI::OS::uint32_t   bufferCount,
                                              OCPI::OS::uint32_t   bufferSize, 
                                              const OCPI::Util::PValue*  props=NULL
                                              )
        throw ( OCPI::Util::EmbeddedException );


      OCPI::Container::Port &  createOutputPort( 
						OCPI::Metadata::PortOrdinal   portId,     
                                               OCPI::OS::uint32_t             bufferCount,
                                               OCPI::OS::uint32_t             bufferSize, 
                                               const OCPI::Util::PValue*               props=NULL
                                               )
        throw ( OCPI::Util::EmbeddedException );

        void read ( uint32_t offset,
                         uint32_t nBytes,
                         void* p_data );
        void write ( uint32_t offset,
                          uint32_t nBytes,
                          const void* p_data );
     
      // Get our transport
      inline OCPI::DataTransport::Transport & getTransport() { return m_transport; }

      virtual ~Worker();


    private:
      void initializeContext();

      // Last error description
      //std::string m_lastError;
      
      // Last WCI operation status
      //WCI_status    m_wciStatus;

      // Our dispatch table
      RCCDispatch* m_dispatch;
      // Our context
      RCCWorker* m_context;
      OCPI::OS::Mutex &m_mutex;

    protected:
      inline uint8_t * getPropertyVaddr() const { return  (uint8_t*)m_context->properties; }

      bool enabled;                // Worker enabled flag

      // List of worker ports
      OCPI::OS::uint32_t sourcePortCount;
      OCPI::OS::uint32_t targetPortCount;

      // Sparse list, indexable by port ordinal
      OCPI::Util::VList  sourcePorts;   
      OCPI::Util::VList  targetPorts;        

      // Worker run condition super-set
      OCPI::OS::uint32_t runConditionSS;

      // Last time that the worker was run
      OCPI::OS::Timer               runTimer;
      OCPI::OS::Timer::ElapsedTime  runTimeout;
                        
      // runtime control
      //      bool run_condition_met;

      // Debug/stats
      OCPI::OS::uint32_t worker_run_count;

      // Pointer into actual RCC worker binary for its dispatch struct
      OCPI::DataTransport::Transport &m_transport;

      // Override the port data based on hardcoded requirements from the worker
      void overRidePortInfo( OCPI::Metadata::Port & portData );

      // Update a ports information (as a result of a connection)
      void updatePort( RDMAPort &port );
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

