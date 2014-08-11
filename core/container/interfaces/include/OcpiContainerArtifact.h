
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
 *   This file contains the interface for the CPi Artifact class
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_ARTIFACT_H
#define OCPI_ARTIFACT_H
#include <list>
#include "ezxml.h"
#include "OcpiParentChild.h"

namespace OCPI {

  namespace Container {

    // An artifact loaded into the container.
    // The OCPI::Library::Artifact represents the artifact in a library whether or
    // not it is loaded.  This class represents that class actually loaded in this
    // container.
    class Interface;
    class Worker;
    class Artifact {
      friend class Application;
      friend class Interface;
      friend class Worker;
      // This list is to remember which workers (which are owned by their container apps),
      // are using the artifact.
      typedef std::list<Worker*> Workers;
      typedef Workers::const_iterator WorkersIter;
      Workers m_workers; // what workers exist created based on this loaded artifact
      OCPI::Library::Artifact &m_libArtifact;
    protected:
      Artifact(OCPI::Library::Artifact &lart, const OCPI::Util::PValue *props = NULL);
      void removeWorker(Worker &);
      // Is this library artifact the one I loaded?
    public:
      virtual const std::string &name() const = 0;
      bool hasArtifact(const void *art);
      Worker &createWorker(Application &a,  const char *instName,
			   const char *impltag, const char *instTag,
			   const OCPI::Util::PValue *props = NULL,
			   const OCPI::Util::PValue *params = NULL);
      Worker &createWorker(Application &app,
			   const char *appInstName,
			   ezxml_t impl, ezxml_t inst, Worker *slave,
			   const OCPI::Util::PValue *wparams = NULL);
    protected:
      virtual ~Artifact();
    };
  } // Container
} // OCPI
#endif

