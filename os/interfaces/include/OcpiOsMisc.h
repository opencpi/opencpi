/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// -*- c++ -*-

#ifndef OCPIOSMISC_H__
#define OCPIOSMISC_H__

/**
 * \file
 *
 * \brief Miscellaneous functionality.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>

namespace OCPI {
  namespace OS {

    /**
     * Pause the current thread.
     *
     * \param[in] msecs The number of milliseconds to sleep. Will not return
     *             before this amount of time has elapsed. Depending on
     *             scheduling, the time slept may be larger than \a msecs.
     *             If \a msecs is zero, then the current thread yields the
     *             rest of its time slice.
     * \throw std::string Operating system error.
     */

    void sleep (unsigned long msecs)
      throw (std::string);

    /**
     * Returns an identifier for the current process.
     *
     * \return   An unsigned long integer identifying the current process.
     *           The number is the same across all threads in the current
     *           process, and is unique among multiple concurrent
     *           processes. Process identifiers may be reused when a
     *           process exits and new processes are started.
     */

    unsigned long getProcessId ()
      throw ();

    /**
     * Returns an identifier for the current thread.
     *
     * \return   An unsigned long integer identifying the current thread.
     *           The number is unique among multiple concurrent
     *           threads in the same process. A thread identifier may be
     *           reused when a thread exits and new threads are started.
     */

    unsigned long getThreadId ()
      throw ();

    /**
     * Returns the unqualified host name.
     *
     * \return   The name that is considered the local host name.
     *           This host name is unique within the local network.
     * \throw std::string Operating system error.
     */

    std::string getHostname ()
      throw (std::string);

    /**
     * Returns the fully qualified host name.
     *
     * \return   The full name of the local host, including all
     *           domain qualifiers.
     * \throw std::string Operating system error.
     */

    std::string getFQDN ()
      throw (std::string);

    /**
     * Returns the local host's IP address.
     *
     * \return The IP address in dotted-decimal notation.
     * \throw std::string Operating system error.
     */

    std::string getIPAddress ()
      throw (std::string);

    /**
     * Determines if a name is an alias for the local host.
     *
     * \param[in] name A name to test.
     * \return    true if \a name is an alias of the local host name.
     *            This includes "localhost", all unqualified and fully
     *            qualified names of the current host on all networks,
     *            and all local IP addresses in "dotted IP" notation.
     *            Returns false otherwise.
     * \throw std::string Operating system error.
     */

    bool isLocalhost (const std::string & name)
      throw (std::string);

    /**
     * Arranges for a function to be called when the current
     * process is externally signaled to terminate. This includes pressing
     * the interrupt key on the console (usually Ctrl-C), receiving the
     * SIGINT signal (Posix), or receiving a CTRL_BREAK_EVENT console
     * event (Win32).
     *
     * \param[in] handler An address of a callback function.
     *
     * \note The \a handler is called in interrupt context and may
     * only call async-safe functions.
     *
     * \note The handler is not called in case of "normal" program
     * termination such as returning from main(), calling exit() or
     * abort().
     */

    void setCtrlCHandler (void (*handler) (void))
      throw ();

    void setError(std::string &error, const char *fmt, ...)
      throw();

    void getExecFile(std::string &name);

  }
}

#endif
