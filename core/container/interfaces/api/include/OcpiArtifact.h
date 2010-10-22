
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
#include "ezxml.h"
#include "OcpiParentChild.h"

namespace OCPI {

  namespace Container {

    // An artifact loaded into an application.
    // The underlying loaded artifact may persist for caching purposes, but this
    // class is the user-visible artifact loaded for the application.
    class Interface;
    class Worker;
    class PValue;
    class Artifact : public OCPI::Util::Parent<Worker>, public OCPI::Util::Child<Interface,Artifact> {
      friend class Application;
      friend class Interface;
    protected:
      Artifact(Interface &, const char *url);
      virtual Worker &createWorkerX(Application &a, ezxml_t impl, ezxml_t inst, OCPI::Util::PValue *) = 0;
      Worker &createWorker(Application &a, const char *impltag, const char *instTag, OCPI::Util::PValue *params = NULL);
      virtual ~Artifact();
      bool hasUrl(const char *url);

      const char *myUrl;
      char * myMetadata;
      ezxml_t myXml;
    };
  } // Container
} // OCPI
#endif

