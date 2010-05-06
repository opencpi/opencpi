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

    class Controller;
    class RCCWorkerInterface;
    class Application;
    class Port;

    // Worker instance information structure
    class Worker : public CPI::Container::Worker
    {

    public:
      friend class Application;
      friend class Controller;
      friend class Port;
      friend class RCCWorkerInterface;


      Worker( CPI::Container::Application & app, const void* entryPoint, CPI::Util::PValue *wparams,
              ::RCCContainer* c, ezxml_t impl, ezxml_t inst );
      CPI::Container::Port& createPort(CPI::Metadata::Port&);


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

        /**
           @brief
           getLastControlError

           This method is used to get the last error that occured during a control
           operation.

           @param [ in ] workerId
           Container worker id.

           @retval std::string - last control error

           ****************************************************************** */
        std::string getLastControlError()
          throw ( CPI::Util::EmbeddedException );


        /**
           @brief
           Request a Worker to perform a control operation.

           The function control() sends a request to a WCI Worker for
           the specified operation. The function will block until the
           Worker responds indicating the success or failure of the
           operation or a timeout occurs.

           @param [ in ] h_worker
           The Worker handle provided to wci_control().

           @param [ in ] operation
           The WCI_control operation provided to wci_control().

           @param [ in ] options
           The WCI_options option provided to wci_control().

           @retval Success WCI_SUCCESS
           @retval Error   Error code that describes the error. Error
           code can be decoded with wci_strerror().

           ****************************************************************** */
        WCI_error control (  WCI_control operation, WCI_options options );

        /**
           @brief
           Query the Worker's status information.

           The function status() returns the status information for a
           Worker in the WCI_status structure.

           @param [ in ] h_worker
           The Worker handle provided to wci_status().

           @param [ out ] p_status
           The WCI_status operation provided to wci_status().
           Will contain a snapshot of the Worker's status
           information after the call completes.

           @retval Success WCI_SUCCESS
           @retval Error   Error code that describes the error. Error
           code can be decoded with wci_strerror().

           ****************************************************************** */
        WCI_error status ( WCI_status* p_status );



        /**
           @brief
           Perform a property space read request.

           The function read() reads the specified number of bytes from
           the Worker's property space. The function will block until the
           Worker responds indicating the success or failure of the
           operation or a timeout occurs.

           @param [ in ] h_worker
           The Worker handle provided to the WCI API function
           that initiated the read operation.

           @param [ in ] offset
           Offset into the property space to start the read.

           @param [ in ] nBytes
           Number of bytes to read from the property space.

           @param [ in ] data_type
           WCI_data_type value that specifies the type
           of data being read.

           @param [ in ] options
           WCI_options argument passed to the WCI API function
           that initiated the read operation.

           @param [ out ] p_data
           Upon successful completion the data read from
           the Worker's property space will be stored in
           this buffer. The caller is responsible for
           ensuring the provided buffer is large enough
           to hold the result of the read request.

           @retval Success WCI_SUCCESS
           @retval Error   Error code that describes the error. Error
           code can be decoded with wci_strerror().

           ****************************************************************** */
        WCI_error read ( WCI_u32 offset,
                         WCI_u32 nBytes,
                         WCI_data_type data_type,
                         WCI_options options,
                         void* p_data );

        /**
           @brief
           Perform a property space write.

           The function write() writes the specified number of bytes into
           the Worker's property space. The function will block until the
           Worker responds indicating the success or failure of the
           operation or a timeout occurs.

           @param [ in ] h_worker
           The Worker handle provided to the WCI API function
           that initiated the write operation.

           @param [ in ] offset
           Offset into the property space to start writing.

           @param [ in ] nBytes
           Number of bytes to write into the property space.

           @param [ in ] data_type
           WCI_data_type value that specifies the type
           of data being written.

           @param [ in ] options
           WCI_options argument passed to the WCI API function
           that initiated the write operation.

           @param [ in ] p_data
           Buffer that contains the data to be written into
           the Worker's property space.

           @retval Success WCI_SUCCESS
           @retval Error   Error code that describes the error. Error
           code can be decoded with wci_strerror().

           ****************************************************************** */
        WCI_error write ( WCI_u32 offset,
                          WCI_u32 nBytes,
                          WCI_data_type data_type,
                          WCI_options options,
                          const void* p_data );


        /**
           @brief
           Ends a session created by wci_open() and releases resources.

           The function close() frees the resources allocated for the
           Worker.  If it fails, the resources may not all be recovered.
           By default, the call to close () asserts the reset signal on the
           RPL worker unless the WCI_DO_NOT_RESET option is specified.

           @param [ in ] h_worker
           The Worker handle provided to the WCI API function
           that initiated the close operation.

           @param [ in ] options
           WCI_options argument passed to the WCI API function
           that initiated the close operation.

           @retval Success WCI_SUCCESS
           @retval Error   Error code that describes the error. Error
           code can be decoded with wci_strerror().

           ****************************************************************** */
        WCI_error close ( WCI_options options ) ;


      void stop();

      // Get our transport
      CPI::DataTransport::Transport & getTransport();

      // Set/get properties
      void setProperties( CPI::OS::uint32_t offset, CPI::OS::uint32_t nBytes, const void *p_data  );
      void getProperties( CPI::OS::uint32_t offset, CPI::OS::uint32_t nBytes, void *p_data  );


    private:

      // Last error description
      std::string m_lastError;
      
      // Last WCI operation status
      WCI_status    m_wciStatus;


    protected:

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
      

      public:      

      inline void initialize(){control(WCI_CONTROL_INITIALIZE, WCI_DEFAULT );}
      inline  void start(){ control(WCI_CONTROL_START, WCI_DEFAULT );}
      inline void release(){ control(WCI_CONTROL_RELEASE, WCI_DEFAULT );}
      inline  void beforeQuery(){control(WCI_CONTROL_BEFORE_QUERY, WCI_DEFAULT );}
      inline  void afterConfigure(){control(WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );}
      inline void test(){control(WCI_CONTROL_TEST, WCI_DEFAULT );}

      // Dont know how this relates to a CP289 worker
      virtual void prepareProperty(CPI::Metadata::Property&, CPI::Container::Property&){};

      
      inline void setBoolProperty(unsigned int where, bool b)
        { setProperties( where, sizeof(bool), &b); }

      inline void setBoolSequenceProperty(unsigned int where, const bool * b, unsigned int count )
        { setProperties( where, sizeof(bool)*count, &b); }

      inline void setCharProperty(unsigned int where, char c)
        { setProperties( where, sizeof(char), &c); }

      inline void setCharSequenceProperty(unsigned int where, const char* p, unsigned int count)
        {setProperties( where, sizeof(p)*count, &p); }

      inline void setDoubleProperty(unsigned int where, double p)
        {setProperties( where, sizeof(p), &p); }

      inline void setDoubleSequenceProperty(unsigned int where, const double*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setFloatProperty(unsigned int where, float p)
        {setProperties( where, sizeof(p), &p); }

      inline void setFloatSequenceProperty(unsigned int where, const float*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setShortProperty(unsigned int where, int16_t p)
        {setProperties( where, sizeof(p), &p); }

      inline void setShortSequenceProperty(unsigned int where, const int16_t*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setLongProperty(unsigned int where, int32_t p)
        {setProperties( where, sizeof(p), &p); }

      inline void setLongSequenceProperty(unsigned int where, const int32_t*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setUCharProperty(unsigned int where, uint8_t p)
        {setProperties( where, sizeof(p), &p); }

      inline void setUCharSequenceProperty(unsigned int where, const uint8_t*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setULongProperty(unsigned int where, uint32_t p)
        {setProperties( where, sizeof(p), &p); }

      inline void setULongSequenceProperty(unsigned int where, const uint32_t*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setUShortProperty(unsigned int where, uint16_t p)
        {setProperties( where, sizeof(p), &p); }

      inline void setUShortSequenceProperty(unsigned int where, const uint16_t*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setLongLongProperty(unsigned int where, int64_t p)
        {setProperties( where, sizeof(p), &p); }

      inline void setLongLongSequenceProperty(unsigned int where, const int64_t*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setULongLongProperty(unsigned int where, uint64_t p)
        {setProperties( where, sizeof(p), &p); }

      inline void setULongLongSequenceProperty(unsigned int where, const uint64_t*p, unsigned int count)
        {setProperties( where, sizeof(p)*count, p); }

      inline void setStringProperty(unsigned int where, const char* p)
        {cpiAssert(!"Not Implemented!!");}

      inline void setStringSequenceProperty(unsigned int where, const char**p, unsigned int count)
        {cpiAssert(!"Not Implemented!!");}


      inline bool getBoolProperty(unsigned int where)
        { bool b;
          getProperties( where, sizeof(bool), &b);
          return b;
        }

      inline unsigned int getBoolSequenceProperty(unsigned int where, bool*b, unsigned int count)
        {  getProperties( where, sizeof(bool)*count, b);
          return count;
        }

      inline char getCharProperty(unsigned int where)
        { char p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getCharSequenceProperty(unsigned int where, char*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline double getDoubleProperty(unsigned int where)
        { double p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getDoubleSequenceProperty(unsigned int where, double*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline float getFloatProperty(unsigned int where)
        { float p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getFloatSequenceProperty(unsigned int where, float*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline int16_t getShortProperty(unsigned int where)
        { int16_t p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getShortSequenceProperty(unsigned int where, int16_t*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline int32_t getLongProperty(unsigned int where)
        { int32_t p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getLongSequenceProperty(unsigned int where, int32_t*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline uint8_t getUCharProperty(unsigned int where)
        { int8_t p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getUCharSequenceProperty(unsigned int where, uint8_t*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }


      inline uint32_t getULongProperty(unsigned int where)
        { int32_t p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getULongSequenceProperty(unsigned int where, uint32_t*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline uint16_t getUShortProperty(unsigned int where)
        { int16_t p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getUShortSequenceProperty(unsigned int where, uint16_t*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline int64_t getLongLongProperty(unsigned int where)
        { int64_t p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getLongLongSequenceProperty(unsigned int where, int64_t*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline uint64_t getULongLongProperty(unsigned int where)
        { int64_t p;
          getProperties( where, sizeof(p), &p);
          return p;
        }

      inline unsigned int getULongLongSequenceProperty(unsigned int where, uint64_t*p, unsigned int count)
        {  getProperties( where, sizeof(p)*count, p);
          return count;
        }

      inline void getStringProperty(unsigned int where, char*, unsigned int)
        {cpiAssert(!"Not Implemented!!");}

      inline unsigned int getStringSequenceProperty(unsigned int where, char**, unsigned int, char*p, unsigned int count)
        {cpiAssert(!"Not Implemented!!"); return 0;}

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

