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
 *   This file defines classes and templates for the container manager and containers.
 *   It is based on the generic ManagerManager->Manager->Driver->Device stack defined in
 *   OcpiDriverManager.h
 *   Where "Device" is a "Container" object that implements an OpenCPI container.
 *   There is one singleton Container::Manager to manage all container drivers.
 *   Each container driver implements a certain type of container, acting as a factory
 *   for those containers (devices).
 *   The inclusion of the separate "ContainerInterface" file is temporary...FIXME.
 *
 * Revision History:
 *
 *    Author: Jim Kulp
 *    Date: 2/2011
 *    Revision Detail: Created
 *
 */

#ifndef CONTAINER_MANAGER_H
#define CONTAINER_MANAGER_H
#include "ContainerDriver.h"
#include "Container.h"
#include "ContainerApplication.h"
#include "ContainerArtifact.h"
#include "ContainerWorker.h"
#include "ContainerPort.h"

namespace OCPI {
  namespace Container {
    // The templated base class inherited by all actual container classes
    // It establishes itself as a device/child for its driver, and inherits the
    // API interface class.  The template parameter is the concrete container
    // class that is inheriting this base class.  This parameter allows the
    // driver use the concrete/derived container class without casting.
    template <class Dri, class Con, class App, class Art>
    class ContainerBase
      : public OCPI::Driver::DeviceBase<Dri,Con>, // for the relationship with our driver
	public Parent<Art>,
	public Parent<App>,
	public Container
    {
    protected:
      ContainerBase<Dri, Con, App, Art>(Con &con, const char *a_name,
					const ezxml_t config = NULL,
					const OCPI::Util::PValue *props = NULL)
	: OCPI::Driver::DeviceBase<Dri,Con>(a_name, con), Container(a_name, config, props) {}
      inline Driver &driver() { return OCPI::Driver::DeviceBase<Dri,Con>::parent(); }
      Artifact *findLoadedArtifact(const char *a_name) {
	return Parent<Art>::findChildByName(a_name);
      }
      Artifact *findLoadedArtifact(const OCPI::Library::Artifact &art) {
	Artifact *a = Parent<Art>::findChild(&Art::hasArtifact, (void *)&art);
	if (a)
	  a->ensureLoaded();
	return a;
      }
      App *firstApplication() const {
	return Parent<App>::firstChild();
      }
      Container *nextContainer() { return OCPI::Driver::DeviceBase<Dri,Con>::nextDevice(); }
    public:
      const std::string &name() const {
	return OCPI::Driver::DeviceBase<Dri,Con>::name();
      }
    };
    extern const char *application;
    template<class Con, class App, class Wrk>
    class ApplicationBase
      : public Child<Con, App, application>,
	public Parent<Wrk>,
        public Application
    {
    protected:
      ApplicationBase<Con, App, Wrk>(Con &con, App &app, const char *a_name,
				     const OCPI::Util::PValue *props)
      : Child<Con, App, application>(con, app, a_name), Application(props) {}
    public:
      Container &container() { return Child<Con,App,application>::parent(); }
      Worker *firstWorker() const {
	return Parent<Wrk>::firstChild();
      }
    };
    class Worker;
    // The template inherited by concrete artifact classes that represent LOADED artifacts.
    extern const char *artifact;
    template<class Con, class Art>
    class ArtifactBase
      : public Child<Con, Art, artifact>,
        public Artifact {
    protected:
      ArtifactBase<Con,Art>(Con &con, Art &art, OCPI::Library::Artifact &lart, const OCPI::Util::PValue *props)
      : Child<Con, Art, artifact>(con, art, lart.name().c_str()), Artifact(lart, props) {}
    public:
      inline Con &container() { return Child<Con,Art,artifact>::parent(); }
      inline const std::string &name() const { return Child<Con, Art, artifact>::name(); }
    };

    extern const char *worker;
    template<class App, class Wrk, class Prt>
    class WorkerBase
      : public Child<App,Wrk,worker>,
	public Parent<Prt>,
        public Worker
    {
    protected:
      WorkerBase<App,Wrk,Prt>(App &app, Wrk &wrk, Artifact *art, const char *a_name,
			      ezxml_t impl, ezxml_t inst, const Workers &a_slaves,
			      bool a_hasMaster, size_t a_member, size_t a_crewSize,
			      const OCPI::Util::PValue *params)
      : Child<App,Wrk,worker>(app, wrk, a_name),
	Worker(art, impl, inst, a_slaves, a_hasMaster, a_member, a_crewSize, params)
      {
      }
      Application *application() { return &Child<App,Wrk,worker>::parent(); }
      Port *findPort(const char *a_name) { return Parent<Prt>::findChildByName(a_name); }
      Worker *nextWorker() { return Child<App,Wrk,worker>::nextChild(); }
    public:
      const std::string &name() const { return Child<App,Wrk,worker>::name(); }
    };
  }
}
#endif
