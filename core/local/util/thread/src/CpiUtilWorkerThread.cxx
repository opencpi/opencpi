#include <string>
#include <CpiOsAssert.h>
#include <CpiOsMutex.h>
#include <CpiOsSemaphore.h>
#include <CpiOsThreadManager.h>
#include "CpiUtilWorkerThread.h"

CPI::Util::WorkerThread::ThreadData::
ThreadData ()
  throw (std::string)
  : terminate (false),
    jobPosted (0),
    jobComplete (0)
{
}

CPI::Util::WorkerThread::
WorkerThread (bool synchronous)
  throw (std::string)
  : m_synchronous (synchronous)
{
  if (!m_synchronous) {
    m_threadManager.start (worker, &m_threadData);
  }
}

CPI::Util::WorkerThread::
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
      cpiAssert (0);
    }
  }
}

void
CPI::Util::WorkerThread::
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
CPI::Util::WorkerThread::
wait ()
  throw (std::string)
{
  if (!m_synchronous) {
    m_threadData.jobComplete.wait ();
  }

  m_mutex.unlock ();
}

void
CPI::Util::WorkerThread::
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
