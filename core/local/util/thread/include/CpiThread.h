/**
 * \file
 * \brief Thread class for asynchronous tasks.
 *
 *   Revision history.
 *
 *   06/24/09 - John Miller
 *   Removed detach() call in destructor if join() was already called.
 *   
 *   01/01/05 - Initial Version
 */

#ifndef CPI_THREAD_H_
#define CPI_THREAD_H_

#include <stdlib.h>
#include <stdio.h>
#include <CpiOsThreadManager.h>

namespace CPI {
  namespace Util {

    /**
     * \brief Thread class for asynchronous tasks.
     */
    class Thread {

    public:
    Thread() 
      :m_joined(true) 
        {
          m_pobjThreadServices = new CPI::OS::ThreadManager;
        }
      virtual ~Thread()
        { 
          if ( ! m_joined ) {
            m_pobjThreadServices->detach(); 
          }
          delete m_pobjThreadServices; 
        }

      // User implementation method
      virtual void run()=0;

      // Thread control
      void start();
      void join();

    private:
      CPI::OS::ThreadManager         *m_pobjThreadServices;
      bool m_joined;
    };

    /**
     * Helper function for the CPI::Util::Thread class.
     */

    inline void thread_proc (void *obj)
    {
      // Argument is reference to "this". Invoke derived
      // class run method.
      Thread* thr = static_cast<Thread*>(obj);
      thr->run ();
    }


    // Thread control
    inline void Thread::start ()
    {
      // Create a new thread
      m_pobjThreadServices->start (thread_proc, (void *)this);
      m_joined = false;
    }

    inline void Thread::join ()
    {
      m_pobjThreadServices->join ();
      m_joined = true;
    }

  }
}

#endif 
