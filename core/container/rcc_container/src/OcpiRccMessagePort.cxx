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

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OM = OCPI::Metadata;
namespace DTM = DataTransfer::Msg;


namespace OCPI {
  namespace RCC {

    MessagePort::
    MessagePort( Worker& w, const OCPI::Metadata::Port & pmd, const OCPI::Util::PValue *params )
      : PortDelegator( w, pmd, 0, params, NULL )
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
      DTM::XferServices * msgService  = factory->getXferServices( (OCPI::Util::Protocol*)&m_metaPort, url, myProps, otherProps );
      ocpiAssert ( msgService );
      m_msgChannel = msgService->getMsgChannel(url,myProps,otherProps);
    }

    void 
    MessagePort::
    advance( OCPI::DataTransport::BufferUserFacet* buffer, uint32_t opcode, uint32_t len)
    {
      (void)opcode;
      if ( ! m_metaPort.provider ) {
	m_msgChannel->post( buffer, len );
      }
      else {
	m_msgChannel->release( buffer );
      }
    }

    bool 
    MessagePort::
    hasEmptyOutputBuffer()
    {
      return m_msgChannel->hasFreeBuffer();
    }
    
    OCPI::DataTransport::BufferUserFacet*
    MessagePort::
    getNextEmptyOutputBuffer()
    {
      return m_msgChannel->getFreeBuffer();
    }
    
    bool 
    MessagePort::
    hasFullInputBuffer()
    {
      return m_msgChannel->msgReady();
    }

    void 
    MessagePort::
    sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet*, unsigned int  )
    {
      ocpiAssert(!"sendZcopyInputBuffer not supported with message ports !!\n");
    }
    
    OCPI::DataTransport::BufferUserFacet*
    MessagePort::
    getNextFullInputBuffer()
    {
      uint32_t length;
      return m_msgChannel->getNextMsg( length );
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

    uint32_t 
    MessagePort::
    getBufferLength()
    {
      return 0;
    }

    uint32_t 
    MessagePort::
    getBufferCount()
    {
      return 1;
    }

  }
}
