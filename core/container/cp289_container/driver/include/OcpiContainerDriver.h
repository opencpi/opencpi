
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


/**
  @file

  @brief
    The OCPI::Container::Driver class provides the API for creating concrete Container instances.

  This file contains the declaration of the OCPI::Container::Driver class used by an 
  application to create RPL and RCC containers.

  Revision History:

    1/8/2009 - John Miller
                 Initial version.

************************************************************************** */

#ifndef OCPI_CONTAINER_FACTORY_H__
#define OCPI_CONTAINER_FACTORY_H__

#include <OcpiDriver.h>


namespace OCPI {

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

      @sa OCPI::Container::Driver

    ********************************************************************** */

    class Driver : public OCPI::Util::Driver  {

    public:

      /**
         @brief
         Constructor 

         @param [ in ] type
         The type of container factory to construct

         @param [ in ] polled
         Tells the factory to contruct a container that will either be event driven or polled       

         @throw OCPI::Util::EmbeddedException  Invalid type requested

         ****************************************************************** */
      Driver( )
        throw ();


      // See if the device described by these properties exists.
      // This would be called by something that had a configuration file
      OCPI::Util::Device *probe(const OCPI::Util::PValue* props, const char *which )
        throw ( OCPI::Util::EmbeddedException );

      // Per driver discovery routine to create devices
      unsigned search(const OCPI::Util::PValue* props, const char **exclude)
        throw ( OCPI::Util::EmbeddedException ){( void ) props; ( void ) exclude; return 1;};


      //!< Destructor
      virtual ~Driver()
        throw ( );

    private:
      OCPI::DataTransport::TransportGlobal *  m_tpg_events;
      OCPI::DataTransport::TransportGlobal *  m_tpg_no_events;

    };
  }
}

#endif

