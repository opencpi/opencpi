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
