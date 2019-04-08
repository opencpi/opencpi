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

#include <cstring> // std::memcpy
#include "OcpiOsAssert.h"
#include "OcpiOsTimer.h"
#include "OcpiContainerRunConditionApi.h"
namespace OCPI { namespace API {

RunCondition::
RunCondition()
  : m_portMasks(m_myMasks), m_timeout(false), m_usecs(0), m_inUse(false), m_allocated(NULL),
    m_hasRun(false), m_allMasks(0) {
  m_myMasks[0] = OCPI_ALL_PORTS; // all connected ports must be ready
  m_myMasks[1] = 0;
  m_allMasks = m_myMasks[0];
}
RunCondition::
RunCondition(OcpiPortMask pm, ...) :
  m_timeout(false), m_usecs(0), m_inUse(false), m_allocated(NULL), m_hasRun(false), m_allMasks(0) {
  va_list ap;
  va_start(ap, pm);
  initMasks(pm, ap);
  va_end(ap);
  va_start(ap, pm);
  setMasks(pm, ap);
  va_end(ap);
}
RunCondition::
RunCondition(OcpiPortMask *rpm, uint32_t usecs, bool timeout)
  : m_portMasks(NULL), m_timeout(timeout), m_usecs(usecs), m_inUse(false), m_allocated(NULL),
    m_hasRun(false), m_allMasks(0) {
  setPortMasks(rpm);
}
RunCondition::
~RunCondition() {
  //? ocpiAssert(!m_inUse);
  delete [] m_allocated;
}
RunCondition &RunCondition::
operator=(const RunCondition& other) {
  ocpiAssert(!m_inUse);
  setPortMasks(other.m_portMasks); // This handles all memory copies, etc.
  m_timeout = other.m_timeout;
  m_usecs = other.m_usecs;
  m_hasRun = false; // Doesn't really matter since activate() resets
  return *this;
}
RunCondition &RunCondition::
operator=(RunCondition&& other) {
  ocpiAssert(!m_inUse);
  ocpiAssert(!other.m_inUse);
  ocpiAssert(this != &other);
  delete [] m_allocated;
  m_allocated = NULL;
  m_timeout = other.m_timeout;
  m_usecs = other.m_usecs;
  m_hasRun = other.m_hasRun;
  /* CentOS 6 won't do this: std::copy(std::begin(other.m_myMasks), std::end(other.m_myMasks), m_myMasks); */
  std::memcpy(m_myMasks, other.m_myMasks, sizeof(m_myMasks));
  if (other.m_allocated) { // Steal them
    m_portMasks = m_allocated = other.m_allocated;
    other.m_allocated = NULL;
  } else {
    m_portMasks = m_myMasks;
  }
  m_allMasks = other.m_allMasks;
  return *this;
}
void RunCondition::
initMasks(OcpiPortMask first, va_list ap) {
  ocpiAssert(!m_inUse);
  OcpiPortMask m = first;
  unsigned n;
  for (n = 2; m && (m = va_arg(ap, OcpiPortMask)); n++)
    ;
  if (n >= sizeof(m_myMasks)/sizeof(OcpiPortMask))
    m_portMasks = m_allocated = new OcpiPortMask[n];
  else
    m_portMasks = m_myMasks;
}
void RunCondition::
setMasks(OcpiPortMask first, va_list ap) {
  ocpiAssert(!m_inUse);
  OcpiPortMask
    *pms = m_portMasks,
    m = first;
  do {
    *pms++ = m;
    m_allMasks |= m;
  } while (m && (m = va_arg(ap, OcpiPortMask)));
  *pms++ = 0; // ok since we always reserve 2
}
void RunCondition::
setPortMasks(OcpiPortMask pm, ...) {
  ocpiAssert(!m_inUse);
  delete [] m_allocated;
  m_allocated = NULL;
  m_allMasks = 0;
  va_list ap;
  va_start(ap, pm);
  initMasks(pm, ap);
  va_end(ap);
  va_start(ap, pm);
  setMasks(pm, ap);
  va_end(ap);
}
void RunCondition::
setPortMasks(OcpiPortMask *rpm) {
  ocpiAssert(!m_inUse);
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
activate(OCPI::OS::Timer &tmr, unsigned nPorts) const {
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
  m_inUse = true;
}
bool RunCondition::
shouldRun(OCPI::OS::Timer &timer, bool &timedOut, bool &bail) const {
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
