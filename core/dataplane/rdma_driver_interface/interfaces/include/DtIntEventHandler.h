
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



/**
   @file

   @brief
   This file contains the Interface for the OCPI event handler.

   Revision History:

   6/14/2008 - John Miller
   Initial version.

   2/18/2009 - John Miller
   Removed exception monitor class.

************************************************************************** */

#ifndef OCPI_DataTransport_Event_Manager_H_
#define OCPI_DataTransport_Event_Manager_H_

#include <stdlib.h>
#include <string>
#include <list>
#include <OcpiOsDataTypes.h>
#include <OcpiOsMutex.h>

namespace DataTransfer {
  struct EndPoint;
  enum ReturnStatus {
    EventTimeout,
    EventFailure,
    EventSuccess,
    EventContinue,
    EventSpin
  };
  
  class EventHandler
  {
  public:
    virtual ReturnStatus waitForEvent( int timeout_us, int &id, OCPI::OS::uint64_t &value )=0;
    virtual ReturnStatus getPendingEvent(int &id, OCPI::OS::uint64_t &value )=0;

    // The follwing methods are used by the container infrastructure to set a timeout value
    // for the event manager to block.  These values are used to allow workers that use
    // a timeout in there run conditions to be scheduled when there is no other event
    // activity.
    virtual void setMinTimeout( OCPI::OS::uint32_t id, OCPI::OS::uint32_t uSec )=0;
    virtual void removeMinTimeout(  OCPI::OS::uint32_t id )=0;

    virtual void consumeEvents()=0;
    virtual void sendEvent( EndPoint* ep, int id )=0;
    virtual ReturnStatus init(int lr, int hr)=0;
    virtual ReturnStatus fini()=0;
    virtual ~EventHandler(){};
  };

  class EventHandlerFactory
  {
  public:
    virtual EventHandler* createEventHandler()=0;
    virtual ~EventHandlerFactory(){};
  };


  // This is the OCPI specialized port class definition
  class EventManager
  {
  public:
        
    // Constructor
    EventManager( 
                 int low_range,  // Event id low range
                 int high_range  // Event id high range
                 );
    ~EventManager();

    // Register an event handler
    static void registerEventHandlerFactory( EventHandlerFactory* eh );

    // Unregister an event handler
    static void unregisterEventHandlerFactory( EventHandlerFactory* eh );
        
    // Allow an application to wait for an event.  Calling this method does remove the
    // returned event from the Q.
    ReturnStatus waitForEvent( int timeout_us );
    ReturnStatus waitForEvent( int timeout_us, int& id, OCPI::OS::uint64_t &value );

    // The follwing methods are used by the container infrastructure to set a timeout value
    // for the event manager to block.  These values are used to allow workers that use
    // a timeout in there run conditions to be scheduled when there is no other event
    // activity.
    void setMinTimeout( OCPI::OS::uint32_t id, OCPI::OS::uint32_t uSec );
    void removeMinTimeout(  OCPI::OS::uint32_t id );

    // Get the next available event
    bool getNextEvent( int& id, OCPI::OS::uint64_t &value );

    // Consume all existing events
    void consumeEvents();
                
    // get the range values that are vectored to this handler
    void getEventRange( int& low_range, int& high_range  );

    // Allows a container to tell this event manager not to block
    void spin( DataTransfer::EndPoint* ep, bool s );

    // Forces this EM to wake up 
    void wakeUp( DataTransfer::EndPoint* ep );
                        
  protected:
                
    void setEventRange( int low_range, int high_range  );        
    static EventHandlerFactory* m_eventHandlerFactory;
    EventHandler* m_eventHandler;
    bool m_spin;
    int m_low;
    int m_high;
    OCPI::OS::Mutex m_mutex;
    struct Events {
      int id;
      OCPI::OS::uint64_t value;
      Events(int& pid, OCPI::OS::uint64_t &pv)
        :id(pid),value(pv){};
    };
    std::list<Events> m_events;
                                                
  private:
    bool oneOfOurs(int id);
  };
        
         
}


#endif
