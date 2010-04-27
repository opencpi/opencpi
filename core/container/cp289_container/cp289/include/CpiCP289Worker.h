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
					       CPI::Util::PValue*	       props=NULL
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

      CPI::Container::WorkerId                       workerId;	// Worker id
      bool		               enabled;		// Worker enabled flag

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

      Worker( CPI::Container::Application & app, const void* entryPoint, CPI::Util::PValue *wparams,
	      ::RCCContainer* c );
      virtual ~Worker();
      void stop(bool);

      // Override the port data based on hardcoded requirements from the worker
      void overRidePortInfo( CPI::Container::PortData * portData, CPI::Container::PortId portId  );


      // Update a ports information (as a result of a connection)
      void updatePort( CPI::CP289::Port &port );
      

      public:      

      virtual void initialize(){};
      virtual void start(){};
      virtual void release(){};
      virtual void beforeQuery(){};
      virtual void afterConfigure(){};
      virtual void test(){};

      // The following are stubbed until we merge for real !!
      virtual void prepareProperty(CPI::Metadata::Property&, CPI::Container::Property&){};
      virtual CPI::Container::Port& createPort(CPI::Metadata::Port&){};
      virtual void setBoolProperty(unsigned int, bool){};
      virtual void setBoolSequenceProperty(unsigned int, const bool*, unsigned int){};
      virtual void setCharProperty(unsigned int, char){};
      virtual void setCharSequenceProperty(unsigned int, const char*, unsigned int){};
      virtual void setDoubleProperty(unsigned int, double){};
      virtual void setDoubleSequenceProperty(unsigned int, const double*, unsigned int){};
      virtual void setFloatProperty(unsigned int, float){};
      virtual void setFloatSequenceProperty(unsigned int, const float*, unsigned int){};
      virtual void setShortProperty(unsigned int, int16_t){};
      virtual void setShortSequenceProperty(unsigned int, const int16_t*, unsigned int){};
      virtual void setLongProperty(unsigned int, int32_t){};
      virtual void setLongSequenceProperty(unsigned int, const int32_t*, unsigned int){};
      virtual void setUCharProperty(unsigned int, uint8_t){};
      virtual void setUCharSequenceProperty(unsigned int, const uint8_t*, unsigned int){};
      virtual void setULongProperty(unsigned int, uint32_t){};
      virtual void setULongSequenceProperty(unsigned int, const uint32_t*, unsigned int){};
      virtual void setUShortProperty(unsigned int, uint16_t){};
      virtual void setUShortSequenceProperty(unsigned int, const uint16_t*, unsigned int){};
      virtual void setLongLongProperty(unsigned int, int64_t){};
      virtual void setLongLongSequenceProperty(unsigned int, const int64_t*, unsigned int){};
      virtual void setULongLongProperty(unsigned int, uint64_t){};
      virtual void setULongLongSequenceProperty(unsigned int, const uint64_t*, unsigned int){};
      virtual void setStringProperty(unsigned int, const char*){};
      virtual void setStringSequenceProperty(unsigned int, const char**, unsigned int){};
      virtual bool getBoolProperty(unsigned int){return true;};
      virtual unsigned int getBoolSequenceProperty(unsigned int, bool*, unsigned int){return 0;};
      virtual char getCharProperty(unsigned int){return 0;};
      virtual unsigned int getCharSequenceProperty(unsigned int, char*, unsigned int){return 0;};
      virtual double getDoubleProperty(unsigned int){return 0;};
      virtual unsigned int getDoubleSequenceProperty(unsigned int, double*, unsigned int){return 0;};
      virtual float getFloatProperty(unsigned int){return 0;};
      virtual unsigned int getFloatSequenceProperty(unsigned int, float*, unsigned int){return 0;};
      virtual int16_t getShortProperty(unsigned int){return 0;};
      virtual unsigned int getShortSequenceProperty(unsigned int, int16_t*, unsigned int){return 0;};
      virtual int32_t getLongProperty(unsigned int){return 0;};
      virtual unsigned int getLongSequenceProperty(unsigned int, int32_t*, unsigned int){return 0;};
      virtual uint8_t getUCharProperty(unsigned int){return 0;};
      virtual unsigned int getUCharSequenceProperty(unsigned int, uint8_t*, unsigned int){return 0;};
      virtual uint32_t getULongProperty(unsigned int){return 0;};
      virtual unsigned int getULongSequenceProperty(unsigned int, uint32_t*, unsigned int){return 0;};
      virtual uint16_t getUShortProperty(unsigned int){return 0;};
      virtual unsigned int getUShortSequenceProperty(unsigned int, uint16_t*, unsigned int){return 0;};
      virtual int64_t getLongLongProperty(unsigned int){return 0;};
      virtual unsigned int getLongLongSequenceProperty(unsigned int, int64_t*, unsigned int){return 0;};
      virtual uint64_t getULongLongProperty(unsigned int){return 0;};
      virtual unsigned int getULongLongSequenceProperty(unsigned int, uint64_t*, unsigned int){return 0;};
      virtual void getStringProperty(unsigned int, char*, unsigned int){};
      virtual unsigned int getStringSequenceProperty(unsigned int, char**, unsigned int, char*, unsigned int){return 0;};

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

