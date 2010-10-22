
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

#include <DtTransferInternal.h>
#include <OcpiPortMetaData.h>
#include <OcpiPortSetMetaData.h>
#include <OcpiOsAssert.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;


void PortMetaData::
init()
{
  // A circuit is created in parts, so we may not know the real location of this
  // port just yet
  std::string nuls;
  if ( real_location_string.length()  ) {
    m_real_tfactory = 
      XferFactoryManager::getFactoryManager().find( real_location_string, nuls );
    if ( ! m_real_tfactory ) {
      std::string ex("Endpoint Not Supported ->");
      ex += real_location_string.c_str();
      throw OCPI::Util::EmbeddedException( ex.c_str() );
    }
    m_real_location = m_real_tfactory->getEndPoint(real_location_string );
  }

  if ( shadow_location_string.length() ) {
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

  m_bufferData = new BufferOffsets[MAX_BUFFERS];
  memset( m_bufferData, 0, sizeof(BufferOffsets[MAX_BUFFERS]));
  m_localPortSetControl = 0;

}

// Standard constructor 
PortMetaData::
PortMetaData( PortOrdinal pid, 
              bool s, 
              OCPI::RDT::Descriptors& port,
              const char* shadow_ep,
              PortSetMetaData* psmd )
  : CU::Child<PortSetMetaData,PortMetaData>(*psmd), remoteCircuitId(-1),
    remotePortId(-1),id(-1),rank(0),output(s),user_data(NULL),
    m_init(false),m_real_location(NULL),m_shadow_location(NULL)
{
  m_bufferData = NULL;
  m_shadow = true;
  m_descriptor = port;
  if ( port.desc.oob.oep ) {
    real_location_string = port.desc.oob.oep;
  }
  if ( shadow_ep ){
    shadow_location_string = shadow_ep;
  }
  id = pid;
  m_portSetMd = psmd;
  init( );
}



PortMetaData::
PortMetaData( PortOrdinal pid, 
              OCPI::RDT::Descriptors& portDesc,
              PortSetMetaData* psmd )
  : CU::Child<PortSetMetaData,PortMetaData>(*psmd), remoteCircuitId(-1),
    remotePortId(-1),id(-1),rank(0),user_data(NULL),
    m_init(false),m_real_location(NULL),m_shadow_location(NULL)
{
  m_descriptor = portDesc;
  output =  (m_descriptor.type == OCPI::RDT::ProducerDescT) ? true : false;
  m_bufferData = NULL;
  m_shadow = true;
  if ( portDesc.desc.oob.oep ) {
    real_location_string = portDesc.desc.oob.oep;
  }
  id = pid;
  m_portSetMd = psmd;
  init( );
}



PortMetaData::
PortMetaData( PortOrdinal pid, 
              bool s, 
              const char* ep,
              const char* shadow_ep,
              PortSetMetaData* psmd )
  : CU::Child<PortSetMetaData,PortMetaData>(*psmd),remoteCircuitId(-1),
    remotePortId(-1),id(-1),rank(0),output(s),user_data(NULL),
    m_init(false),m_real_location(NULL),m_shadow_location(NULL)
{
  m_descriptor.type = s = true ? OCPI::RDT::ProducerDescT : OCPI::RDT::ConsumerDescT;
  m_bufferData = NULL;
  m_shadow = true;
  if ( ep ) {
    real_location_string = ep;
  }
  if ( shadow_ep ){
    shadow_location_string = shadow_ep;
  }
  id = pid;
  m_portSetMd = psmd;
  init( );
}


// Dependency constructor
PortMetaData::
PortMetaData( PortOrdinal pid, 
              bool s, 
              const char* ep, 
              const char* shadow_ep,
              OCPI::RDT::Descriptors&  pdd , 
              OCPI::OS::uint32_t circuitId,
              PortSetMetaData* psmd )
  : CU::Child<PortSetMetaData,PortMetaData>(*psmd),
    remotePortId(-1),id(-1),rank(0),output(s),user_data(NULL),
    m_portSetMd(psmd), m_init(false)
{
  ( void ) circuitId;
  m_bufferData = NULL;
  m_shadow = true;
  if ( ep ) {
    real_location_string = ep;
  }
  if ( shadow_ep ){
    shadow_location_string = shadow_ep;
  }
  id = pid;

  memcpy(&m_externPortDependencyData,&pdd.desc,sizeof(OCPI::RDT::Descriptors) );
  psmd->bufferCount  = pdd.desc.nBuffers;
  psmd->bufferLength = pdd.desc.dataBufferSize;

#ifndef NDEBUG
  printf("External port buffer ep = %s\n", pdd.desc.oob.oep );
#endif

  // In this case the "real" location of the port is its actual physical location,
  // the shadow port in here with its endpoint equal to the endpoint of the output
  // port
  real_location_string = pdd.desc.oob.oep;
  shadow_location_string = ep;
  remotePortId = pdd.desc.oob.port_id;

  std::string nuls;
  m_real_tfactory = 
    XferFactoryManager::getFactoryManager().find( real_location_string, nuls );
  if ( ! m_real_tfactory ) {
    std::string ex("Endpoint Not Supported ->");
    ex += real_location_string.c_str();
    throw OCPI::Util::EmbeddedException( ex.c_str() );
  }
  m_real_location = m_real_tfactory->getEndPoint( real_location_string );

  if ( shadow_location_string.length() ) {
    m_shadow_tfactory = 
      XferFactoryManager::getFactoryManager().find( shadow_location_string, nuls );
    if ( ! m_shadow_tfactory ) {
      std::string ex("Endpoint Not Supported ->");
      ex += shadow_location_string.c_str();
      throw OCPI::Util::EmbeddedException( ex.c_str() );    
    }
    m_shadow_location = m_shadow_tfactory->getEndPoint( shadow_location_string );
  }
  else {
    m_shadow_location = NULL;
    m_shadow_tfactory = NULL;
  }

  m_bufferData = new BufferOffsets[MAX_BUFFERS];
  memset( m_bufferData, 0, sizeof(BufferOffsets[MAX_BUFFERS]));
  m_localPortSetControl = 0;
        
  for ( unsigned int n=0; n<pdd.desc.nBuffers; n++ ) {
    m_bufferData[n].inputOffsets.bufferOffset = 
      pdd.desc.dataBufferBaseAddr + (n*pdd.desc.dataBufferPitch);

    m_bufferData[n].inputOffsets.bufferSize = pdd.desc.dataBufferSize;

    m_bufferData[n].inputOffsets.localStateOffset =  
      pdd.desc.fullFlagBaseAddr + (n*pdd.desc.fullFlagPitch);

    m_bufferData[n].inputOffsets.metaDataOffset = 
      pdd.desc.metaDataBaseAddr + (n*pdd.desc.metaDataPitch);

  }

  // Get the rest of the connection data
  this->m_portSetMd->bufferCount = pdd.desc.nBuffers;
  this->m_portSetMd->bufferLength = pdd.desc.dataBufferSize;

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








