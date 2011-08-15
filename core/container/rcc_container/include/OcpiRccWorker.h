
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
    class Artifact;

    // Worker instance information structure
    class Worker
      : public OCPI::Container::WorkerBase<Application,Worker,Port>,
	public OCPI::Time::Emit
    {

    protected:
      friend class Application;
      friend class Controller;
      friend class Port;
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
      // Set a scalar property value
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                   \
      void set##pretty##Property(OCPI::API::Property &p, const run val) {        \
        if (p.m_info.m_writeError)                                                       \
          throw; /*"worker has errors before write */                           \
        store *pp = (store *)(getPropertyVaddr() + p.m_info.m_offset);	\
        if (bits > 32) {                                                        \
          assert(bits == 64);                                                   \
          uint32_t *p32 = (uint32_t *)pp;                                       \
          p32[1] = ((const uint32_t *)&val)[1];                                 \
          p32[0] = ((const uint32_t *)&val)[0];                                 \
        } else                                                                  \
          *pp = *(const store *)&val;                                           \
        if (p.m_info.m_writeError)					\
          throw; /*"worker has errors after write */                            \
      }                                                                         \
      void set##pretty##SequenceProperty(OCPI::API::Property &p,const run *vals, \
					 unsigned length) {		        \
        if (p.m_info.m_writeError)                                                       \
          throw; /*"worker has errors before write */                           \
        memcpy((void *)(getPropertyVaddr() + p.m_info.m_offset + p.m_info.m_maxAlign), vals,      \
	       length * sizeof(run));					        \
        *(uint32_t *)(getPropertyVaddr() + p.m_info.m_offset) = length;                  \
        if (p.m_info.m_writeError)                                                     \
          throw; /*"worker has errors after write */                            \
      }
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                  \
      virtual void set##pretty##Property(OCPI::API::Property &p, const run val) { \
        unsigned ocpi_length;                                                     \
        if (!val || (ocpi_length = strlen(val)) > p.m_type.stringLength)   \
          throw; /*"string property too long"*/;                                 \
        if (p.m_info.m_writeError)                                                       \
          throw; /*"worker has errors before write */                            \
        uint32_t *p32 = (uint32_t *)(getPropertyVaddr() + p.m_info.m_offset);             \
        /* if length to be written is more than 32 bits */                       \
        if (++ocpi_length > 32/CHAR_BIT)                                          \
          memcpy(p32 + 1, val + 32/CHAR_BIT, ocpi_length - 32/CHAR_BIT);          \
        uint32_t i;                                                              \
        memcpy(&i, val, 32/CHAR_BIT);                                            \
        p32[0] = i;                                                              \
        if (p.m_info.m_writeError)                                                       \
          throw; /*"worker has errors after write */                             \
      }                                                                          \
      void set##pretty##SequenceProperty(OCPI::API::Property &p,const run *vals,  \
					 unsigned length) {		         \
        if (p.m_info.m_writeError)                                                       \
          throw; /*"worker has errors before write */                            \
        char *cp = (char *)(getPropertyVaddr() + p.m_info.m_offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < length; i++) {                                  \
          unsigned len = strlen(vals[i]);                                        \
          if (len > p.m_type.stringLength)	                         \
            throw; /* "string in sequence too long" */                           \
          memcpy(cp, vals[i], len+1);                                            \
        }                                                                        \
        *(uint32_t *)(getPropertyVaddr() + p.m_info.m_offset) = length;                   \
        if (p.m_info.m_writeError)                                                       \
          throw; /*"worker has errors after write */                             \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
	virtual run get##pretty##Property(OCPI::API::Property &p) {	\
        if (p.m_info.m_readError )						\
          throw; /*"worker has errors before read "*/			\
        uint32_t *pp = (uint32_t *)(getPropertyVaddr() + p.m_info.m_offset);	\
        union {								\
	  run r;							\
	  uint32_t u32[bits/32];                                        \
        } u;								\
        if (bits > 32)							\
          u.u32[1] = pp[1];						\
        u.u32[0] = pp[0];						\
        if (p.m_info.m_readError )						\
          throw; /*"worker has errors after read */			\
        return u.r;							\
      }									\
      unsigned get##pretty##SequenceProperty(OCPI::API::Property &p,	\
					     run *vals,			\
					     unsigned length) {		\
        if (p.m_info.m_readError )						\
          throw; /*"worker has errors before read "*/			\
        uint32_t n = *(uint32_t *)(getPropertyVaddr() + p.m_info.m_offset);	\
        if (n > length)							\
          throw; /* sequence longer than provided buffer */		\
        memcpy(vals,							\
	       (void*)(getPropertyVaddr() + p.m_info.m_offset + p.m_info.m_maxAlign),	\
               n * sizeof(run));                                        \
        if (p.m_info.m_readError )						\
          throw; /*"worker has errors after read */			\
        return n;							\
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)		      \
	virtual void get##pretty##Property(OCPI::API::Property &p, char *cp,   \
					   unsigned length) {		      \
	  unsigned stringLength = p.m_type.stringLength;               \
	  if (length < stringLength+1)			      \
	    throw; /*"string buffer smaller than property"*/;		      \
	  if (p.m_info.m_readError)						      \
	    throw; /*"worker has errors before write */			      \
	  uint32_t i32, *p32 = (uint32_t *)(getPropertyVaddr() + p.m_info.m_offset);   \
	  memcpy(cp + 32/CHAR_BIT, p32 + 1, stringLength + 1 - 32/CHAR_BIT); \
	  i32 = *p32;							      \
	  memcpy(cp, &i32, 32/CHAR_BIT);				      \
	  if (p.m_info.m_readError)						      \
	    throw; /*"worker has errors after write */			      \
	}								      \
      unsigned get##pretty##SequenceProperty                                  \
	(OCPI::API::Property &p, run *vals, unsigned length, char *buf,        \
	 unsigned space) {						      \
        if (p.m_info.m_readError)						      \
          throw; /*"worker has errors before read */                          \
        uint32_t                                                              \
          n = *(uint32_t *)(getPropertyVaddr() + p.m_info.m_offset),                   \
          wlen = p.m_type.stringLength + 1;                            \
        if (n > length)                                                       \
          throw; /* sequence longer than provided buffer */                   \
        char *cp = (char *)(getPropertyVaddr() + p.m_info.m_offset + 32/CHAR_BIT);     \
        for (unsigned i = 0; i < n; i++) {                                    \
          if (space < wlen)                                                   \
            throw;                                                            \
          memcpy(buf, cp, wlen);                                              \
          cp += wlen;                                                         \
          vals[i] = buf;                                                      \
          unsigned slen = strlen(buf) + 1;                                    \
          buf += slen;                                                        \
          space -= slen;                                                      \
        }                                                                     \
        if (p.m_info.m_readError)                                                     \
          throw; /*"worker has errors after read */                           \
        return n;                                                             \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

        virtual void prepareProperty(OCPI::Metadata::Property&, OCPI::API::Property&);


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
      inline uint8_t * getPropertyVaddr() { return  (uint8_t*)m_context->properties; }

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
      void updatePort( Port &port );
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

