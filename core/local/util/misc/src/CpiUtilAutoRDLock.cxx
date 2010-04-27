#include <CpiUtilAutoRDLock.h>
#include <CpiOsAssert.h>
#include <CpiOsRWLock.h>

CPI::Util::AutoRDLock::AutoRDLock (CPI::OS::RWLock & rwlock, bool locked)
  throw (std::string)
  : m_locked (locked),
    m_rwlock (rwlock)
{
  if (locked) {
    m_rwlock.rdLock ();
  }
}

CPI::Util::AutoRDLock::~AutoRDLock ()
  throw ()
{
  if (m_locked) {
    m_rwlock.rdUnlock ();
  }
}

void
CPI::Util::AutoRDLock::lock ()
  throw (std::string)
{
  cpiAssert (!m_locked);
  m_rwlock.rdLock ();
  m_locked = true;
}

bool
CPI::Util::AutoRDLock::trylock ()
  throw (std::string)
{
  cpiAssert (!m_locked);
  return (m_locked = m_rwlock.rdTrylock ());
}

void
CPI::Util::AutoRDLock::unlock ()
  throw (std::string)
{
  cpiAssert (m_locked);
  m_rwlock.rdUnlock ();
  m_locked = false;
}
