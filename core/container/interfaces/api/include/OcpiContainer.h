
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
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
// The API-level container interfaces
// FIXME:  the API container class should be decoupled from the underlying implementation
#ifndef OCPI_CONTAINER_H
#define OCPI_CONTAINER_H
#include "OcpiPValue.h"
#include "OcpiContainerInterface.h"

namespace OCPI {
  namespace API {
    typedef Container::Interface Container;
    typedef Util::PValue PValue;
    // The singleton class that manages all container drivers
    class ContainerDriverManager : public DriverManager {
      static Container
	*find(const char *model, const char *processor = NULL, const char *machine = NULL,
	      const char *unit = NULL),
	*find(const PValue *list);
    };
    /*
     * A driver class is code that manages "devices".  We want a dynamic object so we can control its lifecycle.
     * In case we want to shutdown without running static destructors.
     * So we have a trivial template class whose purpose is simply to construct the driver object and connect 
     * it to its parent class.  Since the constructor of the driver class runs at static-construction time
     * (but is NOT itself a static constructor), it must be minimal.  A separate "initialize" member function
     * will be invoked by its parent at some later time.
     *
     * We specialize driver managers so we manage a class of drivers separate from other drivers.
     * We specialize drivers so that specialized driver managers can do unique things with their own class
     * of drivers.  The parent-child relationship between a driver and its manager is in the base class.
     * So if a driver manager needs to iterate over its drivers, it will need to use the child list in the
     * base class and downcast to the particular driver class.  OTOH if the parent-child relationship is in the
     * derived class, the casting will not be necessary, but then every derived class (manager and driver)
     * will have to declare the relationship UNLESS THE BASE CLASS IS A TEMPLATE CLASS...
     * So:  the Driver and DriverManager base classed should be template classes.
    */
    // The class used by each container driver to register itself
    template <class D> class ContainerDriverF{ 
    public:
      DriverF<D>(){new D;}
      DriverF<D>( Parent<ContainerDriverManager>& p){new D;}
      };
    



  }
}
#endif
