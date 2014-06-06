
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
 *   Container application context class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_RCC_APPLICATION_H__
#define OCPI_RCC_APPLICATION_H__

#include <vector>
#include <OcpiList.h>
#include <OcpiOsDataTypes.h>
#include <OcpiContainerDataTypes.h>
#include <OcpiContainerApplication.h>
#include <OcpiContainerManager.h>
#include <OcpiLibraryManager.h>
#include <RCC_Worker.h>
#include <OcpiOsLoadableModule.h>


namespace OCPI {

  namespace DataTransport {
    class Transport;
    class Circuit;
  }

    namespace RCC {

      class Worker;
      class Container;

      class Artifact : public OCPI::Container::ArtifactBase<Container,Artifact> {
	friend class Container;

	Artifact(Container &c, OCPI::Library::Artifact &lart, const OCPI::API::PValue *artifactParams);

      public:
	virtual ~Artifact();
	OCPI::Container::Worker &
	  createWorkerX(OCPI::Container::Application &a, const char *name, ezxml_t impl,
			ezxml_t inst, const OCPI::API::PValue *execProps);
	RCCEntryTable *getDispatch(const char *name);
      private:
	
        OCPI::OS::LoadableModule m_loader;
	RCCEntryTable           *m_entryTable;
	bool                     m_open;
        int                      m_workerCount;

      };


      /**********************************
       * Containers represented Node Application class
       *********************************/
      class Application
	: public OCPI::Container::ApplicationBase<Container,Application,Worker>
      {
        friend class Container;
        friend class Controller;
      protected:
	void run(DataTransfer::EventManager* event_manager, bool &more_to_do);
      public:

	OCPI::Container::Worker &
	  createWorker(OCPI::Container::Artifact *art, const char *appInstName,
		       ezxml_t impl, ezxml_t inst,
		       const OCPI::Util::PValue *wParams);

      /**********************************
       * Constructor
       *********************************/  
        Application(Container &, const char *, const OCPI::Util::PValue *);

      /**********************************
       * Destructor
       *********************************/  
        virtual ~Application();

#if 0
We are assuming that the circuits are lifecycle-managed based on ref counting from ports
and thus this bookkeepping is not neededhere.
It wasnt actually used anyway.
      /**********************************
       * Add a circuit to our managed list
       *********************************/ 
      inline void addCircuit( OCPI::DataTransport::Circuit* c ){m_circuits.push_back(c);}
#endif      
      private:

      // RCC container
      RCCContainer *  m_rccContainer;

      // All circuits in this application
      //      std::vector<OCPI::DataTransport::Circuit*> m_circuits;

    };

    }
}

#endif

