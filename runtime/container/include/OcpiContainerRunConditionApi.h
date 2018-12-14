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

// This file defines run conditions, which is shared by RCC and OCL containers
// and is exposed to RCC workers.

#ifndef __CONTAINER_RUN_CONDITION_API_H__
#define __CONTAINER_RUN_CONDITION_API_H__
#ifndef __cplusplus
#include <stdint.h>
// These two typedefs are redundant with the C++ ones below, but not in a namespace.
typedef uint32_t OcpiPortMask;
typedef uint8_t OcpiBoolean;
#define OCPI_ALL_PORTS (~(OcpiPortMask)0) // macro here, const in C++
#define OCPI_NO_PORTS ((OcpiPortMask)0)   // macro here, const in C++
#else
#include <cstdint>
#include <cstdarg>
#include <cstdlib>
#include "OcpiConfigApi.h" // for OCPI_API_DEPRECATED
// Not exposed to workers at this time; if it is, multiple calls to assert() below should be changed:
// #include "OcpiOsAssert.h"
#include <cassert>

namespace OCPI {
  namespace OCL {
    class Worker;
  }
  namespace RCC {
    class Worker;
  }
  namespace OS {
    class Timer;
  }
}
namespace OCPI { namespace API {
// These two typedefs are redundant with the C ones above
typedef uint32_t OcpiPortMask;
typedef uint8_t OcpiBoolean;
const OcpiPortMask OCPI_ALL_PORTS = (~(OcpiPortMask)0);
const OcpiPortMask OCPI_NO_PORTS = ((OcpiPortMask)0);
class RunCondition {
  friend class OCPI::RCC::Worker;
  friend class OCPI::OCL::Worker;
public: // Should these be protected with friends reading?
  OcpiPortMask *m_portMasks;  // the masks used for checking
  OcpiBoolean   m_timeout;    // is timeout enabled?
  uint32_t      m_usecs;      // usecs of timeout, zero is valid
  bool          m_inUse;      // Are we currently registered? (AV-4722)
 private:
  OcpiPortMask  m_myMasks[3]; // non-allocated masks used almost all the time
  OcpiPortMask *m_allocated;  // NULL or allocated
  bool          m_hasRun;     // Have we run since being activated?
 protected:
  OcpiPortMask  m_allMasks;   // summary of all masks in the list
 public:
  // Constructors
  // Default constructor: no timeout, all ports must be ready
  RunCondition();
  // This allows a zero-terminated list of masks to be provided in the argument list.
  // No timeout is enabled.  A very common case.  If given one arg == 0, then never runs
  RunCondition(OcpiPortMask first, ...);
  // This allows the specification of a mask array (which can be nullptr) and a timeout.
  RunCondition(OcpiPortMask*, uint32_t usecs = 0, bool timeout = false);
  ~RunCondition();
  // These override the defaults:
  RunCondition(const RunCondition&) = delete; // Do not use
  RunCondition(RunCondition&&) = delete; // Do not use
  RunCondition& operator=(const RunCondition&);
  RunCondition& operator=(RunCondition&&);
  // backward compatibility for undocumented method
  // this was never intended to be used directly by users, but some did anyway, so we try and
  // preserve it until 2.0.  It has no effect unless the supplied nPorts is in fact less than
  // the actual number of ports of the worker.
  inline void initDefault(unsigned nPorts)
    OCPI_API_DEPRECATED("2.0",
			"Default constructor now defaults to all ports. Use constructor call with "
			"proper port mask if you need non-default.")
  {
    m_myMasks[0] = (OcpiPortMask)~(-1 << nPorts);
    m_myMasks[1] = 0;
    m_portMasks = nPorts ? m_myMasks : NULL;
    m_allMasks = m_myMasks[0];
  }
  // Support initializing from older C-langage run conditions (internal)
  inline void setRunCondition(OcpiPortMask *portMasks, OcpiBoolean timeout, uint32_t usecs) {
    assert(!m_inUse);
    m_portMasks = portMasks;
    m_timeout = timeout;
    m_usecs = usecs;
    m_allMasks = 0;
    for (OcpiPortMask *pm = m_portMasks; *pm; pm++)
      m_allMasks |= *pm;
  }
  // Disable the timeout, without changing its value
  inline void disableTimeout() { assert(!m_inUse); m_timeout = false; }
  // Enable the timeout, setting its value
  inline void enableTimeout(uint32_t usecs) { assert(!m_inUse); m_timeout = true; m_usecs = usecs; }
  // Enable the timeout, without changing its value
  inline void enableTimeout() { assert(!m_inUse); m_timeout = true; }
  inline void setTimeout(uint32_t usecs) { assert(!m_inUse); m_usecs = usecs; }
  void setPortMasks(OcpiPortMask first, ...);
  void setPortMasks(OcpiPortMask *);
 private:
  void initMasks(OcpiPortMask first, va_list ap);
  void setMasks(OcpiPortMask first, va_list ap);
  void activate(OCPI::OS::Timer &tmr, unsigned nPorts);
  // Return true if should run based on non-port info
  // Set timedout if we are running due to timeout.
  // Set hasRun
  // Set bail if should NOT run based on non-port info
 protected:
  bool shouldRun(OCPI::OS::Timer &tmr, bool &timedout, bool &bail);
};
}} // OCPI::API::
#endif // __cplusplus
#endif // __CONTAINER_RUN_CONDITION_API_H__
