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
 *   This file contains the interface for the CPi Artifact class
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef CONTAINER_ARTIFACT_H
#define CONTAINER_ARTIFACT_H
#include <list>
#include "ezxml.h"
#include "OcpiLibraryManager.h"

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
    public:
      // Make sure this is loaded and ready to execute in case it was unloaded
      virtual void ensureLoaded() {}
      const OCPI::Library::Artifact &libArtifact() const { return m_libArtifact; }
      virtual const std::string &name() const = 0;
      bool hasArtifact(const void *art);
      Worker &createWorker(Application &a,  const char *instName,
			   const char *impltag, const char *instTag,
			   const OCPI::Util::PValue *props = NULL,
			   const OCPI::Util::PValue *params = NULL);
      Worker &createWorker(Application &app,
			   const char *appInstName,
			   ezxml_t impl, ezxml_t inst, const OCPI::Container::Workers &slaves,
			   bool hasMaster, size_t member, size_t crewSize,
			   const OCPI::Util::PValue *wparams = NULL);
    protected:
      virtual ~Artifact();
    };
  } // Container
} // OCPI
#endif

