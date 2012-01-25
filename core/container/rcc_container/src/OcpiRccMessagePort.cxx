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
 *   Container application context class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */
#include <OcpiRccPort.h>
#include <OcpiRccContainer.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <OcpiPortMetaData.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiPValue.h>
#include <OcpiUtilMisc.h>
#include <OcpiParentChild.h>
#include <OcpiBuffer.h>
#include <DtMsgDriver.h>
#include <OcpiRccWorker.h>

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OM = OCPI::Metadata;
namespace DTM = DataTransfer::Msg;


namespace OCPI {
  namespace RCC {

    MessagePort::
    MessagePort( Worker &w, Port& p, const OCPI::Util::PValue *params )
      : PortDelegator( w, p.metaPort(), 0, &p, params )
    {      
      // Empty
    }

    MessagePort::
    ~MessagePort()
    {
      delete m_msgChannel;      
    }
      
    void 
    MessagePort::
    connectURL( const char* url, const OCPI::Util::PValue *myProps,
			       const OCPI::Util::PValue *otherProps)
    {
      // See if we have a message driver that is capable of handling this message type
      
      DTM::XferFactory * factory = 
	DataTransfer::Msg::XferFactoryManager::getFactoryManager().findFactory( url, myProps, otherProps );
      if ( ! factory) {
	std::string err( "Message URL not supported");
	err += url;
	throw err;
      }
      DTM::XferServices * msgService  = factory->getXferServices(m_metaPort, url, myProps, otherProps );
      ocpiAssert ( msgService );
      m_msgChannel = msgService->getMsgChannel(url,myProps,otherProps);
      parent().portIsConnected(portOrdinal());
    }

    void 
    MessagePort::
    sendOutputBuffer( OCPI::DataTransport::BufferUserFacet* buffer, uint32_t len, uint8_t opcode)
    {
      m_msgChannel->sendOutputBuffer( buffer, len, opcode);
    }

    void 
    MessagePort::
    releaseInputBuffer( OCPI::DataTransport::BufferUserFacet* buffer)
    {
      ocpiAssert(m_metaPort.provider);
      m_msgChannel->releaseInputBuffer(buffer);
    }
    
    OCPI::DataTransport::BufferUserFacet*
    MessagePort::
    getNextEmptyOutputBuffer(void *&data, uint32_t &length)
    {
      return m_msgChannel->getNextEmptyOutputBuffer(data, length);
    }
    
    void 
    MessagePort::
    sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet*, unsigned int, uint8_t /*op*/  )
    {
      ocpiAssert(!"sendZcopyInputBuffer not supported with message ports !!\n");
    }
    
    OCPI::DataTransport::BufferUserFacet*
    MessagePort::
    getNextFullInputBuffer(void *&data, uint32_t &length, uint8_t &opcode)
    {
      return m_msgChannel->getNextFullInputBuffer(data, length, opcode);
    }

    void 
    MessagePort::
    disconnect()
      throw ( OCPI::Util::EmbeddedException )
    {
      // Empty
    }

    OCPI::Container::ExternalPort& 
    MessagePort::
    connectExternal(const char*, const OCPI::Util::PValue*,
		    const OCPI::Util::PValue*)
    {
      ocpiAssert( 0 );
      return *(OCPI::Container::ExternalPort *)0;
    }


    OCPI::DataTransport::BufferUserFacet* 
    MessagePort::
    getBuffer( uint32_t /*index*/)
    {
      ocpiAssert(!" MessagePort::getBuffer( uint32_t index ) NOTIMPLEMETED !!");
      return NULL;
    }

#if 0
    uint32_t 
    MessagePort::
    getBufferLength()
    {
      return 0;
    }
#endif
    uint32_t 
    MessagePort::
    getBufferCount()
    {
      return 1;
    }

  }
}
