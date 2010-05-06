/**
   @file

   @brief
   PPP implementation for data transfer event handler class.

   Revision History:

   6/19/09 - John Miller
   Added timer to support worker timeout run conditions.
   Added modify capability.

   2/20/09 - John Miller
   Added method for consuming events.

   10/13/08 - John Miller
   Initial version.

   ************************************************************************** */

#include "DtPPPEventHandler.h"
#include "rose/rose_iovec.h"
#include <CpiPPPSMemServices.h>
#include <CpiThread.h>
#include <CpiOsMisc.h>

#define PPP_EVENT_Q_SIZE 4096
#define ROSE_TEST_CH_MBOX_RANGE_START 500
#define ROSE_TEST_CH_MBOX_RANGE_END   510
#define ROSE_TEST_CH_MBOX_DMA_ERROR   520
#define ROSE_TEST_CH_MBOX_DMA_COMPLETE   525

extern "C" {
  int send_mbox(int tid, int id, int value, int count)
  {
    RoseMemoryInfo memInfo;
    RoseMailboxCookie data;
    int ret=ROSE_SUCCESS;
    int hc=1;

    memInfo.access.bridgeType = ROSE_BUS_LOCAL;
    memInfo.access.bridge.local.space = ROSE_LOCAL_RIO;
    memInfo.access.bridge.local.sp.rio.localport = 0;
    memInfo.access.bridge.local.sp.rio.destid = tid;
    memInfo.access.bridge.local.sp.rio.hopcount = hc;
    memInfo.access.bridge.local.sp.rio.qualifier = 0;        
    rose_mailbox_encode_cookie(id, value, value, &data);
    for ( int n=0; n<count; n++) {
      ret = rose_send_mbox(&memInfo, &data);
      if ( ret != ROSE_SUCCESS ) {
        break;
      }
    }
    return ret;
  }

};


DataTransfer::EventHandler* DataTransfer::PPPEventHandlerFactory::createEventHandler()
{
  DataTransfer::PPPEventHandler* eh;
  try {
    eh = new DataTransfer::PPPEventHandler();
  }
  catch ( std::bad_alloc ) {
    eh = NULL;
  }
  return eh;
}


DataTransfer::PPPEventHandler::PPPEventHandler()
  :m_pending(0)
{
}

namespace DataTransfer {

    class PPPEHWakeupThread : public CPI::Util::Thread
    {
    public:
      int  m_sleepTime;
      bool m_run;
      int &m_tid;
      int m_mbId;
      PPPEHWakeupThread( int& tid, int mbid, int uSec )
        :m_run(true),m_tid(tid),m_mbId(mbid){setTime(uSec);}
      void setTime( int uSec )
      {
        // We will set a max timeout period
        if ( uSec > 5000 ) {
          m_sleepTime = 5000 / 1000;
        }
        else {
          m_sleepTime=(uSec / 1000);
        }
      };

      void run() {
        while(m_run) {

          // For our current implementation we have mSec resolution for timeouts
          CPI::OS::sleep( m_sleepTime );
          if ( m_tid != -1 ) {
            send_mbox( m_tid, m_mbId, 0, 1);          
          }
        }
      }
    };
  }



DataTransfer::ReturnStatus 
DataTransfer::PPPEventHandler::init( int lr, int hr)
{
  int        rc;
  RoseUint32Type port;
  int device_id = 1;
  int i;

  // Create our wakeup thread
  m_tid = -1;
  try {
    m_WakeUpThread = new PPPEHWakeupThread( m_tid, lr, 1000 );
  }
  catch( std::bad_alloc ) {
    return DataTransfer::EventFailure;
  }

  memset(&m_interrupt, 0x0, sizeof(RoseInterruptInfo));
  m_pending = 0;
        
  /* 
  ** assign fabric bridge.
  */
  port = 0;
  m_interrupt.access.bridgeType = ROSE_BUS_LOCAL;
  m_interrupt.access.bridge.local.space = ROSE_LOCAL_RIO;
  m_interrupt.access.bridge.local.sp.rio.localport = port; /* Port */
  m_interrupt.access.bridge.local.sp.rio.hopcount = 1;
  m_interrupt.access.bridge.local.sp.rio.destid = device_id; /* Target ID */

  /* 
  ** 1024 Pending messages
  */
#ifndef _WRS_KERNEL
  m_interrupt.info.interruptType = (RoseInterruptType)(ROSE_INTERRUPT_MAILBOX | ROSE_INTERRUPT_DMA_ERROR | ROSE_INTERRUPT_DMA_COMPLETE);
#else
  m_interrupt.info.interruptType = (RoseInterruptType)(ROSE_INTERRUPT_MAILBOX);
#endif

  m_interrupt.info.interrupt.mailbox.mailboxIndex = 0;
  m_interrupt.info.interrupt.mailbox.deferredMailboxSize = 
    PPP_EVENT_Q_SIZE * sizeof(RoseMailboxCookie);
  m_interrupt.info.interrupt.mailbox.numRanges = 1;
  m_interrupt.info.interrupt.mailbox.channelRanges[0].min = lr;
  m_interrupt.info.interrupt.mailbox.channelRanges[0].max = hr-10;
  m_ourMbId = lr;        
  for (i = 0; i < 4; i++) {
    m_interrupt.info.interrupt.dmaError.channelInfo[i].options = ROSE_DMA_ERROR_FAILED_ID;
    m_interrupt.info.interrupt.dmaError.channelInfo[i].dmaChannel = i;
    m_interrupt.info.interrupt.dmaError.channelInfo[i].mailboxChannel = i + hr;
  }
  m_interrupt.info.interrupt.dmaError.numDmaChannels = 4;

  for (i = 0; i < 4; i++) {
    m_interrupt.info.interrupt.dmaComplete.channelInfo[i].options = 0;
    m_interrupt.info.interrupt.dmaComplete.channelInfo[i].dmaChannel = i;
    m_interrupt.info.interrupt.dmaComplete.channelInfo[i].mailboxChannel = i + (hr+4);
  }
  m_interrupt.info.interrupt.dmaComplete.numDmaChannels = 4;
  rc = rose_attach_interrupt(&m_interrupt, &m_handle);
  if ( rc != ROSE_SUCCESS ) {
#ifndef NDEBUG
    printf("Got error %d from rose_attach_interrupt() \n", rc);
#endif
    delete m_WakeUpThread;
    ( void )rose_detach_interrupt(&m_handle);
    return DataTransfer::EventFailure;
  }

  /* 
  ** get deferred buffer and setup ring buffer.
  */
  rc = rose_get_deferred_buffer(&m_handle, &m_pBuffer, 
                                   PPP_EVENT_Q_SIZE/* *sizeof(RoseMailboxCookie) */);
  if ( rc != ROSE_SUCCESS ) {
#ifndef NDEBUG
    printf("Got error %d from rose_get_deferred_buffer()  \n", rc );
#endif
    delete m_WakeUpThread;
    ( void )rose_detach_interrupt(&m_handle);
    return DataTransfer::EventFailure;
  }
  ( void ) rose_interrupt_check ( &m_handle );
        
  m_ringStart = (RoseMailboxCookie *)m_pBuffer;
  m_ringEnd = m_ringStart + PPP_EVENT_Q_SIZE; //RoseMailboxCookie;
  m_ringCurrent = m_ringStart;

  m_WakeUpThread->start();        
  return DataTransfer::EventSuccess;

}


DataTransfer::ReturnStatus 
DataTransfer::PPPEventHandler::fini()
{
  return DataTransfer::EventSuccess;
}

DataTransfer::PPPEventHandler::~PPPEventHandler()
{
  try {
    m_WakeUpThread->m_run = 0;
    m_WakeUpThread->join();
    delete m_WakeUpThread;
    ( void ) rose_detach_interrupt(&m_handle);
  }
  catch( ... ){};
}


#ifndef NDEBUG
// Used for debug
static bool event_print=false;
#endif


void DataTransfer::PPPEventHandler::consumeEvents()
{
  m_ringCurrent += m_pending;
  if(m_ringCurrent >= m_ringEnd ) {
    m_ringCurrent = m_ringStart + (m_ringCurrent-m_ringEnd);
  }
  m_pending=0;
}

DataTransfer::ReturnStatus 
DataTransfer::PPPEventHandler::getPendingEvent(int &id, CPI::OS::uint64_t &value )
{

#ifndef NDEBUG
  if (event_print )
    printf("In PPPEventHandler::get_pending_event()\n");
#endif

  RoseMailboxCookie *cookie;
  RoseUint32Type        channel;
  RoseUint32Type d22;
  RoseUint32Type d32;
  if ( m_pending == 0 ) {
    return EventFailure;
  }
  m_pending--;
  cookie = m_ringCurrent;
  m_ringCurrent++;
  if(m_ringCurrent == m_ringEnd ) {
#ifndef NDEBUG
    printf("Wrapping the ring buffer\n");
#endif
    m_ringCurrent = m_ringStart;
  }
  rose_mailbox_decode_cookie(cookie, &channel, &d22, &d32);

#ifndef NDEBUG
  if (event_print )
    printf("The mailbox interrupt was for channel %d, pending = %d\n", channel, m_pending);
#endif
  id = channel;

#ifndef NDEBUG
  if ( (channel&0x0f) == 5 ) {
    event_print = true;
  }
  else if ( (channel&0x0f) == 4 ) {
    event_print = false;
  }
  else if ( channel == 103 ) {
    // Reserved for worker threads
  }
  else if ( (channel >= ROSE_TEST_CH_MBOX_DMA_COMPLETE) &&
            (channel <= ROSE_TEST_CH_MBOX_DMA_COMPLETE+4 ) ) {
    printf("Got a DMA chain complete interrupt\n");
    channel = 103;
  }
  else {
    printf("got an event from channel = %d\n", channel );
  }
#endif        
        
  value = ((CPI::OS::uint64_t)d22 << 32) | d32;

#ifndef NDEBUG        
  if (event_print )
    printf("PPPEventHandler::get_pending_event(%d) return value = %lld\n", id, value);
#endif

  return DataTransfer::EventSuccess;
}
        


DataTransfer::ReturnStatus 
DataTransfer::PPPEventHandler::waitForEvent( int timeout_us, int &id, CPI::OS::uint64_t &value )
{
  RoseMailboxCookie *cookie;
  RoseUint32Type        channel;
  RoseUint32Type d22;
  RoseUint32Type d32;
  int        rc;

#ifndef NDEBUG        
  if (event_print ) {
    printf("PPPwfe in wait_for_event()\n");
    printf("PPPwfe nPending = %d\n", m_pending );
  }
#endif

  if ( m_pending == 0 ) {

#ifndef NDEBUG                                
    if ( event_print ) {
      printf("PPPwfe About to block until we get a mailbox interrupt\n");
      printf("m_pending = %d, before block\n", m_pending );
    }
#endif

    rc = rose_interrupt_block(&m_handle);
    m_pending = rose_mailbox_pending(&m_handle);

#ifndef NDEBUG
    if ( event_print )  {
      printf("PPPwfe Got a mailbox interrupt, error = %d\n", rc);
      printf("m_pending = %d, after block\n", m_pending );
    }
#endif

    if ( (rc != ROSE_SUCCESS) || !m_pending ) {
      return DataTransfer::EventFailure;
    }
  }
                
  cookie = m_ringCurrent;
                
  /*
  ** process cookie
  */
  rose_mailbox_decode_cookie(cookie, &channel, &d22, &d32);

#ifndef NDEBUG
  if ( (channel&0x0f) == 5 ) {
    event_print = true;
  }
  else if ( (channel&0x0f) == 4 ) {
    event_print = false;
  }
  else if ( (channel >= ROSE_TEST_CH_MBOX_DMA_COMPLETE) &&
            (channel <= ROSE_TEST_CH_MBOX_DMA_COMPLETE+4 ) ) {
    printf("Got a DMA chain complete interrupt\n");
    channel = 103;                                        
  }
#endif

#ifndef NDEBUG                        
  if ( event_print ) 
    printf("PPPwfe The mailbox interrupt was for channel %d\n", channel);
#endif
                        
  id = channel;
  value = ((CPI::OS::uint64_t)d22 << 32) | d32;
        
  return DataTransfer::EventSuccess;
        
}


void DataTransfer::PPPEventHandler::sendEvent( EndPoint* ep, int eid )
{  
  DataTransfer::PPPEndPoint pppep(ep->end_point);
  ( void )send_mbox( pppep.target_id, eid, 0, 1);
  m_tid = pppep.target_id;
}


void DataTransfer::PPPEventHandler::setMinTimeout( CPI::OS::uint32_t id, CPI::OS::uint32_t uSec )
{
  
  if ( m_minTime > uSec ) {
    m_minTime = uSec;
  }
  CPI::OS::uint64_t m = (CPI::OS::uint64_t)(((CPI::OS::uint64_t)id<<32) | uSec);
  m_minTimeList.push_back( m );
  m_WakeUpThread->setTime( m_minTime );
  
}


void DataTransfer::PPPEventHandler::removeMinTimeout(  CPI::OS::uint32_t id )
{
  std::list<CPI::OS::uint64_t>::iterator it;
  for ( it=m_minTimeList.begin(); it != m_minTimeList.end(); it++ ) {
    CPI::OS::uint64_t tid = (*it) >> 32;
    if ( tid == id ) {
      m_minTimeList.erase( it );
      break;
    }
  }
}
