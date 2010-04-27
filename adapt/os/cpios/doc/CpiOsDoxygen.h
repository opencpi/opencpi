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

#ifndef CPIOSDOXYGEN_H__
#define CPIOSDOXYGEN_H__

/**
 * \file
 * \brief Documentation only.
 *
 * This file does not contain any functional content, but only documentation
 * intended for consumption by doxygen.  It shall not be included by any user
 * code.
 *
 * Revision History:
 *
 *     02/19/2008 - Frank Pilhofer
 *                  Initial version.
 *
 *     05/30/2008 - Frank Pilhofer
 *                  Polished Linux implementation.
 *
 */

/**
 * \mainpage The CPI::OS Operating System Abstraction Library
 *
 * This library provides a number of "local" operating system abstractions
 * for thread and process management and intraprocess use.  Interprocess
 * communication is out of scope for this library.
 *
 * The header files provided by this library isolate from operating
 * system-specific header files (such as <windows.h>).  User code shall
 * only include portable header files, which includes the CPI OS header
 * files and the standard C and C++ library header files.
 *
 * This library contains no "smarts:" the classes do not try to
 * second-guess wrong behavior on part of the caller.  E.g., classes
 * do not track which operations are valid at any given point.
 * Violating documented semantics results at best in a run-time
 * exception and at worst in undefined behavior.
 *
 * \section sync Intra-Process Signaling and Synchronization
 *
 * CPI::OS::Mutex\n
 * CPI::OS::SpinLock\n
 * CPI::OS::RWLock\n
 * CPI::OS::Event\n
 * CPI::OS::Semaphore\n
 *
 * \section tpm Thread and Process Management
 *
 * CPI::OS::ThreadManager\n
 * CPI::OS::ProcessManager\n
 *
 * \section lm Loadable Modules
 *
 * CPI::OS::LoadableModule\n
 *
 * \section files File System
 *
 * CPI::OS::FileSystem\n
 * CPI::OS::FileIterator\n
 *
 * \section sock Sockets
 *
 * CPI::OS::Socket\n
 * CPI::OS::ClientSocket\n
 * CPI::OS::ServerSocket\n
 *
 * \section misc Miscellaneous
 *
 * CpiOsTimer.h &mdash; Measure wall clock time\n
 * CpiOsMisc.h &mdash; Various functionality\n
 * CpiOsDebug.h &mdash; Debugging\n
 * CpiOsAssert.h &mdash; Assertion\n
 * CpiOsDataTypes.h &mdash; Data types\n
 *
 * \section impl Implementation Issues
 *
 * \subsection impl-win Windows
 *
 * - Requires at least Visual Studio 7.1.  More precisely, at least the
 *   C++ compiler that comes with Visual Studio 7.1 is required.
 * - CPI::OS::FileIterator: Networked shares are not included when
 *   listing files in the root directories.  The root directory of a
 *   networked share can not be listed.  E.g., listing the directory
 *   "/ad-fpilhofe1" will fail even though the host may export shares.
 *
 * \subsection impl-linux Linux
 *
 * - CPI::OS::dumpStack(): Prints function names and offsets, but no
 *   source code locations.  An external tool is available to post-process
 *   the output from dumpStack() into source code locations (if the code
 *   was built with debugging symbols).  The "-rdynamic" command-line
 *   option must be used when linking applications.
 * - Applications must be linked against the pthread and rt libraries,
 *   i.e., "-lpthread -lrt" must be added to the linker command line.
 *
 * \subsection impl-vxdkm VxWorks DKM
 *
 * - CPI::OS::SpinLock: implementation is the same as Mutex for now.
 * - CPI::OS::LoadableModule: modules shall be built as DKMs (".out").
 *   Unresolved symbols are logged to the console.  Dependencies between
 *   loadable modules are not supported, so dependent modules must be
 *   explicitly loaded to avoid unresolved symbols.
 * - CPI::OS::ProcessManager: the same limitations as for loadable
 *   modules apply.  "Processes" are implemented by calling the module's
 *   main() function in a new task.  No memory separation exists,
 *   "processes" share global symbols with all other instances.
 *   The shutdown() operation requires that the process installs an
 *   appropriate handler for the SIGINT signal (the default signal
 *   handler does not terminate the task).  The kill() operation deletes
 *   the process' main thread, but other threads that were started by
 *   that process may continue running (and will likely crash as the
 *   module is unloaded from memory).  Task deletion does not release
 *   resources that were held by the task.
 * - CPI::OS::dumpStack() ignores its output stream parameter and always
 *   prints the backtrace (bottom up) to the console.  The stack trace
 *   uses symbols from the system symbol table, which must be populated.
 *   Since non-exported static or inline functions are not loaded into
 *   the system symbol table, they will be misreported.  The stack trace
 *   will instead show the name of the preceding non-static function.
 * - CPI::OS::debugBreak() suspends the current task (i.e., it calls
 *   taskSuspend(0)).  After attaching the debugger, taskResume() can
 *   be used from the console to resume the task.
 */

#endif
