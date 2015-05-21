
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

#ifndef OCPI_THREAD_H_
#define OCPI_THREAD_H_

#include <stdlib.h>
#include <stdio.h>
#include <OcpiOsThreadManager.h>

namespace OCPI {
  namespace Util {

    /**
     * \brief Thread class for asynchronous tasks.
     */
    class Thread {

    public:
    Thread() 
      :m_joined(true) 
        {
          m_pobjThreadServices = new OCPI::OS::ThreadManager;
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
      OCPI::OS::ThreadManager         *m_pobjThreadServices;
      bool m_joined;
    };

    /**
     * Helper function for the OCPI::Util::Thread class.
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
