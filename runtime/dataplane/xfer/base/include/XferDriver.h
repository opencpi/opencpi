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

// This file is included by transfer driver implementations and provides templates
// for the classes in each driver

#ifndef XferDriver_h
#define XferDriver_h

#include "OcpiDriverManager.h"
#include "XferFactory.h"

struct XFTemplate;
namespace DataTransfer {

  class EndPoint;
  class XferRequest;
  class XferFactory;
  class XferServices;
  class XferManager;
  class Device;
  // The template class directly inherited by concrete transfer drivers, with the
  // requirement that the name of the driver be passed to the template
  // using a static const char* variable.
  // We inherit XferFactory class here among other things.
  template <class ConcreteDriver, class ConcreteDevice, class ConcreteSvcs,
	    const char *&name, class Base = DataTransfer::XferFactory>
  class DriverBase :
    public OCPI::Util::Parent<ConcreteSvcs>, // destroy these last
    public OCPI::Driver::DriverBase<XferManager, Base,
				    ConcreteDriver, ConcreteDevice, name>
  {
  };
  template <class Dri, class Dev>
  class DeviceBase :
    public OCPI::Driver::DeviceBase<Dri,Dev>,
    public Device
  {
  protected:
    DeviceBase<Dri, Dev>(const char *childName, Dev &dev)
    : OCPI::Driver::DeviceBase<Dri, Dev>(childName, dev)
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
    ConnectionBase<ConcDri, ConcConn, ConcXfer, Base>(ConcConn &cc, EndPoint &source,
						      EndPoint &target)
    : OCPI::Util::Child<ConcDri,ConcConn>(OCPI::Util::Singleton<ConcDri>::
					   getSingleton(), cc),
      Base(OCPI::Util::Singleton<ConcDri>::getSingleton(), source, target)
    {}
  };
    template <class ConcConn, class ConcXfer, class Base = XferRequest>
  class TransferBase :
    public Base,
    public OCPI::Util::Child<ConcConn, ConcXfer>
  {
  protected:
    TransferBase<ConcConn, ConcXfer, Base>(ConcConn &conn, ConcXfer &xfer, XFTemplate *temp = NULL)
    : Base(temp), OCPI::Util::Child<ConcConn,ConcXfer>(conn, xfer) {}
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
