// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

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


#ifndef CPI_CP289_WORKER_H_
#define CPI_CP289_WORKER_H_

#include <CpiCP289Port.h>

namespace CPI {

  namespace Container {
    class PortData;
  }

  namespace CP289 {

    namespace CM = CPI::Metadata;
    class Controller;
    class RCCWorkerInterface;
    class Application;
    class Port;

    // Worker instance information structure
    class Worker : public CPI::Container::Worker
    {
      static ::WCI_control wciControls[CM::Worker::OpsLimit];

    public:
      friend class Application;
      friend class Controller;
      friend class Port;
      friend class RCCWorkerInterface;

      Worker( CPI::Container::Application & app, RCCDispatch *entryPoint, CPI::Util::PValue *wparams,
              ::RCCContainer* c, ezxml_t impl, ezxml_t inst );
      CPI::Container::Port& createPort(CPI::Metadata::Port&);

      // Control Operations 
#if 1
#define CONTROL_OP(x, c, t, s1, s2, s3) virtual void x();
      CPI_CONTROL_OPS
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
#undef CPI_DATA_TYPE_S
      // Set a scalar property value
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      void set##pretty##Property(unsigned ord, const run val) {                \
        CPI::Metadata::Property &p = property(ord);                                \
        if (!p.is_writable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (p.write_error ) \
          throw; /*"worker has errors before write */                        \
        store *pp = (store *)(getPropertyVaddr() + p.offset);                        \
        if (bits > 32) {                                                \
          assert(bits == 64);                                                \
          uint32_t *p32 = (uint32_t *)pp;                                \
          p32[1] = ((uint32_t *)&val)[1];                                \
          p32[0] = ((uint32_t *)&val)[0];                                \
        } else                                                                \
          *pp = *(store *)&val;                                                \
        if (p.write_error ) \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      void set##pretty##SequenceProperty(unsigned ord,const run *vals, unsigned length) { \
        CPI::Metadata::Property &p = property(ord);                                \
        assert(p.types->type == CPI::Metadata::Property::CPI_##pretty);                \
        if (!p.is_writable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (length > p.sequence_size)                                        \
          throw;                                                        \
        if (p.write_error )  \
          throw; /*"worker has errors before write */                        \
        memcpy((void *)(getPropertyVaddr() + p.offset + p.maxAlign), vals, length * sizeof(run)); \
        *(uint32_t *)(getPropertyVaddr() + p.offset) = length;                \
        if (p.write_error ) \
          throw; /*"worker has errors after write */                        \
      }
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void set##pretty##Property(unsigned ord, const run val) {        \
        CPI::Metadata::Property &p = property(ord);                                \
        assert(p.types->type == CPI::Metadata::Property::CPI_##pretty);                \
        if (!p.is_writable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        unsigned cpi_length;                                                \
        if (!val || (cpi_length = strlen(val)) > p.types->size)                \
          throw; /*"string property too long"*/;                        \
        if (p.write_error ) \
          throw; /*"worker has errors before write */                        \
        uint32_t *p32 = (uint32_t *)(getPropertyVaddr() + p.offset);                \
        /* if length to be written is more than 32 bits */                \
        if (++cpi_length > 32/CHAR_BIT)                                        \
          memcpy(p32 + 1, val + 32/CHAR_BIT, cpi_length - 32/CHAR_BIT); \
        uint32_t i;                                                        \
        memcpy(&i, val, 32/CHAR_BIT);                                        \
        p32[0] = i;                                                        \
        if (p.write_error ) \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      void set##pretty##SequenceProperty(unsigned ord,const run *vals, unsigned length) { \
        CPI::Metadata::Property &p = property(ord);                                \
        assert(p.types->type == CPI::Metadata::Property::CPI_##pretty);                \
        if (!p.is_writable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (length > p.sequence_size)                                        \
          throw;                                                        \
        if (p.write_error) \
          throw; /*"worker has errors before write */                        \
        char *cp = (char *)(getPropertyVaddr() + p.offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < length; i++) {                                \
          unsigned len = strlen(vals[i]);                                \
          if (len > p.types->size)                                        \
            throw; /* "string in sequence too long" */                        \
          memcpy(cp, vals[i], len+1);                                        \
        }                                                                \
        *(uint32_t *)(getPropertyVaddr() + p.offset) = length;                \
        if (p.write_error) \
          throw; /*"worker has errors after write */                        \
      }
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE_S
#undef CPI_DATA_TYPE
      // Get Scalar Property
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      virtual run get##pretty##Property(unsigned ord) {                        \
        CPI::Metadata::Property &p = property(ord);                                \
        if (!p.is_readable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (p.read_error ) \
          throw; /*"worker has errors before read "*/                        \
        uint32_t *pp = (uint32_t *)(getPropertyVaddr() + p.offset);                \
        union {                                                                \
                run r;                                                        \
                uint32_t u32[bits/32];                                        \
        } u;                                                                \
        if (bits > 32)                                                        \
          u.u32[1] = pp[1];                                                \
        u.u32[0] = pp[0];                                                \
        if (p.read_error )\
          throw; /*"worker has errors after read */                        \
        return u.r;                                                        \
      }                                                                        \
      unsigned get##pretty##SequenceProperty(unsigned ord, run *vals, unsigned length) { \
        CPI::Metadata::Property &p = property(ord);                                \
        if (!p.is_readable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (p.read_error ) \
          throw; /*"worker has errors before read "*/                        \
        uint32_t n = *(uint32_t *)(getPropertyVaddr() + p.offset);                \
        if (n > length)                                                        \
          throw; /* sequence longer than provided buffer */                \
        memcpy(vals, (void*)(getPropertyVaddr() + p.offset + p.maxAlign),        \
               n * sizeof(run));                                        \
        if (p.read_error ) \
          throw; /*"worker has errors after read */                        \
        return n;                                                        \
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void get##pretty##Property(unsigned ord, char *cp, unsigned length) { \
        CPI::Metadata::Property &p = property(ord);                                \
        assert(p.types->type == CPI::Metadata::Property::CPI_##pretty);                \
        if (!p.is_readable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (length < p.types->size+1)                                        \
          throw; /*"string buffer smaller than property"*/;                \
        if (p.read_error) \
          throw; /*"worker has errors before write */                        \
        uint32_t i32, *p32 = (uint32_t *)(getPropertyVaddr() + p.offset);        \
        memcpy(cp + 32/CHAR_BIT, p32 + 1, p.types->size + 1 - 32/CHAR_BIT); \
        i32 = *p32;                                                        \
        memcpy(cp, &i32, 32/CHAR_BIT);                                        \
        if (p.read_error) \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      unsigned get##pretty##SequenceProperty                                \
      (unsigned ord, run *vals, unsigned length, char *buf, unsigned space) { \
        CPI::Metadata::Property &p = property(ord);                                \
        assert(p.types->type == CPI::Metadata::Property::CPI_##pretty);                \
        if (!p.is_readable)                                                \
          throw; /*"attempt to get property that is not readable" */        \
        if (length > p.sequence_size)                                        \
          throw;                                                        \
        if (p.read_error ) \
          throw; /*"worker has errors before read */                        \
        uint32_t                                                        \
          n = *(uint32_t *)(getPropertyVaddr() + p.offset),                        \
          wlen = p.types->size + 1;                                        \
        if (n > length)                                                        \
          throw; /* sequence longer than provided buffer */                \
        char *cp = (char *)(getPropertyVaddr() + p.offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < n; i++) {                                \
          if (space < wlen)                                                \
            throw;                                                        \
          memcpy(buf, cp, wlen);                                        \
          cp += wlen;                                                        \
          vals[i] = buf;                                                \
          unsigned slen = strlen(buf) + 1;                                \
          buf += slen;                                                        \
          space -= slen;                                                \
        }                                                                \
        if (p.read_error)                                                \
          throw; /*"worker has errors after read */                        \
        return n;                                                        \
      }
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE_S
#undef CPI_DATA_TYPE
#define CPI_DATA_TYPE_S CPI_DATA_TYPE

        virtual void prepareProperty(CPI::Metadata::Property&, CPI::Container::Property&);


      CPI::Container::Port &  createInputPort(
                                              CPI::Container::PortId              portId,      
                                              CPI::OS::uint32_t   bufferCount,
                                              CPI::OS::uint32_t   bufferSize, 
                                              CPI::Util::PValue*  props=NULL
                                              )
        throw ( CPI::Util::EmbeddedException );


      CPI::Container::Port &  createOutputPort( 
                                               CPI::Container::PortId        portId,     
                                               CPI::OS::uint32_t             bufferCount,
                                               CPI::OS::uint32_t             bufferSize, 
                                               CPI::Util::PValue*               props=NULL
                                               )
        throw ( CPI::Util::EmbeddedException );




      // The following methods are WCI control ops and are all deprecated.
        std::string getLastControlError()
          throw ( CPI::Util::EmbeddedException );
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
      void setProperties( CPI::OS::uint32_t offset, CPI::OS::uint32_t nBytes, const void *p_data  );
      void getProperties( CPI::OS::uint32_t offset, CPI::OS::uint32_t nBytes, void *p_data  );

     
      // Get our transport
      CPI::DataTransport::Transport & getTransport();


    private:

      // Last error description
      std::string m_lastError;
      
      // Last WCI operation status
      WCI_status    m_wciStatus;


    protected:
      uint8_t * getPropertyVaddr();

      CPI::Util::PValue properties;  
      RCCWorkerInterface * m_rcc_worker;               

      CPI::Container::WorkerId                       workerId;        // Worker id
      bool                               enabled;                // Worker enabled flag

      // List of worker ports
      CPI::OS::uint32_t sourcePortCount;
      CPI::OS::uint32_t targetPortCount;

      // Sparse list, indexable by port ordinal
      CPI::Util::VList  sourcePorts;   
      CPI::Util::VList  targetPorts;        

      // Worker run condition super-set
      CPI::OS::uint32_t runConditionSS;

      // Last time that the worker was run
      CPI::OS::Timer               runTimer;
      CPI::OS::Timer::ElapsedTime  runTimeout;
                        
      // runtime control
      bool run_condition_met;

      // Debug/stats
      CPI::OS::uint32_t worker_run_count;
      virtual ~Worker();
      void stop(bool);

      // Override the port data based on hardcoded requirements from the worker
      void overRidePortInfo( CPI::Metadata::Port & portData );

      // Update a ports information (as a result of a connection)
      void updatePort( CPI::CP289::Port &port );



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

