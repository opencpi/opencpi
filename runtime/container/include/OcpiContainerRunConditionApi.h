// This file defines run conditions, which is shared by RCC and OCL containers
// and is exposed to RCC workers.

#ifndef __CONTAINER_RUN_CONDITION_API_H__
#define __CONTAINER_RUN_CONDITION_API_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
namespace OCPI { namespace API {
#endif
typedef uint32_t OcpiPortMask;
typedef uint8_t OcpiBoolean;
#ifndef __cplusplus
#define OCPI_ALL_PORTS (~(OcpiPortMask)0)
#define OCPI_NO_PORTS ((OcpiPortMask)0)
#else
const OcpiPortMask OCPI_ALL_PORTS = (~(OcpiPortMask)0);
const OcpiPortMask OCPI_NO_PORTS = ((OcpiPortMask)0);
}}
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
class RunCondition {
  friend class OCPI::RCC::Worker;
  friend class OCPI::OCL::Worker;
 public:
  OcpiPortMask *m_portMasks;  // the masks used for checking
  OcpiBoolean  m_timeout;    // is timeout enabled?
  uint32_t     m_usecs;      // usecs of timeout, zero is valid
 private:
  OcpiPortMask  m_myMasks[3]; // non-allocated masks used almost all the time
  OcpiPortMask *m_allocated;  // NULL or allocated
  bool         m_hasRun;     // Have we run since being activated?
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
  // backward compatibility for undocumented method
  inline void initDefault(unsigned /*nPorts*/) {}
  // Support initializing from older C-langage run conditions (internal)
  inline void setRunCondition(OcpiPortMask *portMasks, OcpiBoolean timeout, uint32_t usecs) {
    m_portMasks = portMasks;
    m_timeout = timeout;
    m_usecs = usecs;
    m_allMasks = 0;
    for (OcpiPortMask *pm = m_portMasks; *pm; pm++)
      m_allMasks |= *pm;
  }
  // Disable the timeout, without changing its value
  inline void disableTimeout() { m_timeout = false; }
  // Enable the timeout, setting its value
  inline void enableTimeout(uint32_t usecs) { m_timeout = true; m_usecs = usecs; }
  // Enable the timeout, without changing its value
  inline void enableTimeout() { m_timeout = true; }
  inline void setTimeout(uint32_t usecs) { m_usecs = usecs; }
  void setPortMasks(OcpiPortMask first, ...);
  void setPortMasks(OcpiPortMask *);
 private:
  void initMasks(va_list ap);
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
