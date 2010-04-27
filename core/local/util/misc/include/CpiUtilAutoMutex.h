// -*- c++ -*-

#ifndef CPIUTILAUTOMUTEX_H__
#define CPIUTILAUTOMUTEX_H__

/**
 * \file
 * \brief Auto-release an CPI::OS::Mutex.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <CpiOsMutex.h>
#include <string>

namespace CPI {
  namespace Util {

    /**
     * \brief Auto-release an CPI::OS::Mutex.
     *
     * Keeps track of the locked state of a CPI::OS::Mutex object.  If
     * the lock is held at the time of destruction, the lock is released.
     * This helps releasing the locked mutex in all code paths, e.g.,
     * when an exception occurs.
     */

    class AutoMutex {
    public:
      /**
       * Constructor.
       *
       * \param[in] mutex The mutex object to manage.
       * \param[in] locked  If true, calls lock().
       *
       * \note Throughout the life time of this object, this thread
       * shall not manipulate \a mutex directly.
       */

      AutoMutex (CPI::OS::Mutex & mutex, bool locked = true)
	throw (std::string);

      /**
       * Destructor.
       *
       * Releases the mutex lock if it is currently held.
       */

      ~AutoMutex ()
	throw ();

      void lock ()
	throw (std::string);

      bool trylock ()
	throw (std::string);

      void unlock ()
	throw (std::string);

    private:
      bool m_locked;
      CPI::OS::Mutex & m_mutex;
    };

  }
}

#endif
