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

/*
 *  John Miller -  12/2010
 *  Initial version
 *
 */

        
#ifndef Dt_Msg_Driver_h
#define Dt_Msg_Driver_h

#include <vector>
#include <string.h>
#include "ezxml.h"
#include <OcpiOsDataTypes.h>
#include <OcpiList.h>
#include <OcpiDriverManager.h>
#include <OcpiUtilProtocol.h>

namespace OCPI {
  namespace DataTransport {
    class BufferUserFacet;
  }
}

namespace DataTransfer {

  namespace Msg {



    // This is the base class for a factory configuration sheet
    // that is common to the manager, drivers, and devices
    class FactoryConfig {
    public:
      // These are arguments to allow different defaults
      FactoryConfig(uint32_t retryCount = 0);
      void parse(FactoryConfig *p, ezxml_t config );
      size_t m_retryCount;
      ezxml_t  m_xml; // the element that these attributes were parsed from
    };

    class XferServices;
    class XferFactoryManager;
    class XferFactory
      : public OCPI::Driver::DriverType<XferFactoryManager, XferFactory>,
      public FactoryConfig
      {
      public:

	XferFactory(const char *);
                 
	// Destructor
	virtual ~XferFactory();
                 
	// Configure from xml
	void configure(ezxml_t x);

	// Get our protocol string
	virtual const char* getProtocol()=0;

	// Supports transfer query
	virtual bool supportsTx( const char* url,
				 const OCPI::Util::PValue *our_props=0,
				 const OCPI::Util::PValue *other_props=0 )=0;

	/***************************************
	 *  This method is used to get a transfer service object. The implementation
	 * of this method should cache the service object if possible so that they
	 * can be re-used.
	 ***************************************/
	virtual XferServices* getXferServices( const OCPI::Util::Protocol &protocol,
					       const char* other_url,
					       const OCPI::Util::PValue *our_props=0,
					       const OCPI::Util::PValue *other_props=0 )=0;
      protected:


      };


    // This is the transfer factory manager
    // - the driver manager for transfer drivers
    extern const char *msg_transfer;
    class XferFactoryManager : 
    public OCPI::Driver::ManagerBase<XferFactoryManager, XferFactory, msg_transfer >,
      public FactoryConfig
      {
      public:
	friend class XferFactory;
	inline static XferFactoryManager& getFactoryManager() {
	  return getSingleton();
	}

	// Configure the manager and it drivers
	void configure(  ezxml_t config );


	// Get the factory from the url
	XferFactory* findFactory( const char* url,
				  const OCPI::Util::PValue *our_props=0,
				  const OCPI::Util::PValue *other_props=0 );

	// Constructors/Destructors
	XferFactoryManager();
	~XferFactoryManager();

      protected:

	OCPI::OS::Mutex    m_mutex;
	//	FactoryConfig      m_config;
	bool            m_configured;

      };


    // OCPI::Driver::Device is virtually inherited to give access
    // to the class that is not normally inherited here.
    // This also delays the calling of the destructor
    class Device : public FactoryConfig, virtual public OCPI::Driver::Device {
    public:
      Device(){};
      virtual XferFactory &driverBase() = 0;
    protected:
      void configure(ezxml_t x);
    };



    // A single request to perform a data transfer, this is effectively a transfer 
    // template that is used aOCPI::OS::int32_t with a transfer service object to 
    // describe a transfer.  This base class is specialized by each transfer driver
    class MsgChannel
    {
    public:
   
      // Constructor
      MsgChannel(){};

      typedef enum {
	CompleteSuccess = 0,
	CompleteFailure = 1,
	Pending = 2
      } CompletionStatus;

      /*
       * Queue Msg Transfer Request
       */
      virtual void sendOutputBuffer(OCPI::DataTransport::BufferUserFacet* buffer,
				    size_t msg_size, uint8_t opcode = 0 ) = 0;

      /*
       * Release a buffer previously leased by nextMsg
       */
      virtual void releaseInputBuffer( OCPI::DataTransport::BufferUserFacet*  buffer) = 0;    

      /*
       * Get a free output buffer if one is available
       */
      virtual  OCPI::DataTransport::BufferUserFacet*
	getNextEmptyOutputBuffer(void *&data, size_t &length) = 0;    

      /*
       *  Get data on the channel, If no data is available this may block
       */
      virtual OCPI::DataTransport::BufferUserFacet*
	getNextFullInputBuffer(void *&data, size_t & length, uint8_t &opcode ) = 0;

      // Destructor - Note that invoking OcpiXferServices::Release is the preferred method.
      virtual ~MsgChannel () {};

    };


    // Driver dependent data transfer services.  Instances of the driver classes that
    // inherit this base class manage a connection between a local and remote 
    // endpoint and create transfers between them (instances of the driver class that 
    // inherits from MsgManager above).
    class XferServices 
    {
                
    public:
                 
      /*
       * Create tranfer services template
       *        Arguments:
       *                p1        - Source memory instance
       *                p2        - Destination memory instance
       *        Returns: void
       *
       *        Errors:
       *                DataTransferEx for all exception conditions
       */
      XferServices (  const OCPI::Util::Protocol & p, 
		      const char  *a_url,
		      const OCPI::Util::PValue *our_props=0,
		      const OCPI::Util::PValue *other_props=0 )
	: m_protocol(p),m_url(a_url)
	{(void)our_props;(void)other_props;attach(); };

      /*
       * Finalize the connection with the remote cookie
       */
      virtual void finalize( uint64_t /* cookie */ ){}

                 
      /*
       * Create tranfer request object
       */
      virtual MsgChannel* getMsgChannel( const char  * url,
		      const OCPI::Util::PValue *our_props=0,
					 const OCPI::Util::PValue *other_props=0 )=0;

      /*
       * use counter
       */
      inline void attach(){m_use_count++;}
      inline void release(){m_use_count--;}
      

      /*
       * Member access
       */
      inline std::string & url(){return m_url;}
      inline const OCPI::Util::Protocol & protocol(){return m_protocol;}

                 
      virtual ~XferServices () {};

    private:
      int m_use_count;
      // FIXME:  make sure the life cycle is ok for this to be a reference
      const OCPI::Util::Protocol  &m_protocol;
      std::string m_url;


    };


    // The template class directly inherited by concrete transfer drivers, with the
    // requirement that the name of the driver be passed to the template
    // using a static const char* variable.
    // We inherit XferFactory class here among other things.
    template <class ConcreteDriver, class ConcreteDevice, class ConcreteSvcs,
      const char *&name>
      class DriverBase :
    public OCPI::Driver::DriverBase<XferFactoryManager, XferFactory,
      ConcreteDriver, ConcreteDevice, name>,
      public OCPI::Util::Parent<ConcreteSvcs>
      {
      };
    template <class Dri, class Dev>
      class DeviceBase :
    public OCPI::Driver::DeviceBase<Dri,Dev>,
      public Device
      {
      protected:
	DeviceBase<Dri, Dev>(const char *childName, Dev &dev)
	  : OCPI::Driver::DeviceBase<Dri, Dev>(childName, dev)
	  {}
	  inline XferFactory &driverBase() {
	    return OCPI::Driver::DeviceBase<Dri,Dev>::parent();
	  }
      };

    // The template class  directory inherited by concrete connecion classes 
    // (a.k.a. XferServices)
    template <class ConcDri, class ConcConn, class ConcXfer>
      class ConnectionBase :
    public OCPI::Util::Child<ConcDri,ConcConn>,
      public OCPI::Util::Parent<ConcXfer>,
      public XferServices
      {
      protected:
	ConnectionBase<ConcDri, ConcConn, ConcXfer>(ConcConn &conn,
						    const OCPI::Util::Protocol &a_protocol,
						    const char* other_url,
						    const OCPI::Util::PValue *our_props=0,
						    const OCPI::Util::PValue *other_props=0 )
	  : OCPI::Util::Child<ConcDri,ConcConn> (OCPI::Util::Singleton<ConcDri>::
						 getSingleton(), conn),
	  XferServices(a_protocol,other_url,our_props,other_props)
	    {}
      };
    template <class ConcConn, class ConcXfer>
      class TransferBase :
    public MsgChannel,
      public OCPI::Util::Child<ConcConn, ConcXfer>
      {
      protected:
	TransferBase<ConcConn, ConcXfer>(ConcConn &conn, ConcXfer &xfer)
	  : OCPI::Util::Child<ConcConn,ConcXfer>(conn, xfer) {}
      };
    template <class Dri>
      class RegisterTransferDriver
      : OCPI::Driver::Registration<Dri>
      {};



  }
}
#endif
