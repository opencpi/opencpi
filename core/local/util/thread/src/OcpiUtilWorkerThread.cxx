
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

#include <string>
#include <OcpiOsAssert.h>
#include <OcpiOsMutex.h>
#include <OcpiOsSemaphore.h>
#include <OcpiOsThreadManager.h>
#include "OcpiUtilWorkerThread.h"

OCPI::Util::WorkerThread::ThreadData::
ThreadData ()
  throw (std::string)
  : terminate (false),
    jobPosted (0),
    jobComplete (0)
{
}

OCPI::Util::WorkerThread::
WorkerThread (bool synchronous)
  throw (std::string)
  : m_synchronous (synchronous)
{
  if (!m_synchronous) {
    m_threadManager.start (worker, &m_threadData);
  }
}

OCPI::Util::WorkerThread::
~WorkerThread ()
  throw ()
{
  if (!m_synchronous) {
    try {
      m_mutex.lock ();
      m_threadData.terminate = true;
      m_threadData.jobPosted.post ();
      m_threadManager.join ();
      m_mutex.unlock ();
    }
    catch (const std::string &) {
      ocpiAssert (0);
    }
  }
}

void
OCPI::Util::WorkerThread::
start (void (*job) (void *), void * opaque)
  throw (std::string)
{
  m_mutex.lock ();

  if (!m_synchronous) {
    m_threadData.job = job;
    m_threadData.opaque = opaque;

    try {
      m_threadData.jobPosted.post ();
    }
    catch (...) {
      m_mutex.unlock ();
      throw;
    }
  }
  else {
    (*job) (opaque);
  }
}

void
OCPI::Util::WorkerThread::
wait ()
  throw (std::string)
{
  if (!m_synchronous) {
    m_threadData.jobComplete.wait ();
  }

  m_mutex.unlock ();
}

void
OCPI::Util::WorkerThread::
worker (void * opaque)
{
  ThreadData & data = *reinterpret_cast<ThreadData *> (opaque);

  data.jobPosted.wait ();

  while (!data.terminate) {
    (*data.job) (data.opaque);
    data.jobComplete.post ();
    data.jobPosted.wait ();
  }
}
