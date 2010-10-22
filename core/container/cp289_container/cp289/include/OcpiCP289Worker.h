
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


#ifndef OCPI_CP289_WORKER_H_
#define OCPI_CP289_WORKER_H_

#include <OcpiCP289Port.h>

namespace OCPI {

  namespace Container {
    class PortData;
  }

  namespace CP289 {

    namespace CM = OCPI::Metadata;
    class Controller;
    class RCCWorkerInterface;
    class Application;
    class Port;

    // Worker instance information structure
    class Worker : public OCPI::Container::Worker
    {
      static ::WCI_control wciControls[CM::Worker::OpsLimit];

    public:
      friend class Application;
      friend class Controller;
      friend class Port;
      friend class RCCWorkerInterface;

      Worker( OCPI::Container::Application & app, RCCDispatch *entryPoint, OCPI::Util::PValue *wparams,
              ::RCCContainer* c, ezxml_t impl, ezxml_t inst );
      OCPI::Container::Port& createPort(OCPI::Metadata::Port&);

      // Control Operations 
#if 1
#define CONTROL_OP(x, c, t, s1, s2, s3) virtual void x();
      OCPI_CONTROL_OPS
#undef CONTROL_OP
#else
      inline void initialize(){control(WCI_CONTROL_INITIALIZE, WCI_DEFAULT );}
      inline void start(){ control(WCI_CONTROL_START, WCI_DEFAULT );}
      inline void stop(){enabled=false;stop(true);}
      inline void release(){ control(WCI_CONTROL_RELEASE, WCI_DEFAULT );}
      inline void beforeQuery(){control(WCI_CONTROL_BEFORE_QUERY, WCI_DEFAULT );}
      inline void afterConfigure(){control(WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );}
      inline void test(){control(WCI_CONTROL_TEST, WCI_DEFAULT );}
#endif

      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors. 
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                   \
      void set##pretty##Property(Metadata::Property &p, const run val) {        \
        if (p.writeError)                                                       \
          throw; /*"worker has errors before write */                           \
        store *pp = (store *)(getPropertyVaddr() + p.offset);                   \
        if (bits > 32) {                                                        \
          assert(bits == 64);                                                   \
          uint32_t *p32 = (uint32_t *)pp;                                       \
          p32[1] = ((const uint32_t *)&val)[1];                                 \
          p32[0] = ((const uint32_t *)&val)[0];                                 \
        } else                                                                  \
          *pp = *(const store *)&val;                                           \
        if (p.writeError)                                                       \
          throw; /*"worker has errors after write */                            \
      }                                                                         \
      void set##pretty##SequenceProperty(Metadata::Property &p,const run *vals, \
					 unsigned length) {		        \
        if (p.writeError)                                                       \
          throw; /*"worker has errors before write */                           \
        memcpy((void *)(getPropertyVaddr() + p.offset + p.maxAlign), vals,      \
	       length * sizeof(run));					        \
        *(uint32_t *)(getPropertyVaddr() + p.offset) = length;                  \
        if (p.writeError)                                                     \
          throw; /*"worker has errors after write */                            \
      }
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                  \
      virtual void set##pretty##Property(Metadata::Property &p, const run val) { \
        unsigned ocpi_length;                                                     \
        if (!val || (ocpi_length = strlen(val)) > p.members->type.stringLength)   \
          throw; /*"string property too long"*/;                                 \
        if (p.writeError)                                                       \
          throw; /*"worker has errors before write */                            \
        uint32_t *p32 = (uint32_t *)(getPropertyVaddr() + p.offset);             \
        /* if length to be written is more than 32 bits */                       \
        if (++ocpi_length > 32/CHAR_BIT)                                          \
          memcpy(p32 + 1, val + 32/CHAR_BIT, ocpi_length - 32/CHAR_BIT);          \
        uint32_t i;                                                              \
        memcpy(&i, val, 32/CHAR_BIT);                                            \
        p32[0] = i;                                                              \
        if (p.writeError)                                                       \
          throw; /*"worker has errors after write */                             \
      }                                                                          \
      void set##pretty##SequenceProperty(Metadata::Property &p,const run *vals,  \
					 unsigned length) {		         \
        if (p.writeError)                                                       \
          throw; /*"worker has errors before write */                            \
        char *cp = (char *)(getPropertyVaddr() + p.offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < length; i++) {                                  \
          unsigned len = strlen(vals[i]);                                        \
          if (len > p.members->type.stringLength)	                         \
            throw; /* "string in sequence too long" */                           \
          memcpy(cp, vals[i], len+1);                                            \
        }                                                                        \
        *(uint32_t *)(getPropertyVaddr() + p.offset) = length;                   \
        if (p.writeError)                                                       \
          throw; /*"worker has errors after write */                             \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
	virtual run get##pretty##Property(Metadata::Property &p) {	\
        if (p.readError )						\
          throw; /*"worker has errors before read "*/			\
        uint32_t *pp = (uint32_t *)(getPropertyVaddr() + p.offset);	\
        union {								\
	  run r;							\
	  uint32_t u32[bits/32];                                        \
        } u;								\
        if (bits > 32)							\
          u.u32[1] = pp[1];						\
        u.u32[0] = pp[0];						\
        if (p.readError )						\
          throw; /*"worker has errors after read */			\
        return u.r;							\
      }									\
      unsigned get##pretty##SequenceProperty(Metadata::Property &p,	\
					     run *vals,			\
					     unsigned length) {		\
        if (p.readError )						\
          throw; /*"worker has errors before read "*/			\
        uint32_t n = *(uint32_t *)(getPropertyVaddr() + p.offset);	\
        if (n > length)							\
          throw; /* sequence longer than provided buffer */		\
        memcpy(vals,							\
	       (void*)(getPropertyVaddr() + p.offset + p.maxAlign),	\
               n * sizeof(run));                                        \
        if (p.readError )						\
          throw; /*"worker has errors after read */			\
        return n;							\
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)		      \
	virtual void get##pretty##Property(Metadata::Property &p, char *cp,   \
					   unsigned length) {		      \
	  unsigned stringLength = p.members->type.stringLength;               \
	  if (length < stringLength+1)			      \
	    throw; /*"string buffer smaller than property"*/;		      \
	  if (p.readError)						      \
	    throw; /*"worker has errors before write */			      \
	  uint32_t i32, *p32 = (uint32_t *)(getPropertyVaddr() + p.offset);   \
	  memcpy(cp + 32/CHAR_BIT, p32 + 1, stringLength + 1 - 32/CHAR_BIT); \
	  i32 = *p32;							      \
	  memcpy(cp, &i32, 32/CHAR_BIT);				      \
	  if (p.readError)						      \
	    throw; /*"worker has errors after write */			      \
	}								      \
      unsigned get##pretty##SequenceProperty                                  \
	(Metadata::Property &p, run *vals, unsigned length, char *buf,        \
	 unsigned space) {						      \
        if (p.readError)						      \
          throw; /*"worker has errors before read */                          \
        uint32_t                                                              \
          n = *(uint32_t *)(getPropertyVaddr() + p.offset),                   \
          wlen = p.members->type.stringLength + 1;                            \
        if (n > length)                                                       \
          throw; /* sequence longer than provided buffer */                   \
        char *cp = (char *)(getPropertyVaddr() + p.offset + 32/CHAR_BIT);     \
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
        if (p.readError)                                                     \
          throw; /*"worker has errors after read */                           \
        return n;                                                             \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

        virtual void prepareProperty(OCPI::Metadata::Property&, OCPI::Container::Property&);


      OCPI::Container::Port &  createInputPort(
                                              OCPI::Container::PortId              portId,      
                                              OCPI::OS::uint32_t   bufferCount,
                                              OCPI::OS::uint32_t   bufferSize, 
                                              OCPI::Util::PValue*  props=NULL
                                              )
        throw ( OCPI::Util::EmbeddedException );


      OCPI::Container::Port &  createOutputPort( 
                                               OCPI::Container::PortId        portId,     
                                               OCPI::OS::uint32_t             bufferCount,
                                               OCPI::OS::uint32_t             bufferSize, 
                                               OCPI::Util::PValue*               props=NULL
                                               )
        throw ( OCPI::Util::EmbeddedException );




      // The following methods are WCI control ops and are all deprecated.
        std::string getLastControlError()
          throw ( OCPI::Util::EmbeddedException );
        WCI_error control (  WCI_control operation, WCI_options options );
        WCI_error status ( WCI_status* p_status );
        WCI_error read ( WCI_u32 offset,
                         WCI_u32 nBytes,
                         WCI_data_type data_type,
                         WCI_options options,
                         void* p_data );
        WCI_error write ( WCI_u32 offset,
                          WCI_u32 nBytes,
                          WCI_data_type data_type,
                          WCI_options options,
                          const void* p_data );
        WCI_error close ( WCI_options options ) ;


      // Set/get properties : deprecated
      void setProperties( OCPI::OS::uint32_t offset, OCPI::OS::uint32_t nBytes, const void *p_data  );
      void getProperties( OCPI::OS::uint32_t offset, OCPI::OS::uint32_t nBytes, void *p_data  );

     
      // Get our transport
      OCPI::DataTransport::Transport & getTransport();


    private:

      // Last error description
      std::string m_lastError;
      
      // Last WCI operation status
      WCI_status    m_wciStatus;


    protected:
      uint8_t * getPropertyVaddr();

      // this wont work      OCPI::Util::PValue properties;  
      RCCWorkerInterface * m_rcc_worker;               

      OCPI::Container::WorkerId                       workerId;        // Worker id
      bool                               enabled;                // Worker enabled flag

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
      bool run_condition_met;

      // Debug/stats
      OCPI::OS::uint32_t worker_run_count;
      virtual ~Worker();
      void stop(bool);

      // Override the port data based on hardcoded requirements from the worker
      void overRidePortInfo( OCPI::Metadata::Port & portData );

      // Update a ports information (as a result of a connection)
      void updatePort( OCPI::CP289::Port &port );



    };



    // C++ worker interface
    class RCCWorkerInterface 
    {

    public:
      //!< Worker States
      enum WorkerState {
        WorkerExists,
        WorkerInitialized,
        WorkerOperational,
        WorkerSuspended,
        WorkerUnusable
      };

      // Constructor
      RCCWorkerInterface( ::RCCDispatch* wd, Worker* wi );

      // Destructor
      ~RCCWorkerInterface();

      inline WorkerState getState() {return m_state;}

      // init
      inline RCCResult initialize()
        {
          RCCResult rc = RCC_OK;
          if ( m_dispatch->initialize ) {
            rc = m_dispatch->initialize(m_context);
            if ( rc == RCC_OK ) {
              m_state = WorkerInitialized;
            }
            else if ( rc == RCC_ERROR ) {
              // No state change
            }
            else {
              m_state = WorkerUnusable;
            }
              
          }
          else {
            m_state = WorkerInitialized;
          }
          return rc;
        };


      // start
      inline RCCResult start()
        {
          RCCResult rc = RCC_OK;          
          if ( m_dispatch->start ) {
            rc = m_dispatch->start(m_context);
            if ( rc == RCC_OK ) {
              m_state = WorkerOperational;
            }
            else if ( rc == RCC_ERROR ) {
              // No state change
            }
            else {
              m_state = WorkerUnusable;
            }
          }
          else {
            m_state = WorkerOperational;
          }
          return rc;
        };

      // stop
      inline RCCResult stop() 
        {
          RCCResult rc = RCC_OK;
          if ( m_dispatch->stop ) {
            rc = m_dispatch->stop(m_context);
            if ( rc == RCC_OK ) {
              m_state = WorkerSuspended;
            }
            else if ( rc == RCC_ERROR ) {
              // No state change
            }
            else {
              m_state = WorkerUnusable;
            }
          }
          else {
            m_state = WorkerSuspended;
          }
          return rc;
        };

      // release
      inline bool releaseImplemented() {return  m_dispatch->release ? true : false; }
      inline RCCResult release() 
        {
          RCCResult rc = RCC_OK;
          if ( m_dispatch->release ) {
            rc = m_dispatch->release(m_context);
            if ( rc == RCC_OK ) {
              m_state = WorkerExists;
            }
            else if ( rc == RCC_ERROR ) {
              // No state change
            }
            else {
              m_state = WorkerUnusable;
            }
          }
          else {
            m_state = WorkerExists;

          }
          return rc;
        };

      inline bool testImplemented() {return  m_dispatch->test ? true : false; }
      // test
      inline RCCResult test()
        {
          if ( m_dispatch->test ) {
            return m_dispatch->test(m_context);
          }
          else {
            return RCC_ERROR;
          }
        };

      // Configuration
      inline RCCResult afterConfigure()
        {
          if ( m_dispatch->afterConfigure ) {
            return m_dispatch->afterConfigure(m_context);
          }
          else {
            return RCC_OK;
          }
        };
      inline RCCResult beforeQuery()
        {
          if ( m_dispatch->beforeQuery ) {
            return m_dispatch->beforeQuery(m_context);
          }
          else {
            return RCC_OK;
          }
        };

      // run
      inline RCCResult run(RCCBoolean timedout, RCCBoolean *newRunCondition)
        {
          if ( m_dispatch->run ) {
            return m_dispatch->run(m_context,timedout,newRunCondition);
          }
          else {
            return RCC_OK;
          }
        }

      // Worker state
      WorkerState m_state;

      // Our context
      RCCWorker* m_context;

      // Number of ports
      int m_nports;

      // Properties
      RCCOrdinal *m_properties;

      // Our dispatch table
      RCCDispatch* m_dispatch;

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

