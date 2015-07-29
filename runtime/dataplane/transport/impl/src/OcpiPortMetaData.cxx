
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
 *   This file contains the implementation for the Ocpi port meta data class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include "OcpiOsAssert.h"
#include "DtTransferInternal.h"
#include "OcpiUtilMisc.h"
#include "OcpiPortSetMetaData.h"
#include "OcpiPortMetaData.h"

using namespace OCPI::DataTransport;
using namespace DataTransfer;


void PortMetaData::
init()
{
  if (m_init)
    return;
  m_init = true;
  // A circuit is created in parts, so we may not know the real location of this
  // port just yet
  std::string nuls;
  if ( !m_real_location && real_location_string.length()  ) {
    m_real_tfactory = 
      XferFactoryManager::getFactoryManager().find( real_location_string, nuls );
    if ( ! m_real_tfactory ) {
      std::string ex("Endpoint Not Supported ->");
      ex += real_location_string.c_str();
      throw OCPI::Util::EmbeddedException( ex.c_str() );
    }
    m_real_location = m_real_tfactory->getEndPoint(real_location_string );
  }

  if ( !m_shadow_location && shadow_location_string.length() ) {
    m_shadow_tfactory = 
      XferFactoryManager::getFactoryManager().find( nuls, shadow_location_string );
    if ( ! m_shadow_tfactory ) {
      std::string ex("Endpoint Not Supported ->");
      ex += shadow_location_string.c_str();
      throw OCPI::Util::EmbeddedException( ex.c_str() );
    }
    m_shadow_location = m_shadow_tfactory->getEndPoint(shadow_location_string );
  }
  else {
    m_shadow_location = NULL;
    m_shadow_tfactory = NULL;
  }

  m_bufferData = new BufferOffsets[m_portSetMd->bufferCount];
  memset( m_bufferData, 0, sizeof(BufferOffsets[m_portSetMd->bufferCount]));
  m_localPortSetControl = 0;

}

// Standard constructor 
PortMetaData::
PortMetaData( PortOrdinal pid, 
              bool s, 
	      DataTransfer::EndPoint *ep,
              const OCPI::RDT::Descriptors& desc,
              PortSetMetaData* psmd )
  : CU::Child<PortSetMetaData,PortMetaData>(*psmd, *this), m_shadow(true), remoteCircuitId(0),
    remotePortId(-1),
    id(pid),rank(0),output(s),user_data(NULL),
    m_portSetMd(psmd),m_init(false),m_real_location(ep),m_shadow_location(NULL),
    m_bufferData(NULL)
{
  m_descriptor = desc;
  if (ep)
    real_location_string = ep->end_point;
  else
    real_location_string = desc.desc.oob.oep;
  init( );
}

PortMetaData::
PortMetaData( PortOrdinal pid, 
	      DataTransfer::EndPoint &ep, 
              const OCPI::RDT::Descriptors& portDesc,
              PortSetMetaData* psmd )
  : CU::Child<PortSetMetaData,PortMetaData>(*psmd, *this),m_shadow(true),remoteCircuitId(0),
    remotePortId(-1),
    id(pid),rank(0),user_data(NULL),
    m_portSetMd(psmd),m_init(false),m_real_location(&ep),m_shadow_location(NULL),
    m_bufferData(NULL)
{
  m_descriptor = portDesc;
  output =  (m_descriptor.type == OCPI::RDT::ProducerDescT) ? true : false;
  real_location_string = ep.end_point;
  init( );
}

PortMetaData::
PortMetaData( PortOrdinal pid, 
              bool s, 
              EndPoint *ep,
              EndPoint* shadow_ep,
              PortSetMetaData* psmd )
  : CU::Child<PortSetMetaData,PortMetaData>(*psmd, *this),m_shadow(true),remoteCircuitId(0),
    remotePortId(-1),
    id(pid),rank(0),output(s),user_data(NULL),
    m_portSetMd(psmd),m_init(false),m_real_location(ep),m_shadow_location(shadow_ep),
    m_bufferData(NULL)
{
  m_descriptor.type = s ? OCPI::RDT::ProducerDescT : OCPI::RDT::ConsumerDescT;
  m_descriptor.role = s ? OCPI::RDT::ActiveMessage : OCPI::RDT::ActiveFlowControl;
  if ( ep ) {
    real_location_string = ep->end_point;
  }
  if ( shadow_ep ){
    shadow_location_string = shadow_ep->end_point;
  }
  init( );
}


// Dependency constructor
PortMetaData::
PortMetaData( PortOrdinal pid, 
              DataTransfer::EndPoint &ep, 
              DataTransfer::EndPoint &shadow_ep, 
              const OCPI::RDT::Descriptors &pdd, 
              PortSetMetaData* psmd )
  : CU::Child<PortSetMetaData,PortMetaData>(*psmd, *this),m_shadow(true),
    real_location_string(shadow_ep.end_point),
    shadow_location_string(ep.end_point),
    remotePortId(OCPI_UTRUNCATE(PortOrdinal,pdd.desc.oob.port_id)),
    id(pid),rank(0),output(false),user_data(NULL),
    m_portSetMd(psmd), m_init(false),
    m_real_location(&shadow_ep),  m_real_tfactory(shadow_ep.factory),
    m_shadow_location(&ep),  m_shadow_tfactory(ep.factory),
    m_bufferData(NULL)
{
  memcpy(&m_externPortDependencyData,&pdd.desc,sizeof(OCPI::RDT::Descriptors) );
  psmd->bufferCount  = pdd.desc.nBuffers;
  psmd->bufferLength = pdd.desc.dataBufferSize;

  ocpiDebug("Remote port buffer ep = %s", pdd.desc.oob.oep );

  m_bufferData = new BufferOffsets[psmd->bufferCount];
  memset( m_bufferData, 0, sizeof(BufferOffsets) * psmd->bufferCount);
  m_localPortSetControl = 0;
        
  for ( unsigned int n=0; n<pdd.desc.nBuffers; n++ ) {
    m_bufferData[n].inputOffsets.bufferOffset = 
      OCPI_UTRUNCATE(OU::ResAddr, pdd.desc.dataBufferBaseAddr + (n*pdd.desc.dataBufferPitch));

    m_bufferData[n].inputOffsets.bufferSize = pdd.desc.dataBufferSize;

    m_bufferData[n].inputOffsets.localStateOffset =  
      OCPI_UTRUNCATE(OU::ResAddr, pdd.desc.fullFlagBaseAddr + (n*pdd.desc.fullFlagPitch));

    m_bufferData[n].inputOffsets.metaDataOffset = 
      OCPI_UTRUNCATE(OU::ResAddr, pdd.desc.metaDataBaseAddr + (n*pdd.desc.metaDataPitch));

  }
}

PortMetaData::
~PortMetaData()
{
  if ( ! m_init ) {
    delete[] m_bufferData;
    return;
  }
  if ( ! m_real_location ) {
    return;
  }



}








