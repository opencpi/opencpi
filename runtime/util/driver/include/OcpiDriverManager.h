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
 *   This file defines classes and templates for managers, drivers and devices
 *   The parent-child stack is:  ManagerManager->Manager->Driver->Device.
 *   ManagerManager, Manager, and Driver classes are dynamically constructed singletons.
 *   The ManagerManager is purely generic and thus concrete.
 *   Managers are specialized (as the manager of all FOO drivers) by inheriting the Manager template,
 *   which specifies the derived class of drivers that the manager manages.
 *   Drivers are specialized (as driver for all ABC devices) by inheriting the Driver template,
 *   which specifies the derived class of devices it supports.
 *
 *   This all relies on the OCPI::Util::Parent/Child templates.
 *     
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 2/2011
 *    Revision Detail: Created
 *
 */
#ifndef OCPI_DRIVER_MANAGER_H
#define OCPI_DRIVER_MANAGER_H
#include "OcpiOsMutex.h"
#include "OcpiParentChild.h"
#include "OcpiPValue.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h" // Singleton
#include "ezxml.h"

namespace OCPI {
  namespace Driver {
    using OCPI::Util::Child;
    //    using OCPI::Util::ChildWithBase;
    using OCPI::Util::Sibling;
    using OCPI::Util::Parent;
    using OCPI::Util::PValue;
    class Manager;
    // The concrete owner of all driver managers, not inherited.
    // Created on demand when the first manager is created
    // It doesn't ever need to know or access the derived type of its
    // children (the various specialized driver managers)
    class ManagerManager : public Parent<Manager> {
      OCPI::OS::Mutex m_mutex;  // for thread-safe configuration
      std::string m_configFile; // system configuration file
      bool m_configured;        // to do lazy (and avoid redundant) configuration
      ezxml_t m_xml;
      static bool s_exiting;
    public:
      ManagerManager();
      ~ManagerManager();
      static bool exiting() { return s_exiting; }
      void configureOnce(const char *cf = NULL, const OCPI::Util::PValue *params = NULL);
      // This is public only for debugging, etc.
      static ManagerManager &getManagerManager();
      // Use this method to do early what will be done anyway at static destruction
      static void cleanup();
      // Use this method to ensure first-time configuration AFTER static construction
      static void configure(const char *file = NULL);
      // Report configuration errors
      static void configError(ezxml_t x, const char *fmt,...);
      // Global suppression of discovery
      static void suppressDiscovery();
      // Load a specific driver
      static bool loadDriver(const char *managerName, const char *drvrName, std::string &error);
    };
    // The base class for all (singleton) driver managers which are children of
    // ManagerManager. This is NOT directly inherited by derived managers. They
    // inherit the ManagerBase template class below
    class Driver;
    class Manager : public Child<ManagerManager,Manager> {
      friend class ManagerManager;
    protected:
      bool m_doNotDiscover;
      Manager(const char *mname)
	: Child<ManagerManager,Manager>
	  (OCPI::Util::Singleton<ManagerManager>::getSingleton(), *this, mname),
	  m_doNotDiscover(false)
      {}
      // Configure the manager. X is the node whose name is the name of the manager
      // The default implementation just configures any drivers
      virtual void configure(ezxml_t x);
      // Discover all devices on all drivers, return device count.
      virtual Driver *firstDriverBase() = 0;
      virtual unsigned cleanupPosition();
      bool shouldDiscover() const { return !m_doNotDiscover; }
    public:
      virtual unsigned discover(const OCPI::Util::PValue *params = NULL) = 0;
      inline void suppressDiscovery() { m_doNotDiscover = true; }
      inline void enableDiscovery() { m_doNotDiscover = false; }
      virtual ~Manager();
    };
    class Device;
    // The base class for all (singleton) drivers.
    class Driver {
      ezxml_t m_config;
    protected:
      Driver();
      virtual ~Driver();
      ezxml_t getDeviceConfig(const char *name);
    public:
      virtual const std::string &name() const = 0;
      virtual Driver *nextDriverBase() = 0;
      virtual void configure(ezxml_t);
      virtual Device *firstDeviceBase() = 0;
    };
    // The template class inherited by concrete managers, with the template
    // parameter specifying the derived driver class managed by the
    // inheriting manager.
    // Mgr is the inheriting class.
    // DerivedDriver is the derived driver class inherited by all drivers
    //    under this manager
    // mname is the manager name
    template <class Mgr, class DerivedDriver, const char *&mname>
      class ManagerBase
      : public Parent<DerivedDriver>,  // This manager manages these drivers, generically
        public OCPI::Util::Singleton<Mgr>,
        public Manager
    {
    protected:
      ManagerBase<Mgr, DerivedDriver, mname>()
      : Manager(mname)
      {
      }
      DerivedDriver *firstDriver() {
	return Parent<DerivedDriver>::firstChild();
      }
      Driver *firstDriverBase() {
	return firstDriver();
      }
    public:
      unsigned discover(const OCPI::Util::PValue *params) {
	parent().configureOnce();
	unsigned found  = 0;
	ocpiInfo("Performing discovery for all %s drivers", name().c_str());
	if (!m_doNotDiscover)
	  for (DerivedDriver *dd = firstDriver(); dd; dd = dd->nextDriver())
	    dd->search(params, NULL, false);
	return found;
      }
    };
    // This class is virtually inherited, so the constructor can't take args,
    // and the destructor will be called very late.
    class Device {
      friend class Driver;
      virtual Device *nextDeviceBase() = 0;
    protected:
      virtual void configure(ezxml_t x);
      virtual ~Device();
    public:
      virtual const std::string &name() const = 0;
    };
    // This template class will be inherited by driver base (type) classes
    // i.e. the class that is inherited by all the concrete drivers under
    // a particular concrete manager
    // The concrete manager will want to see all its children as DriBase
    // objects.
    // The argument is the class that inherits this class
    template <class DriMgr, class DriBase>
    class DriverType : public Child<DriMgr,DriBase>, public Driver {
    protected:
      DriverType(const char *a_name, DriBase &d)
	: Child<DriMgr,DriBase>(DriMgr::getSingleton(), d, a_name)
      {
	ocpiInfo("Registering/constructing %s driver: %s", 
		 Child<DriMgr,DriBase>::parent().name().c_str(), a_name);
      }
    public:
      // Configure from system configuration XML
      //      virtual void configure(ezxml_t );//{}
      // Per driver discovery routine to create devices that are found,
      // excluding the ones named in the "exclude" list.
      virtual unsigned search(const PValue* props = NULL, const char **exclude = NULL,
			      bool discoveryOnly = false) {
	(void) props; (void) exclude; (void) discoveryOnly;
	return 0;
      }
      // Probe for a particular device and return it if found, and creating it
      // if not yet created. Return NULL if it is not found.
      // This would typically be called by something that had a configuration file
      // and didn't want "discovery" via search.
      // If "which" is null, return any one that matches props, otherwise
      // request a specific one.
      virtual Device *probe(const char *which = NULL, const char *processor = NULL,
			    const char *mach = NULL) {
	(void)which; (void)processor; (void)mach;
	return NULL;
      };
      DriBase *nextDriver() { return Sibling<DriBase>::nextChild(); }
      inline Driver *nextDriverBase() { return nextDriver(); }
      inline const std::string &name() const {
	return Child<DriMgr,DriBase>::name();
      }
    protected:
      virtual ~DriverType(){};
    };

    // The template class that concrete drivers should inherit from, with the
    // template parameters being:
    // 1. the derived driver manager class (e.g. Container::Manager)
    // 2. the derived driver base class inherited by the concrete driver 
    //    (e.g. Container::Driver)
    // 3. the concrete device class for this concrete driver inheriting this 
    //    template  (e.g.RCC::Container)
    //    this is the class of device that the concrete driver knows how to handle
    template <class Man, class DriBase, class ConcDri, class Dev, const char *&name>
    class DriverBase
      : public Parent<Dev>,
        public OCPI::Util::Singleton<ConcDri>,
	public DriBase { // destroy this class BEFORE children 
    public:
      // to access a specific driver
      inline static ConcDri &getDriver() {
	return OCPI::Util::Singleton<ConcDri>::getSingleton();
      }
      // The language does not allow this to be protected with "friend class Man".
      // inline DriBase *nextDriver() { return *Sibling<DriBase>::nextChildP();}
      inline Dev *firstDevice() {
	return static_cast<Dev*>(Parent<Dev>::firstChild());
      }
      inline Device *firstDeviceBase() { return firstDevice(); }
      inline Dev *findDevice(const char *dname) {
	return Parent<Dev>::findChildByName(dname);
      }
    protected:
      // This is the constructor that is called at static construction time.
      DriverBase<Man, DriBase, ConcDri, Dev, name>()
      : DriBase(name) {
        ocpiInfo("Registering/constructing driver: %s", name);
      }
    };
    // The template that concrete drivers should use to register themselves at
    // static construction time.  The template parameter is the concrete driver class.
    // Drivers should include the line:
    // static OCPI::Util::Driver::Registration<concrete-driver> x;

    // FIXME: this is actually generic and could be elsewhere in some utility
    extern void Registration_debug_hook();
    template <class D> class Registration {
    public:
      // This constructor will run at static construction time,
      // the driver object itself will be dynamically constructed at that time
      // and implicitly registered with its parent, so there is nothing to do
      // at static destruction time.
      Registration<D>() { Registration_debug_hook(); OCPI::Util::Singleton<D>::getSingleton();}
    };
    // This device template takes the concrete driver and concrete device class
    // as template arguments, and also obtains its parent from the known driver
    // class.  The generic Device class is virtually inherited so that
    // it can be accessed from any manager-specific Device base classes.
    extern const char *device;
    template <class Dri, class Dev>
    class DeviceBase: public Child<Dri,Dev,device>, virtual public Device {
    public:
      inline Dev *nextDevice() { return Child<Dri,Dev,device>::nextChild(); }
      inline Device *nextDeviceBase() { return nextDevice(); }
      inline Dri &driver() { return Child<Dri,Dev,device>::parent(); }
      const std::string &name() const {
	return Child<Dri,Dev,device>::name();
      }
    protected:
      DeviceBase<Dri, Dev>(const char *childName, Dev &dev)
      : Child<Dri, Dev, device>(OCPI::Util::Singleton<Dri>::getSingleton(), dev, childName)
      {}
    };
  }
}
#endif
