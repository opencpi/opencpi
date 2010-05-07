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
*   This file contains the driver implementation for creating RCC containers.
*
* Revision History: 

    03/02/2009 - John Miller
    Added event range for transport system.

    2/7/2009  - John Miller
    Fixed the "polled" flag because its definition was inverted.

    9/2008    - John Miller
    Initial version.

*
*/

#include <CpiContainerInterface.h>
#include <DtIntEventHandler.h>
#include <CpiContainerDriver.h>
#include <CpiOsAssert.h>
#include <CpiContainerErrorCodes.h>
#include <CpiCP289Container.h>
#include <CpiTransportGlobal.h>

//#include <DtTransferInternal.h>

using namespace CPI::Container;

static int event_range_start=0;

// Static registration of our factory
static Driver* cf = new Driver();

Driver::Driver()
  throw ( )
  :  CPI::Util::Driver("Container", "RCC", false),
     m_tpg_events(NULL),
     m_tpg_no_events(NULL)
{
  // Empty 
}


static int g_cid=1;
CPI::Util::Device *
Driver::
probe(const CPI::Util::PValue* props, const char *which )
  throw ( CPI::Util::EmbeddedException )
{
  int cid = g_cid++;
  cid += getpid();
  bool polled=true;
  for (int n=0; props[n].name; n++) {
    if (strcmp(props[n].name, "polled") == 0) {
      if (props[n].type != CPI::Metadata::Property::CPI_Bool) {
        throw CPI::Util::EmbeddedException(" \"Polled\" property has wrong type, should be Bool");
      }
      polled = props[n].vBool;
    }
  }

  CPI::DataTransport::TransportGlobal *  tpg=NULL;
  bool use_events= !polled;
  if ( use_events ) {
    if ( m_tpg_events == NULL ) {
      try {
        m_tpg_events = 
          new CPI::DataTransport::TransportGlobal( event_range_start++, use_events );
      }
      catch( std::bad_alloc ) {
        throw CPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
      }
    }
    tpg = m_tpg_events;
  }
  else {
    if ( m_tpg_no_events == NULL ) {    
      try {
        m_tpg_no_events = 
          new CPI::DataTransport::TransportGlobal( event_range_start++, use_events );
      }
      catch( std::bad_alloc ) {
        throw CPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
      }
    }
    tpg = m_tpg_no_events;
  }


  CPI::CP289::Container* container;
  // Our general purpose container implementation (Co-located)
  try {
    container = new CPI::CP289::Container(*this, cid, tpg, props );
  }
  catch( std::bad_alloc ) {
    throw CPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }

  return container;
}

Driver::~Driver()
  throw ( )
{
  if ( m_tpg_no_events ) delete m_tpg_no_events;
  if ( m_tpg_events ) delete m_tpg_events;
}




