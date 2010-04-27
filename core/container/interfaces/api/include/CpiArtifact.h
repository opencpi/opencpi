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

#ifndef CPI_ARTIFACT_H
#define CPI_ARTIFACT_H
#include "ezxml.h"
#include "CpiParentChild.h"

namespace CPI {

  namespace Container {

    // An artifact loaded into an application.
    // The underlying loaded artifact may persist for caching purposes, but this
    // class is the user-visible artifact loaded for the application.
    class Interface;
    class Worker;
    class PValue;
    class Artifact : public CPI::Util::Parent<Worker>, public CPI::Util::Child<Interface,Artifact> {

#ifdef WAS
    class Artifact : public CPI::Util::Parent<Worker>  {
#endif

      const char *myUrl;
      char * myMetadata;
      ezxml_t myXml;
      friend class Application;
      friend class Interface;
    protected:
      virtual Worker &createWorkerX(Application &a, ezxml_t impl, ezxml_t inst, CPI::Util::PValue *) = 0;
      Worker &createWorker(Application &a, const char *impltag, const char *instTag, CPI::Util::PValue *params = NULL);
      Artifact(Interface &, const char *url);
      virtual ~Artifact();
      bool hasUrl(const char *url);
    };
  } // Container
} // CPI
#endif

