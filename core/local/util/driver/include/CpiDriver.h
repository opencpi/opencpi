// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * Abstact:
 *   This file contains the interface for the CPI Driver class
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */


#ifndef CPI_DRIVER_H
#define CPI_DRIVER_H

#include <string>
#include <vector>
#include <map>
#include <CpiOsMutex.h>
#include <CpiUtilAutoMutex.h>
#include <CpiParentChild.h>
#include <CpiUtilException.h>


namespace CPI {
  namespace Util {

    class PValue;
    class DriverManager;
    class Driver;
    class DriverFactory;
    class Device;


    // All devices must derive from this interface class so that they maintain the correct
    // parent/child relationship
    class Device : public Child<Driver, Device> {
    public:
      Device( Driver& parent, const char* instance_name )
        throw ( CPI::Util::EmbeddedException );

      virtual ~Device()
        throw () {}

    private:
      Device();  // Not implemented
    };

    
    // This is the driver class that is specialized to know how to create devices.  Note that since these objects
    // are often created in the global static space, the contructors are not allowed to throw
    // exceptions.  All exceptions generated in the constructor must be caught and if needed the
    // member m_error must be set and m_lastError should be filled with the error description.
    // It is up to the owner of the DriverManager class for the driver to process the error.
    class Driver  : public Parent<Device>, public Child<DriverManager, Driver> {


    public:
      friend class DriverManager;
      Driver(DriverManager& parent, const char* type, const char *name, bool isDiscoverable)
        throw ();
      Driver(const char* type, const char *name, bool isDiscoverable)
        throw ();

      // See if the device described by these properties exists.
      // This would be called by something that had a configuration file
      virtual Device *probe(const CPI::Util::PValue* props, const char *which )
        throw ( CPI::Util::EmbeddedException ) = 0;

      // Per driver discovery routine to create devices
      virtual unsigned search(const PValue* props, const char **exclude)
        throw ( CPI::Util::EmbeddedException ) = 0;

      inline bool hasException(){return m_error;}
      inline std::string& getErrorString(){return m_lastError;}

      virtual ~Driver()
        throw ();

    private:
      Driver();  // Not Implemented
      bool         m_error;
      std::string  m_lastError;
      std::string m_type;
      std::string m_name;
      bool        m_isDiscoverable;
    };



    // The driver class that knows how to discover, probe and create concrete drivers
    class DriverManager : public Parent<Driver> {

    public:

      DriverManager( const char* driver_type )
        throw ();

      // Load the runtime drivers that match the search criteria
      // dirs         - a colon separated path to look for drivers
      // regex_dnames - a colon separted list of driver names in the form of a regular expression
      // excludes     - a colon separted list of excluded directories
      // The colon separted format is used to directly support environment variables.  Having this
      // interface instead of using "hardcode" environment variable internally allows each manager
      // to use their own variables if needed.
      virtual unsigned loadDynamicDrivers(const char* dirs, const char* regex_dnames, const char* exlcudes)
        throw ( CPI::Util::EmbeddedException );

      // Find me a device among the registered devices, creating if needed, 
      // Fits the pattern when devices are created by specific request
      virtual Device *getDevice(const PValue* props, const char *which )
        throw ( CPI::Util::EmbeddedException );

      // find and create all the drivers there are, returning how many
      // Fits the pattern of "use all the devices available".
      virtual unsigned discoverDevices(const PValue* props, const char **exclude)
        throw ( CPI::Util::EmbeddedException );

      // Add a driver to this manager
      void addDriver( Driver*  )
        throw ();

      // Manages this type
      inline const char* type(){return m_type.c_str();}

      // Register a driver 
      static void registerDriver( Driver* dev);

      // Returns a reference to the list of our drivers
      std::vector< Driver* >& getDrivers();

      virtual ~DriverManager(){}

    private:

      static CPI::OS::Mutex m_g_mutex;
      static std::map< std::string,std::vector< Driver* > >& getDriverMap() {
        static std::map< std::string,std::vector< Driver* > > dr_map;
        return dr_map;
      }


      // Type of devices that we manage
      std::string           m_type;
      CPI::OS::Mutex        m_mutex;

    };

    // This is the class that a driver uses to regsiter itself.  
    // This class is used for life cycle managment.  Since the loader owns this class it does not get deleted until
    // the process exits, however we want the DriverManager class to manage the lifecyle of this class so 
    // that the drivers, and their children (Device's) are deleted at the correct time.
    template <class D> class DriverF{ 
    public:
      DriverF<D>(){new D;}
      DriverF<D>( Parent<DriverManager>& p){new D;}
      };


  } // Container
} // CPI
#endif

