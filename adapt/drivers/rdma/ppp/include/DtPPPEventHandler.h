
/**
   @file

   @brief
   PPP implementation for data transfer event handler class.

   Revision History:

   5/20/2009 - John Miller
   Reorganized include files to compile in linux

   2/20/2009 - John Miller
   Added method for consuming events.

   10/13/2008 - John Miller
   Initial version.

   ************************************************************************** */

#ifndef DataTransfer_PPP_EventHandler_H_
#define DataTransfer_PPP_EventHandler_H_

#include "DtIntEventHandler.h"
#include "rose/rose_defs.h"
#include "rose/rose_user_if.h"

namespace DataTransfer {

  class PPPEHWakeupThread;

  class PPPEventHandlerFactory : public EventHandlerFactory
  {
    EventHandler* createEventHandler();
    virtual ~PPPEventHandlerFactory(){};
  };

	
  class PPPEventHandler : public EventHandler
  {
  public:
    PPPEventHandler();
    virtual ~PPPEventHandler();
    ReturnStatus waitForEvent( int timeout_us, int &id, CPI::OS::uint64_t &value );
    void consumeEvents();
    void sendEvent( EndPoint* ep, int id );
    ReturnStatus getPendingEvent(int &id, CPI::OS::uint64_t &value );

    // The follwing methods are used by the container infrastructure to set a timeout value
    // for the event manager to block.  These values are used to allow workers that use
    // a timeout in there run conditions to be scheduled when there is no other event
    // activity.
    void setMinTimeout( CPI::OS::uint32_t id, CPI::OS::uint32_t uSec );
    void removeMinTimeout(  CPI::OS::uint32_t id );

    ReturnStatus init( int lr, int hr);
    ReturnStatus fini();

    RoseInterruptInfo        m_interrupt;
    RoseUserInterruptHandle  m_handle;
    RoseVirtAddrType         m_pBuffer;
    RoseMailboxCookie       *m_ringStart;
    RoseMailboxCookie       *m_ringEnd;
    RoseMailboxCookie       *m_ringCurrent;
    int                      m_pending;
    int                      m_ourMbId;  // Our mailbox id
    CPI::OS::uint32_t        m_minTime;
    std::list<CPI::OS::uint64_t> m_minTimeList;
    PPPEHWakeupThread       *m_WakeUpThread;
    int                      m_tid;
			
  };

}

#endif



