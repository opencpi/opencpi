// -*- c++ -*-

#ifndef CPIUTILAUTORDLOCK_H__
#define CPIUTILAUTORDLOCK_H__

/**
 * \file
 * \brief Auto-release a CPI::OS::RWLock read lock.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <CpiOsRWLock.h>
#include <string>

namespace CPI {
  namespace Util {

    /**
     * \brief Auto-release a CPI::OS::RWLock read lock.
     *
     * Keeps track of the read-locked state of a CPI::OS::RWLock object.  If
     * the lock is held at the time of destruction, the read lock is released.
     * This helps releasing the lock in all code paths, e.g., when an
     * exception occurs.
     */

    class AutoRDLock {
    public:
      /**
       * Constructor.
       *
       * \param[in] rwlock The lock object to manage.
       * \param[in] locked If true, calls lock().
       *
       * \note Throughout the life time of this object, this thread
       * shall not manipulate \a mutex directly.
       */

      AutoRDLock (CPI::OS::RWLock & rwlock, bool locked = true)
	throw (std::string);

      /**
       * Destructor.
       *
       * Releases the read lock if it is currently held.
       */

      ~AutoRDLock ()
	throw ();

      void lock ()
	throw (std::string);

      bool trylock ()
	throw (std::string);

      void unlock ()
	throw (std::string);

    private:
      bool m_locked;
      CPI::OS::RWLock & m_rwlock;
    };

  }
}

#endif
