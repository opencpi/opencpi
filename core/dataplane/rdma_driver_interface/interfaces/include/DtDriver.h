// This file is included by transfer driver implementations and provides templates
// for the classes in each driver
        
#ifndef DtDriver_h
#define DtDriver_h
#include "DtTransferInternal.h"

namespace DataTransfer {

  // The template class directly inherited by concrete transfer drivers, with the
  // requirement that the name of the driver be passed to the template
  // using a static const char* variable.
  // We inherit XferFactory class here among other things.
  template <class ConcreteDriver, class ConcreteDevice, class ConcreteSvcs,
	    const char *&name>
  class DriverBase :
    public OCPI::Driver::DriverBase<XferFactoryManager, DataTransfer::XferFactory,
				    ConcreteDriver, ConcreteDevice, name>,
    public OCPI::Util::Parent<ConcreteSvcs>
  {
  };
  template <class Dri, class Dev>
  class DeviceBase :
    public OCPI::Driver::DeviceBase<Dri,Dev>,
    public Device
  {
  protected:
    DeviceBase<Dri, Dev>(const char *childName)
    : OCPI::Driver::DeviceBase<Dri, Dev>(childName)
    {}
    inline XferFactory &driverBase() {
      return OCPI::Driver::DeviceBase<Dri,Dev>::parent();
    }
  };

  // The template class  directory inherited by concrete connecion classes 
  // (a.k.a. XferServices)
  template <class ConcDri, class ConcConn, class ConcXfer>
  class ConnectionBase :
    public OCPI::Util::Child<ConcDri,ConcConn>,
    public OCPI::Util::Parent<ConcXfer>,
    public XferServices
  {
  protected:
    ConnectionBase<ConcDri, ConcConn, ConcXfer>(SmemServices* source,
						SmemServices* target)
    : OCPI::Util::Child<ConcDri,ConcConn> (OCPI::Driver::Singleton<ConcDri>::
					   getSingleton()),
      XferServices(source, target)
    {}
  };
  template <class ConcConn, class ConcXfer>
  class TransferBase :
    public XferRequest,
    public OCPI::Util::Child<ConcConn, ConcXfer>
  {
  protected:
    TransferBase<ConcConn, ConcXfer>(ConcConn &conn)
    : OCPI::Util::Child<ConcConn,ConcXfer>(conn) {}
  };
  template <class Dri>
  class RegisterTransferDriver
    : OCPI::Driver::Registration<Dri>
  {};
}
#endif
