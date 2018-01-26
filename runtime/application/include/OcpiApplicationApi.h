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
 * The user-level API for applications (via the ACI).
 * Usage is:
 * {
 *    OA::Application app(filename_of_xml_file); // throws error relative to the xml_file
 *    app.initialize();                          // throws errors relative to deploying the app
 *    app.start();
 *    // wait until you are happy with the result...
 * }
 */
#ifndef OCPIAPPLICATIONAPI_H
#define OCPIAPPLICATIONAPI_H
#include "OcpiContainerApi.h"

namespace OCPI {
  namespace API {
    class ApplicationI;
    class ApplicationX;
    class Application {
      friend class ApplicationX;
    protected:
      ApplicationI &m_application;
      Application(ApplicationI &);
    public:
      // The constructor does the planning, deciding what impl will run where
      explicit Application(const char *file, const OCPI::API::PValue *params = NULL);
      explicit Application(const std::string &string, const OCPI::API::PValue *params = NULL);
      // Creates a new Application instance from the app template
      explicit Application(Application &app, const OCPI::API::PValue *params = NULL);

      virtual ~Application();
      // INTERNAL ACCESSOR NOT SUPPORTED FOR API
      const ApplicationI &applicationI() const { return m_application; }
      // This does the setup - creating/instantiating workers,
      // setting initial properties, and making connections
      void initialize();
      // This makes the application operational, and resumes after "stop"
      void start();
      // Set the delayed properties on schedule
      void setDelayedProperties();
      // return true if timed out, unless timeOutIsError, in which case throw exception
      bool wait(unsigned timeout_us = 0, bool timeOutIsError = false);
      // do functional (not cleanup) things at the end of execution
      void finish();
      // Suspension, that can be resumed with "start".
      void stop();
      const std::string &name() const;
      ExternalPort &getPort(const char *, const OCPI::API::PValue *params = NULL);
#if 0
      ExternalPort &getPort(unsigned index, std::string &name);
      size_t getPortCount();
#endif
      bool getProperty(unsigned ordinal, std::string &name, std::string &value,
		       bool hex = false, bool *parameterp = NULL, bool *cachedp = NULL,
		       bool uncached = false);
      // Use top level names or instance:property
      void getProperty(const char* prop_name, std::string &value, bool hex = false);
      void setProperty(const char* prop_name, const char *value);
      // Access properties by instance name and property name
      void getProperty(const char* instance_name, const char* prop_name, std::string &value,
		       bool hex = false);
      void setProperty(const char* instance_name, const char* prop_name, const char *value);
      void dumpDeployment(const char *appFile, const std::string &file);
      void dumpProperties(bool printParameters = true, bool printCached = true,
			  const char *context = NULL) const;
      // setter template without implementation.  we only implement the ones for our types
      template <typename T> void
      setPropertyValue(const char *w, const char *p, const T value,
		       AccessList &list = emptyList) const;
      // Convenience for app-level property, or w.p syntax
      template <typename T> inline void
      setPropertyValue(const char *w, const T value, AccessList &list = emptyList) const {
	setPropertyValue(w, NULL, value, list);
      }
      // Convenience for passing std::string names rather than const char*
      template <typename T> inline void
      setPropertyValue(const std::string &w, const std::string &p, const T value,
		       AccessList &list = emptyList) const {
        setPropertyValue(w.c_str(), p.c_str(), value, list);
      }
      template <typename T> inline void
      setPropertyValue(const std::string &w, const T value, AccessList &list = emptyList) const {
        setPropertyValue(w.c_str(), NULL, value, list);
      }
      // getter templates that require the type at the call site
      // e.g. double d = getPropertyValue<double>("foo");
      // as with string-based getters above, app-level properties do not need the second arg
      // This one will not be implemented for the OA::String type, but will be for std::string
      template <typename T>
      T getPropertyValue(const char *w, const char *p, AccessList &list = emptyList) const;
      template <typename T>
      T getPropertyValue(const char *w, AccessList &list = emptyList) const {
	return getPropertyValue<T>(w, NULL, list);
      }
      // extra convenience allowing std::string names
      template <typename T>
      T getPropertyValue(const std::string &w, const std::string &p, AccessList &list = emptyList) const {
	return getPropertyValue<T>(w.c_str(), p.c_str(), list);
      }
      // compatibility, requires variable at call site, no access list.
      template <typename T>
      void getPropertyValue(const std::string &w, const std::string &p, T &value,
			    AccessList &list = emptyList) const {
	value = getPropertyValue<T>(w.c_str(), p.c_str(), list);
      }
    private:
      friend class Property;
      Worker &getPropertyWorker(const char *name, const char *&pname) const;
    };
  }
}

#endif
