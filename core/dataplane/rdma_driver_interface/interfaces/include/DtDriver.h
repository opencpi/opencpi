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
	    const char *&name, class Base = DataTransfer::XferFactory>
  class DriverBase :
    public OCPI::Util::Parent<ConcreteSvcs>, // destroy these last
    public OCPI::Driver::DriverBase<XferFactoryManager, Base,
				    ConcreteDriver, ConcreteDevice, name>
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
  template <class ConcDri, class ConcConn, class ConcXfer, class Base = XferServices>
  class ConnectionBase :
    public OCPI::Util::Child<ConcDri,ConcConn>,
    public OCPI::Util::Parent<ConcXfer>,
      public Base
  {
  protected:
    ConnectionBase<ConcDri, ConcConn, ConcXfer, Base>(SmemServices* source,
						      SmemServices* target)
    : OCPI::Util::Child<ConcDri,ConcConn> (OCPI::Driver::Singleton<ConcDri>::
					   getSingleton()),
      Base(source, target)
    {}
  };
    template <class ConcConn, class ConcXfer, class Base = XferRequest>
  class TransferBase :
    public Base,
    public OCPI::Util::Child<ConcConn, ConcXfer>
  {
  protected:
  TransferBase<ConcConn, ConcXfer, Base>(ConcConn &conn, XF_template temp = NULL)
      : Base(temp), OCPI::Util::Child<ConcConn,ConcXfer>(conn) {}
    // Allow the base class to get at the derived parent
    // To do that it needs to declare a pure virtual method
    ConcConn &parent() { return OCPI::Util::Child<ConcConn,ConcXfer>::parent(); }
  };
  template <class Dri>
  class RegisterTransferDriver
    : OCPI::Driver::Registration<Dri>
  {};
}
#endif
