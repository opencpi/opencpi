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

#include <unistd.h>
#include <CpiDriver.h>
#include <CpiUtilAutoMutex.h>
#include <CpiPValue.h>


// Be very minimal for static constructors
#define W(s) write(2, s, sizeof(s) - 1)

namespace CPI {
  namespace Util {

    Driver::Driver(DriverManager& parent, const char* type, const char *name, bool dis )
      throw()
      :Child<DriverManager,Driver>(parent,name),
       m_type(type),m_name(name),m_isDiscoverable(dis)
    {
      DriverManager::registerDriver(this);
    }

    Driver::Driver(const char* type, const char *name, bool dis) 
      throw()
      : Child<DriverManager,Driver>(name), m_type(type),m_name(name),m_isDiscoverable(dis)
    {
      DriverManager::registerDriver(this);
    }

    void DriverManager::addDriver( Driver* dr ) 
      throw()
    {
      DriverManager::registerDriver(dr);
    }

    DriverManager::DriverManager( const char* type )
      throw()
      :m_type(type)
    {
      CPI::Util::AutoMutex guard ( m_mutex, true ); 
      std::vector<Driver*>::iterator it;
      std::vector<Driver*>&  drivers = getDrivers();
      for (it=drivers.begin(); it!=drivers.end(); it++ ) {
	(*it)->setParent(*this);
      }
    }

    Device * 
    DriverManager::
    getDevice(const CPI::Util::PValue* props, const char *instance ) 
      throw ( CPI::Util::EmbeddedException )
    {
      Device* d=NULL;
      CPI::Util::AutoMutex guard ( m_mutex, true ); 
      std::vector<Driver*>::iterator it;
      std::vector<Driver*>&  drivers = getDrivers();
      for (it=drivers.begin(); it!=drivers.end(); it++ ) {
	if ( (d=(*it)->probe(props,instance)) ) {
	  break;
	}
      }
      return d;
    }


    unsigned
    DriverManager::
    loadDynamicDrivers(const char* dirs, const char* regex_dnames, const char* exlcudes)
      throw ( CPI::Util::EmbeddedException )
    {
      return 0;
    }

    unsigned 
    DriverManager::
    discoverDevices(const PValue* props, const char **exclude) 
      throw ( CPI::Util::EmbeddedException )
    {
      unsigned n = 0;
      CPI::Util::AutoMutex guard ( m_mutex, true ); 
      std::vector<Driver*>::iterator it;
      std::vector<Driver*>&  drivers = getDrivers();
      for (it=drivers.begin(); it!=drivers.end(); it++ ) {
	if ((*it)->m_isDiscoverable) {
	  n += (*it)->search(props, exclude);
	}
      }
      return n;
    }


    void 
    DriverManager::
    registerDriver( Driver* dev) {
	CPI::Util::AutoMutex guard ( m_g_mutex, true ); 
	std::map< std::string,std::vector< Driver* > > & dr_map = getDriverMap();
	std::map<std::string,std::vector< Driver* > >::iterator it =dr_map.find(dev->m_type);
	if ( it != dr_map.end() ) {
	  (*it).second.push_back( dev );
	  return;
	}
	std::vector<Driver*> d;
	d.push_back(dev);
	dr_map[dev->m_type] = d;
      }


    std::vector< Driver* >& 
    DriverManager::
    getDrivers() {
	CPI::Util::AutoMutex guard ( m_g_mutex, true ); 
	std::map< std::string,std::vector< Driver* > > & dr_map = getDriverMap();
	std::map<std::string,std::vector< Driver* > >::iterator it =dr_map.find(m_type);	
	if ( it == dr_map.end() ) {
	  std::vector<Driver*> d;
	  dr_map[m_type] = d;	  
	}
	return dr_map[m_type];
      }


    Device::Device( Driver& parent, const char* instance_name )
      throw ( CPI::Util::EmbeddedException )
	: Child<Driver,Device>(parent, instance_name)
    {
      // Empty
    }


    Driver::~Driver()
      throw()
    {
      // Empty
    };

    CPI::OS::Mutex DriverManager::m_g_mutex;
  }
}
