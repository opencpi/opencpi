#include "OcpiOsAssert.h"
#include "OcpiOsTimer.h"
#include "OcpiContainerRunConditionApi.h"
namespace OCPI { namespace API {

RunCondition::
RunCondition()
  : m_portMasks(m_myMasks), m_timeout(false), m_usecs(0), m_allocated(NULL), m_allMasks(0) {
  m_myMasks[0] = OCPI_ALL_PORTS; // all connected ports must be ready
  m_myMasks[1] = 0;
  m_allMasks = m_myMasks[0];
}
RunCondition::
RunCondition(OcpiPortMask pm, ...) :
  m_timeout(false), m_usecs(0), m_allocated(NULL), m_allMasks(0) {
  va_list ap;
  va_start(ap, pm);
  initMasks(ap);
  va_end(ap);
  va_start(ap, pm);
  setMasks(pm, ap);
  va_end(ap);
}
RunCondition::
RunCondition(OcpiPortMask *rpm, uint32_t usecs, bool timeout)
  : m_portMasks(NULL), m_timeout(timeout), m_usecs(usecs), m_allocated(NULL), m_allMasks(0) {
  setPortMasks(rpm);
}
RunCondition::
~RunCondition() {
  delete [] m_allocated;
}
void RunCondition::
initMasks(va_list ap) {
  unsigned n;
  OcpiPortMask m;
  for (n = 2; (m = va_arg(ap, OcpiPortMask)); n++)
    ;
  if (n >= sizeof(m_myMasks)/sizeof(OcpiPortMask))
    m_portMasks = m_allocated = new OcpiPortMask[n];
  else
    m_portMasks = m_myMasks;
}
void RunCondition::
setMasks(OcpiPortMask first, va_list ap) {
  OcpiPortMask
    *pms = m_portMasks,
    m = first;
  do {
    *pms++ = m;
    m_allMasks |= m;
  } while ((m = va_arg(ap, OcpiPortMask)));
  *pms++ = 0;
}
void RunCondition::
setPortMasks(OcpiPortMask pm, ...) {
  delete [] m_allocated;
  m_allocated = NULL;
  m_allMasks = 0;
  va_list ap;
  va_start(ap, pm);
  initMasks(ap);
  va_end(ap);
  va_start(ap, pm);
  setMasks(pm, ap);
  va_end(ap);
}
void RunCondition::
setPortMasks(OcpiPortMask *rpm) {
  delete [] m_allocated;
  m_allocated = NULL;
  m_allMasks = 0;
  m_portMasks = NULL;
  if (rpm) {
    unsigned n;
    for (n = 0; rpm[n]; n++)
      ;
    if (n >= sizeof(m_myMasks)/sizeof(OcpiPortMask))
      m_portMasks = m_allocated = new OcpiPortMask[n + 1];
    else
      m_portMasks = m_myMasks;
    OcpiPortMask m;
    OcpiPortMask *pms = m_portMasks;
    do {
      *pms++ = m = *rpm++;
      m_allMasks |= m;
    } while (m);
  }
}
void RunCondition::
activate(OCPI::OS::Timer &tmr, unsigned nPorts) {
  if (m_timeout)
    tmr.reset(m_usecs / 1000000, (m_usecs % 1000000) * 1000);
  // fix up default run condition when there are no ports at all
  if (!nPorts && m_portMasks && m_portMasks[0] == OCPI_ALL_PORTS) {
    if (m_timeout)
      m_portMasks[0] = OCPI_NO_PORTS;
    else
      m_portMasks = NULL;
  }
  m_hasRun = false;
}
bool RunCondition::
shouldRun(OCPI::OS::Timer &timer, bool &timedOut, bool &bail) {
  if (!m_portMasks) // no port mask array means run all the time
    return true;
  if (m_timeout && timer.expired()) {
    ocpiInfo("WORKER TIMED OUT, elapsed time = %u,%u", 
	     timer.getElapsed().seconds(), timer.getElapsed().nanoseconds());
    timedOut = true;
    return true;
  }
  // If no port masks, then we don't run except for timeouts, checked above
  if (!m_portMasks[0]) {
    if (m_timeout && !m_hasRun) {
      m_hasRun = true;
      return true; // run if we're in period execution and haven't run at all yet
    } else
      bail = true;
  }
  return false;
}
}} // OCPI::API
