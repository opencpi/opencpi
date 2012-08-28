
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
 * The user-level API for applications (via the ACI).
 * Usage is:
 * {
 *    OA::Application app(filename_of_xml_file); // throws error relative to the xml_file
 *    app.initialize();                          // throws errors relative to deploying the app
 *    app.start();
 *    // wait until you are happy with the result...
 * }
 */
#include "OcpiContainerApi.h"

namespace OCPI {
  namespace API {
    class ApplicationI;
    class Application {
      ApplicationI &m_application;
    public:
      // The constructor does the planning, deciding what impl will run where
      explicit Application(const char *file, const OCPI::API::PValue *params = NULL);
      explicit Application(const std::string &string, const OCPI::API::PValue *params = NULL);
      
      // Creates a new Application instance from the app template
      explicit Application(Application & app, const OCPI::API::PValue *params = NULL);     

      virtual ~Application();
      // This does the setup - creating/instantiating workers, 
      // setting initial properties, and making connections
      void initialize();
      // This makes the application operational, and resumes after "stop"
      void start();
      // return true if timed out
      bool wait( unsigned timeout_us = 0);
      // Suspension, that can be resumed with "start".
      void stop();
      ExternalPort &getPort(const char *);
      bool getProperty(unsigned ordinal, std::string &name, std::string &value);
      void getProperty(const char* worker_name, const char* prop_name, std::string &value);
      void setProperty(const char* worker_name, const char* prop_name, const char *value);
    private:
      friend class Property;
      Worker &getPropertyWorker(const char *name);
    };
  }
}

