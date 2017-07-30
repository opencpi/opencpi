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

/*
 * Abstract:
 *   This file contains the interface for the OCPI Application class
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef CONTAINER_APPLICATION_H
#define CONTAINER_APPLICATION_H

#include "OcpiContainerApi.h"

#include "OcpiOsTimer.h"
#include "OcpiPValue.h"
#include "OcpiLibraryManager.h"

namespace OCPI {
  namespace API {
    class Application;
  }
  namespace Container {

    // A (local) application running in this container
    class Artifact;
    class Container;
    class Worker;
    class Application :
      public OCPI::API::ContainerApplication {
      friend class Container;
      friend class Artifact;
      std::string m_package;
      OCPI::API::Application *m_apiApplication; // top level app if there is one
    protected:
      Application(const OCPI::Util::PValue *props = NULL);
    public:
      void setApplication(OCPI::API::Application *app) { m_apiApplication = app; }
      OCPI::API::Application *getApplication() { return m_apiApplication; }
      virtual Container &container() = 0;
      virtual Worker &createWorker(Artifact *, const char *appInstName,
				   ezxml_t impl, ezxml_t inst, Worker *slave, bool hasMaster,
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
				       const char *selectCriteria = NULL);
      // This is the API method to create a worker from libraries
      OCPI::API::Worker &createWorker(const char *appInstName, const char *specName,
				      const OCPI::API::PValue *wParams = NULL,
				      const OCPI::Util::PValue *wprops = NULL,
				      const char *selectCriteria = NULL,
				      const OCPI::API::Connection *connections = NULL);
      Worker &createWorker(OCPI::Library::Artifact &art, const char *appInstName, 
			   const ezxml_t impl, const ezxml_t inst, Worker *slave, bool hasMaster,
			   const OCPI::Util::PValue *wparams = NULL);


      virtual Worker *firstWorker() const = 0;
      //      void start();
      // If not master, then we ignore slave, so there are three cases
      void startMasterSlave(bool isMaster, bool isSlave, bool IsSource);
      void stop(bool isMaster, bool isSlave);
      void release(bool isMaster, bool isSlave);
      bool isDone();
      // This method should block until all the workers in the application are "done".
      virtual bool wait(OCPI::OS::Timer *timer = NULL);
    };
  } // Container
} // OCPI
#endif

