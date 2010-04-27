#include <CpiUtilAutoWRLock.h>
#include <CpiOsAssert.h>
#include <CpiOsRWLock.h>

CPI::Util::AutoWRLock::AutoWRLock (CPI::OS::RWLock & rwlock, bool locked)
  throw (std::string)
  : m_locked (locked),
    m_rwlock (rwlock)
{
  if (locked) {
    m_rwlock.wrLock ();
  }
}

CPI::Util::AutoWRLock::~AutoWRLock ()
  throw ()
{
  if (m_locked) {
    m_rwlock.wrUnlock ();
  }
}

void
CPI::Util::AutoWRLock::lock ()
  throw (std::string)
{
  cpiAssert (!m_locked);
  m_rwlock.wrLock ();
  m_locked = true;
}

bool
CPI::Util::AutoWRLock::trylock ()
  throw (std::string)
{
  cpiAssert (!m_locked);
  return (m_locked = m_rwlock.wrTrylock ());
}

void
CPI::Util::AutoWRLock::unlock ()
  throw (std::string)
{
  cpiAssert (m_locked);
  m_rwlock.wrUnlock ();
  m_locked = false;
}
