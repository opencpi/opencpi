/*
 * Abstact:
 *   This file contains the interface for the CPI Application class
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef CPI_APPLICATION_H
#define CPI_APPLICATION_H

#include "CpiParentChild.h"
#include "CpiContainerInterface.h"
#include "CpiWorker.h"
#include "CpiPValue.h"



namespace CPI {

  namespace Container {

    // A (local) application running in this container
    class Interface;
    class Artifact;

    class Application : public CPI::Util::Child<Interface, Application>, public CPI::Util::Parent<Worker> {
      friend class Interface;
      const char *m_name;
    protected:
      Application(Interface &, const char *name=0, CPI::Util::PValue *props = 0);
    public:
      virtual ~Application();

      // convenience.
      Artifact & loadArtifact(const char *url, CPI::Util::PValue *artifactParams = 0);

      // Convenience method that does loadArtifact(url), artifact->createWorker

      Worker & createWorker(const char *url, CPI::Util::PValue *aparams,
			    const void *entryPoint, const char * inst=NULL, CPI::Util::PValue *wparams = NULL);


      virtual Worker & createWorker(Artifact &, const char * impl, const char * inst=NULL,
                                    CPI::Util::PValue *wparams = NULL);


    };
  } // Container
} // CPI
#endif

