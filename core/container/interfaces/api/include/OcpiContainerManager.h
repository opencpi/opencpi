
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

#ifndef OCPI_CONTAINER_MANAGER_H
#define OCPI_CONTAINER_MANAGER_H
#include "OcpiParentChild.h"
#include "OcpiDriverManager.h"
#include "OcpiContainerApi.h"
#include "OcpiContainerInterface.h"
#include "OcpiContainerApplication.h"
#include "OcpiContainerArtifact.h"
#include "OcpiWorker.h"
#include "OcpiContainerPort.h"
namespace OCPI {
  namespace Container {
    using OCPI::Util::Child;
    using OCPI::Util::Parent;
    class Driver;
    extern const char *container;
    // The concrete class that manages container drivers
    class Manager : public OCPI::API::ContainerManager,
		    public OCPI::Driver::ManagerBase<Manager, Driver, container> {
      unsigned cleanupPosition();
    public:
      OCPI::API::Container *find(const char *model, const char *which,
				 const OCPI::API::PValue *props);
      void shutdown();
    };

    // A base class inherited by all container drivers for common behavior
    class Driver : public OCPI::Driver::DriverType<Manager,Driver> {
    protected:
      Driver(const char *);
    public:
      virtual Container *findContainer(const char *which) = 0;
      virtual Container *probeContainer(const char *which,
					const OCPI::API::PValue *props = 0) = 0;
      // Any methods specific to container drivers (beyond what drivers do)
      // NONE at this time.
    };
    // A template class directly inherited by derived container drivers, with the
    // requirement that the name of the container driver be passed to the template.
    template <class ConcreteDriver, class ConcreteCont, const char *&name>
    class DriverBase :
      public OCPI::Driver::DriverBase
      <Manager, Driver, ConcreteDriver, ConcreteCont, name>
    {
      virtual Container *findContainer(const char *which) {
	if (!which)
	  return Parent<ConcreteCont>::firstChild();
	return Parent<ConcreteCont>::findChildByName(which);
      }
    };
    // The convenience class for driver registration/static instantiation
    template<class Driver>
    class RegisterContainerDriver : OCPI::Driver::Registration<Driver> {
    };

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
      ContainerBase<Dri, Con, App, Art>(const char *name,
					const ezxml_t config = NULL,
					const OCPI::Util::PValue *props = NULL)
	: OCPI::Driver::DeviceBase<Dri,Con>(name), Container(config, props) {}
      inline Driver &driver() { return OCPI::Driver::DeviceBase<Dri,Con>::parent(); }
      Artifact *findLoadedArtifact(const char *name) {
	return Parent<Art>::findChildByName(name);
      }
      Artifact *findLoadedArtifact(const OCPI::Library::Artifact &art) {
	return Parent<Art>::findChild(&Art::hasArtifact, (void *)&art);
      }
      App *firstApplication() const {
	return Parent<App>::firstChild();
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
      ApplicationBase<Con, App, Wrk>(Con &con, const char *name, const OCPI::Util::PValue *props)
      : Child<Con, App, application>(con, name), Application(props) {}
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
      ArtifactBase<Con,Art>(Con &con, OCPI::Library::Artifact &lart, const OCPI::Util::PValue *props)
      : Child<Con, Art, artifact>(con, lart.name().c_str()), Artifact(lart, props) {}
    public:
      inline Con &container() { return Child<Con,Art,artifact>::parent(); }
    };

    extern const char *worker;
    template<class App, class Wrk, class Prt>
    class WorkerBase
      : public Child<App,Wrk,worker>,
	public Parent<Prt>,
        public Worker
    {
    protected:
      WorkerBase<App,Wrk,Prt>(App &app, Artifact *art, const char *name,
			  ezxml_t impl, ezxml_t inst, const OCPI::Util::PValue *props)
      : Child<App,Wrk,worker>(app, name), Worker(art, impl, inst, props) {
      }
      Application &application() { return Child<App,Wrk,worker>::parent(); }
      Port *findPort(const char *name) { return Parent<Prt>::findChildByName(name); }
      Worker *nextWorker() { return Child<App,Wrk,worker>::nextChild(); }
    };
    extern const char *port;
    template<class Wrk, class Prt, class Ext>
    class PortBase
      : public Child<Wrk, Prt, port>,
        public Parent<Ext>,
        public Port {
    protected:
      PortBase<Wrk,Prt,Ext>(Wrk &worker, const OCPI::Util::PValue *props,
			    const OCPI::Metadata::Port &mport, bool isProvider)
      : Child<Wrk,Prt,port>(worker, mport.name), Port(worker.parent().parent(), mport,
						      isProvider, props) {}
    };
    extern const char *externalPort;
    template<class Prt, class Ext>
    class ExternalPortBase
      : public Child<Prt,Ext,externalPort>,
        public ExternalPort {
    protected:
      ExternalPortBase<Prt,Ext>(Prt &port, const char *name,
				const OCPI::Util::PValue *props,
				const OCPI::Metadata::Port &metaPort,
				bool isProvider)
      : Child<Prt,Ext,externalPort>(port, name),
	ExternalPort(metaPort, isProvider, props) {}
    };
  }
}
#endif
