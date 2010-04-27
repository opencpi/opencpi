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

// -*- c++ -*-

#ifndef CPIOSPROCESSMANAGER_H__
#define CPIOSPROCESSMANAGER_H__

/**
 * \file
 *
 * \brief Start and manage processes.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Use 64-bit type for our opaque data, to ensure
 *                  alignment.
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiOsDataTypes.h>
#include <string>
#include <vector>

namespace CPI {
  namespace OS {

    /**
     * \brief Start and manage processes.
     *
     * \note In VxWorks DKM, executables are downloadable kernel modules
     * that export main(), which is run in a new kernel task.  No memory
     * separation exists, "processes" share global symbols with all other
     * instances.  The shutdown() operation requires that the process
     * installs an appropriate handler for the SIGINT signal (the default
     * signal handler does not terminate the task).  The kill() operation
     * deletes the process' main thread, but other threads that were
     * started by that process are not affected, may continue running,
     * and will likely crash as the module is unloaded from memory.
     * Task deletion does not release resources that were held by the
     * task.
     * \note In VxWorks DKM, system defaults are used for the new task's
     * priority, options and stack size.  These system defaults can be
     * configured using the kernel symbols <em>spTaskPriority</em>,
     * <em>spTaskOptions</em> and <em>spTaskStackSize</em>, respectively.
     * Some applications may find the default stack size of 20000 bytes
     * insufficient and may want to redefine the default.
     * \note In VxWorks DKM, the \a executable parameter to the #start()
     * can contain a colon character followed by the name of an entrypoint
     * to be used instead of "main".
     */

    class ProcessManager {
    public:
      typedef std::vector<std::string> ParameterList;

    public:

      /**
       * Constructor: Initializes the ProcessManager instance, but does
       *              not start a process.
       */

      ProcessManager ()
	throw ();

      /**
       * Constructor: Initializes the ProcessManager instance, and then
       *              calls #start (\a executable, \a parameters).
       * \throw std::string See start().
       */

      ProcessManager (const std::string & executable,
		      const ParameterList & parameters)
	throw (std::string);

      /**
       * Destructor.
       *
       * \pre The instance may only be destructed if it is not managing a
       * process. If a process was started, wait() or detach() must be
       * called first.
       */

      ~ProcessManager ()
	throw ();

      /**
       * Start a process and manage it.
       *
       * \param[in] executable The absolute path name of the program to start
       *                       in the new process, according to native path
       *                       name convention.
       * \param[in] parameters The command-line arguments, starting with the
       *                       first (excluding "argv[0]").
       *
       * \throw std::string If \a is not an executable or if starting the
       * new process fails.
       *
       * \pre May not be called if this instance already manages a process.
       * \post The new process is running and is being managed.
       */

      void start (const std::string & executable,
		  const ParameterList & parameters)
	throw (std::string);

      /**
       * Returns the started process' process identifier.
       *
       * \return The process identifier of the started process.
       * \throw std::string Operating system error.
       * \pre A process was started and is being managed.
       */

      unsigned long pid ()
	throw (std::string);

      /**
       * Returns the exit code of the process, if it has exited.
       *
       * \return The exit code.
       *
       * \pre wait() returned true.
       *
       * \throw std::string If the process did not terminate properly
       * but was killed (e.g., by a signal).
       *
       * \note Most operating systems only make the lower 8 bits of
       * the process' exit code available.
       */

      int getExitCode ()
	throw (std::string);

      /**
       * Waits for the process to exit.
       *
       * \param[in] timeout A timeout period to wait for the process to exit,
       *               in milliseconds. If 0, does not block. If -1, waits
       *               forever.
       * \return       true if the process exited before the timeout expired,
       *               false if the timeout expires, and the process is still
       *               running.
       *
       * \pre A process was started and is being managed.
       * \post If this operation returns true, management of the process
       * ceases, and its exit code is made available to getExitCode().
       */

      bool wait (unsigned long timeout = static_cast<unsigned long> (-1))
	throw (std::string);

      /**
       * Signals the managed process to exit.
       *
       * This sends a "friendly" request to the managed process that it
       * should terminate. The managed process may intercept the signal,
       * and perform a proper termination, but it may also ignore the
       * signal.
       *
       * \throw std::string Operating system error, including the race
       * condition where the process has already terminated of its own.
       *
       * \pre A process was started and is being managed.
       */

      void shutdown ()
	throw (std::string);

      /**
       * Requests immediate termination of the managed process.
       *
       * Requests the operating system to unconditionally terminate the
       * process, which may not be able to react to the signal, and may
       * thus not terminate cleanly.
       *
       * \throw std::string Operating system error, including the race
       * condition where the process has already terminated of its own.
       *
       * \pre A process was started and is being managed.
       */

      void kill ()
	throw (std::string);

      /**
       * "Forgets" about the started process, which will continue to
       * run.
       *
       * \pre A process was started and is being managed.
       * \post Management of the process ceases.
       */

      void detach ()
	throw ();

    private:
      CPI::OS::uint64_t m_osOpaque[4];

    private:
      /**
       * Not implemented.
       */

      ProcessManager (const ProcessManager &);

      /**
       * Not implemented.
       */

      ProcessManager & operator= (const ProcessManager &);
    };

  }
}

#endif
