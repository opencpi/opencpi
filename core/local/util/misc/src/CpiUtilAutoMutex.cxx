#include <CpiUtilAutoMutex.h>
#include <CpiOsAssert.h>
#include <CpiOsMutex.h>

CPI::Util::AutoMutex::AutoMutex (CPI::OS::Mutex & mutex, bool locked)
  throw (std::string)
  : m_locked (locked),
    m_mutex (mutex)
{
  if (locked) {
    m_mutex.lock ();
  }
}

CPI::Util::AutoMutex::~AutoMutex ()
  throw ()
{
  if (m_locked) {
    m_mutex.unlock ();
  }
}

void
CPI::Util::AutoMutex::lock ()
  throw (std::string)
{
  cpiAssert (!m_locked);
  m_mutex.lock ();
  m_locked = true;
}

bool
CPI::Util::AutoMutex::trylock ()
  throw (std::string)
{
  cpiAssert (!m_locked);
  return (m_locked = m_mutex.trylock ());
}

void
CPI::Util::AutoMutex::unlock ()
  throw (std::string)
{
  cpiAssert (m_locked);
  m_mutex.unlock ();
  m_locked = false;
}
