#include <string.h>
#include "OcpiContainerManager.h"
#include "OcpiContainerErrorCodes.h"
#include "OcpiRccContainer.h"
#include "OcpiTransport.h"
#include "OcpiUtilMisc.h"
// This is the "driver" for RCC containers, which finds them, constructs them, and 
// in general manages them.  It is an object manages by the Container::Manager class.
// It acts as the factory for RCC containers.

namespace OC = OCPI::Container;
namespace OU = OCPI::Util;
namespace OA = OCPI::API;
namespace OCPI {
  namespace RCC {
    const char *rcc = "rcc";
    Driver::
    Driver() throw() 
      :
      //      m_tpg_events(NULL), m_tpg_no_events(NULL), 
      m_count(0) {
      ocpiCheck(pthread_key_create(&s_threadKey, NULL) == 0);
      ocpiDebug("Registering the RCC Container driver");
    }
    pthread_key_t Driver::s_threadKey;
    // Look for a container that doesn't exist yet.
    OC::Container *Driver::
    probeContainer(const char *which, std::string &/*error*/, const OA::PValue *params)
	throw ( OU::EmbeddedException )
    {
      return new Container(which, params);
    }
    // Per driver discovery routine to create devices
    unsigned Driver::
    search(const OA::PValue* /* params */, const char **/* exclude */, bool /* discoveryOnly */)
      throw ( OU::EmbeddedException ) {
      std::string error;
      return probeContainer("rcc0", error, NULL) ? 1 : 0;
    }
    Driver::
    ~Driver()
      throw ( )
    {
      // Force containers to shutdown before we remove transport globals.
      OU::Parent<Container>::deleteChildren();
      //      if ( m_tpg_no_events ) delete m_tpg_no_events;
      //      if ( m_tpg_events ) delete m_tpg_events;
      ocpiCheck(pthread_key_delete(s_threadKey) == 0);
    }
    // Register this driver
    OC::RegisterContainerDriver<Driver> driver;
  }
}
