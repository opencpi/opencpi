
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

/*
 * Abstact:
 *   This file contains the interface for the OCPI Application class
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_CONTAINER_APPLICATION_H
#define OCPI_CONTAINER_APPLICATION_H

#include "OcpiParentChild.h"
#include "OcpiContainerInterface.h"
#include "OcpiWorker.h"
#include "OcpiPValue.h"

namespace OCPI {
  namespace Container {

    // A (local) application running in this container
    class Artifact;
    class Application :
      public OCPI::API::ContainerApplication
    {
      friend class Container;
      friend class Artifact;
    protected:
      Application(const OCPI::Util::PValue *props = 0);
    public:
      virtual Container &container() = 0;
      virtual Worker & createWorker(Artifact *, const char *appInstName,
				    ezxml_t impl, ezxml_t inst,
				    const OCPI::Util::PValue *wparams) = 0;
      virtual ~Application();

      // This the API method using explicit artifact file names
      OCPI::API::Worker & createWorker(const char *url,
				       const OCPI::API::PValue *aparams,
				       const char *appInstName,
				       const char *implName,
				       const char *preInstName,
				       const OCPI::Util::PValue *wprops,
				       const OCPI::API::PValue *wparams,
				       const OCPI::Util::PValue *selectCriteria = NULL);
      // This is the API method to create a worker from libraries
      OCPI::API::Worker &createWorker(const char *appInstName, const char *specName,
				      const OCPI::API::PValue *wParams = NULL,
				      const OCPI::Util::PValue *wprops = NULL,
				      const OCPI::Util::PValue *selectCriteria = NULL,
				      const OCPI::API::Connection *connections = NULL);
      virtual Worker *firstWorker() const = 0;
      void start();
    };
  } // Container
} // OCPI
#endif

