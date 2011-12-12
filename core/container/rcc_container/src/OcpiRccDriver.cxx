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
      : m_tpg_events(NULL), m_tpg_no_events(NULL), m_count(0) {
      printf("Registering the RCC Container driver\n");
    }
    // Look for a container that doesn't exist yet.
    OC::Container *Driver::
    probeContainer(const char *which, const OA::PValue *props)
	throw ( OU::EmbeddedException )
    {
      static unsigned event_range_start = 0;
      bool polled = true;
      OU::findBool(props, "polled", polled);
      OCPI::DataTransport::TransportGlobal **tpg(polled ? &m_tpg_no_events : &m_tpg_events);
	  
      OCPI::RCC::Container *rcc;	  
      try {
	if (!*tpg)
	  *tpg = new OCPI::DataTransport::TransportGlobal( event_range_start++, !polled );
	rcc = new Container(which, *tpg, props);
      } catch( std::bad_alloc ) {
	throw OU::EmbeddedException( OU::NO_MORE_MEMORY, "new", OU::ContainerFatal);
      }
      return rcc;
    }
    // Per driver discovery routine to create devices
    unsigned Driver::
    search(const OA::PValue* props, const char **exclude)
      throw ( OU::EmbeddedException ) {
      (void) props; (void)exclude;
      probeContainer("rcc0", NULL);
      return 1;
    }
    Driver::
    ~Driver()
      throw ( )
    {
      // Force containers to shutdown before we remove transport globals.
      OU::Parent<Container>::deleteChildren();
      if ( m_tpg_no_events ) delete m_tpg_no_events;
      if ( m_tpg_events ) delete m_tpg_events;
    }
    // Register this driver
    OC::RegisterContainerDriver<Driver> driver;
  }
}
