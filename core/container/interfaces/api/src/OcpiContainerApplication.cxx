
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

#include "OcpiContainerInterface.h"
#include "OcpiContainerApplication.h"
#include "OcpiContainerArtifact.h"
#include "OcpiPValue.h"


namespace OA = OCPI::API;
namespace OL = OCPI::Library;

namespace OCPI {
  namespace Container {
    Application::Application(const OA::PValue *) {
    }
    Application::~Application() {
    }

    OA::Worker &Application::
    createWorker(const char *url, const OA::PValue *aParams, const char *name,
		 const char *impl, const char *inst,
		 const OA::PValue *wProps, const OA::PValue *wParams,
		 const OA::PValue * /* selectCriteria */ , 
		 const OA::Connection * /* connections */) {
      if (url)
	return container().loadArtifact(url, aParams).createWorker(*this, name,
								   impl, inst,
								   wProps, wParams);
      // This is the special hack for passing in a dispatch table for RCC workers.
      else {
	Worker &w = createWorker(NULL, name, (ezxml_t)NULL, (ezxml_t)NULL, aParams);
	w.initialize();
	return w;
      }
    }
    OA::Worker &Application::
    createWorker(const char *name, const char *impl,
		 const OA::PValue *wProps,
		 const OA::PValue *wParams,
		 const OA::PValue *selectCriteria, 
		 const OA::Connection *connections) {
      // Find an artifact (and instance within the artifact), for this worker
      const char *inst = NULL;
      OL::Artifact &a =
	OL::Manager::findArtifact(container(), impl, wParams, selectCriteria,  connections, inst);
      // Load the artifact and create the worker
      return
	container().loadArtifact(a).createWorker(*this, name, impl, inst, wProps, wParams);
    }
    void Application::
    start() {
      for (Worker *w = firstWorker(); w; w = w->nextWorker())
	w->start();
    }
  }
  namespace API {
    ContainerApplication::~ContainerApplication(){}
  }
}
