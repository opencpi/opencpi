/*
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
#ifndef OCPI_RCC_RUN_CONDITION_H
#define OCPI_RCC_RUN_CONDITION_H

// Hackish: we need to access OS timer class, but don't want to expose that header since
// this file is included in component headers
namespace OCPI { namespace OS { class Timer; } }

namespace OCPI {
  namespace RCC {
    typedef bool RCCBoolean;
    typedef uint32_t RCCPortMask;
    typedef struct {
      RCCPortMask *portMasks;
      RCCBoolean   timeout;
      uint32_t     usecs;
    } RCCRunCondition;
    struct RunCondition {
      RCCPortMask *m_portMasks;  // the masks used for checking
      RCCPortMask  m_myMasks[3]; // non-allocated masks used almost all the time
      RCCBoolean   m_timeout;    // is timeout enabled?
      uint32_t     m_usecs;      // usecs of timeout, zero is valid
      RCCPortMask *m_allocated;  // NULL or allocated
      RCCPortMask  m_allMasks;   // summary of all masks in the list
      bool         m_hasRun;     // Have we run since being activated?
      // Constructors
      // Default constructor: no timeout, all ports must be ready
      RunCondition();
      // This allows a zero-terminated list of masks to be provided in the argument list.
      // No timeout is enabled.  A very common case.  If given one arg == 0, then never runs
      RunCondition(RCCPortMask first, ...);
      // This allows the specification of a mask array (which can be nullptr) and a timeout.
      RunCondition(RCCPortMask*, uint32_t usecs = 0, bool timeout = false);
      ~RunCondition();
      // initialize the default run condition, given how many ports there are
      // assume default contructor has already been run
      inline void initDefault(unsigned nPorts) {
	m_myMasks[0] = ~(-1 << nPorts);
	m_myMasks[1] = 0;
	m_portMasks = nPorts ? m_myMasks : NULL;
	m_allMasks = m_myMasks[0];
      }
      // Compatibility hack to support older C-langage run conditions
      inline void setRunCondition(const RCCRunCondition &crc) {
	m_portMasks = crc.portMasks;
	m_timeout = crc.timeout;
	m_usecs = crc.usecs;
	m_allMasks = 0;
	for (RCCPortMask *pm = m_portMasks; *pm; pm++)
	  m_allMasks |= *pm;
      }
      // Disable the timeout, without changing its value
      inline void disableTimeout() { m_timeout = false; }
      // Enable the timeout, setting its value
      inline void enableTimeout(uint32_t usecs) { m_timeout = true; m_usecs = usecs; }
      // Enable the tinmeout, without changing its value
      inline void enableTimeout() { m_timeout = true; }
      inline void setTimeout(uint32_t usecs) { m_usecs = usecs; }
      void activate(OCPI::OS::Timer &tmr);
      // Return true if should run based on non-port info
      // Set timedout if we are running due to timeout.
      // Set hasRun
      // Set bail if should NOT run based on non-port info
      bool shouldRun(OCPI::OS::Timer &tmr, bool &timedout, bool &bail);
    };
  }
}
#endif
