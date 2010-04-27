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

/**
  @file

  @brief
    The CPI::Container::Driver class provides the API for creating concrete Container instances.

  This file contains the declaration of the CPI::Container::Driver class used by an 
  application to create RPL and RCC containers.

  Revision History:

    1/8/2009 - John Miller
                 Initial version.

************************************************************************** */

#ifndef CPI_CONTAINER_FACTORY_H__
#define CPI_CONTAINER_FACTORY_H__

#include <CpiDriver.h>


namespace CPI {

  namespace DataTransport {
    class TransportGlobal;
  }

  namespace Container {

    class Interface;

    /**
      @class Driver

      @brief
        The factory class for creating containers.

	This class is used by an application to create a concrete instance of a 
	container for RCC and RPL workers.  

	Note that all containers created by a factory instance must be deleted before 
	the factory is destroyed.

      @sa CPI::Container::Driver

    ********************************************************************** */

    class Driver : public CPI::Util::Driver  {

    public:

      /**
	 @brief
	 Constructor 

	 @param [ in ] type
	 The type of container factory to construct

	 @param [ in ] polled
	 Tells the factory to contruct a container that will either be event driven or polled       

	 @throw CPI::Util::EmbeddedException  Invalid type requested

	 ****************************************************************** */
      Driver( )
	throw ();


      // See if the device described by these properties exists.
      // This would be called by something that had a configuration file
      CPI::Util::Device *probe(const CPI::Util::PValue* props, const char *which )
	throw ( CPI::Util::EmbeddedException );

      // Per driver discovery routine to create devices
      unsigned search(const CPI::Util::PValue* props, const char **exclude)
	throw ( CPI::Util::EmbeddedException ){return 1;};


      //!< Destructor
      virtual ~Driver()
	throw ( );

    private:
      CPI::DataTransport::TransportGlobal *  m_tpg_events;
      CPI::DataTransport::TransportGlobal *  m_tpg_no_events;

    };
  }
}

#endif

