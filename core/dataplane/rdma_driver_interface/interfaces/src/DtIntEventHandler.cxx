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

/**
   @file

   @brief
   This file contains the Implementation for the CPI transport event handler.

   Revision History:

   6/15/2008 - John Miller
   Initial version.

   2/18/2009
   Added methods to allow another thread to wake up the handler.
   

   ************************************************************************** */

#include "DtIntEventHandler.h"
#include "CpiUtilAutoMutex.h"
#include "CpiOsAssert.h"
#include "DtExceptions.h"

DataTransfer::EventHandlerFactory* DataTransfer::EventManager::m_eventHandlerFactory;

DataTransfer::EventManager::EventManager( int lr, int hr )
  :m_spin(false)
{
  if ( ! m_eventHandlerFactory ) {
          throw CPI::Util::EmbeddedException( EM_NOT_SUPPORTED_FOR_EP, "Use Polling mode ONLY for selected endpoint");
  }
  setEventRange(lr,hr);
  m_eventHandler = m_eventHandlerFactory->createEventHandler();
  m_eventHandler->init(m_low, m_high);
}

DataTransfer::EventManager::~EventManager()
{
#ifndef NDEBUG
  printf(" IN DataTransfer::EventManager::~EventManager(), eh = %p\n", m_eventHandler);
#endif

  delete m_eventHandler;
}


// This is the CPI specialized port class definition
void DataTransfer::EventManager::registerEventHandlerFactory( 
                                                        DataTransfer::EventHandlerFactory* factory )
{
  m_eventHandlerFactory = factory;
};

void  DataTransfer::EventManager::setMinTimeout( CPI::OS::uint32_t id, CPI::OS::uint32_t uSec )
{
  if ( m_eventHandler ) {
    m_eventHandler->setMinTimeout( id, uSec );
  }
}

void  DataTransfer::EventManager::removeMinTimeout(  CPI::OS::uint32_t id )
{
  if ( m_eventHandler ) {
    m_eventHandler->removeMinTimeout( id  );
  }
}

void DataTransfer::EventManager::setEventRange( int low_range, int high_range  )
{
  m_low = low_range;
  m_high = high_range;
}

void DataTransfer::EventManager::getEventRange( int& low_range, int& high_range  )
{
  low_range = m_low;
  high_range = m_high;
}

void  DataTransfer::EventManager::spin( DataTransfer::EndPoint* ep, bool s )
{
  m_spin = s;
  if ( m_eventHandler && m_spin) {
    m_eventHandler->sendEvent( ep, m_low );
  }
}

void DataTransfer::EventManager::wakeUp( DataTransfer::EndPoint* ep )
{
  if ( m_eventHandler ) {
    m_eventHandler->sendEvent( ep, m_low );
  }  
}


void DataTransfer::EventManager::unregisterEventHandlerFactory( EventHandlerFactory* factory )
{
  m_eventHandlerFactory = NULL;

};

void DataTransfer::EventManager::consumeEvents()
{
  m_events.erase(m_events.begin(), m_events.end());
  cpiAssert( m_eventHandler );
  m_eventHandler->consumeEvents();
}

// Get the next available event
bool DataTransfer::EventManager::getNextEvent( int& id, CPI::OS::uint64_t &value )
{
  cpiAssert( m_eventHandler );
  if ( m_eventHandler->getPendingEvent(id,value) == DataTransfer::EventSuccess ) {
#ifndef NDEBUG
    printf("Next ID = %d\n", id );
#endif
    return true;
  }
  return false;
}

        
bool
DataTransfer::EventManager::oneOfOurs(int id)
{
  if ( (id >= m_low) && (id <= m_high ) ) {
    return true;
  }
  return false;
}


DataTransfer::ReturnStatus 
DataTransfer::EventManager::waitForEvent( int timeout_us )
{
  int id;
  CPI::OS::uint64_t value;
  return waitForEvent( timeout_us, id, value );
}


DataTransfer::ReturnStatus 
DataTransfer::EventManager::waitForEvent( int timeout_us, int& id, CPI::OS::uint64_t &value )
{
  cpiAssert( m_eventHandler );  

  if ( m_spin ) {
    id = -1;
    value = 0;
    return EventSpin;
  }

  //  CPI::Util::AutoMutex guard ( m_mutex, true ); 

        
  // NOTE:  This implementation can only handle a single event handler. 
  // Multi-threading is needed for more than 1
#ifdef USE_CACHE
  std::list<Events>::iterator it;
  for ( it = m_events.begin(); it != m_events.end(); it++ ) {
    if ( one_of_ours( (*it).id ) ) {
      id = (*it).id;
      value = (*it).value;
      m_events.erase(it);
      printf("Returning a SAVED event\n");
      return EventSuccess;
    }
  }
        
  while( m_eventHandler->get_pending_event(id,value) == DataTransfer::EventSuccess ) {
    if ( one_of_ours( id ) ) {
      printf("Returning a PENDING event(%d)\n",id);
      return EventSuccess;
    }
    else {
      m_events.push_back( Events(id,value) );
    }
  }
#endif

  if ( m_events.size() == 0 ) {
#ifndef NDEBUG
           printf("Waiting for an event !!!\n");
#endif
    if( m_eventHandler->waitForEvent( timeout_us,id,value) == EventSuccess ) {
      if ( ! oneOfOurs( id ) ) {
                                
#ifdef USE_CACHE                                
        printf("Caching an Event(%d) !!\n", id );
        m_events.push_back( Events(id,value) );
#endif
                                
        return EventContinue;
      }
    }
  }
        
#ifndef NDEBUG
  printf("Returning one of our events\n");
#endif
  return EventSuccess;
        
};

        

