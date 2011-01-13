
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
   The OCPI::Container::Interface class is used to defined the API for Container implementations.

   Revision History:

   06/15/09 - John Miller
   Added getLastControlError to interface.

   10/13/08 - John Miller
   Initial version.

************************************************************************** */

#ifndef OCPI_CONTAINER_INTERFACE_H_
#define OCPI_CONTAINER_INTERFACE_H_

#include <vector>
#include <OcpiOsDataTypes.h>
#include <OcpiDriver.h>
#include <OcpiParentChild.h>
#include <OcpiContainerDataTypes.h>
#include <OcpiContainerApplication.h>


#include <DtIntEventHandler.h>


namespace OCPI {

  namespace Util {
    class PValue;
  }

  namespace Container {

    class Port;
    class PortData;
    class Application;
    class Artifact;

    /**
       @class Interface

       @brief
       Container Interface class.

       This class provides the interface definition for container implementations.

       @sa OCPI::Container::Interface

       ********************************************************************** */
    class Interface : public OCPI::Util::Parent<Application>, public OCPI::Util::Parent<Artifact>, public OCPI::Util::Device
    {

    public:


      //!< Dispatch thread return codes
      enum DispatchRetCode {
        MoreWorkNeeded,   // Dispatch returned, but there is more work needed
        Spin,             // Dispatch completed it current tasks
        DispatchNoMore,   // No more dispatching required
        Stopped           // Container is stopped
      };

      //!< Default constructor
      Interface(OCPI::Util::Driver &, const char *)
        throw ( OCPI::Util::EmbeddedException );

      /**
         @brief
         Constructor

         @param [ in ] properties
         Container startup properties

         @throw  OCPI::Util::EmbeddedException  If an error is detected during construction

         ****************************************************************** */
      Interface( OCPI::Util::Driver &, const char *, const OCPI::Util::PValue* props )
        throw ( OCPI::Util::EmbeddedException );

      //!< Destructor
      virtual ~Interface()
        throw ( );

      /**
         @brief
         createApplication

         Creates an application  for all subsequent method calls.

         @throw OCPI::Util::EmbeddedException  If an error is detected during the creation of the .

         ****************************************************************** */        
      virtual Application * createApplication()
        throw ( OCPI::Util::EmbeddedException );


      /**
         @brief
         getSupportedEndpoints

         This is the method that gets called by the creator to get tge list of endpoints that this container supports.

         ****************************************************************** */        
      virtual std::vector<std::string> getSupportedEndpoints()
        throw ();


      OCPI::Util::PValue *getProperties();
      OCPI::Util::PValue *getProperty(const char *);


      /**
         @brief
         dispatch

         This is the method that gets called by the creator to provide thread time to the container.  If this method
         returns "true" the caller must continue to call this method.  If the return is "false" the method no longer needs
         to be called.

         @param [ in ] event_manager
         Event Manager object that is associated with this container.  This parameter can be NULL if the container is
         being used in polled mode.

         @throw OCPI::Util::EmbeddedException  If an error is detected during dispatch

         ****************************************************************** */        
      virtual DispatchRetCode dispatch(DataTransfer::EventManager* event_manager)
        throw ( OCPI::Util::EmbeddedException );

      Artifact & loadArtifact(const char *url, OCPI::Util::PValue *artifactParams = 0);
      virtual Artifact & createArtifact(const char *url, OCPI::Util::PValue *artifactParams = 0)=0;


      /**
         @brief
         packPortDesc

         This method is used to "pack" a port descriptor into a string that
         can be sent over a wire.        

         @param [ in ] port
         Port to be packed.

         @retval std::string packed port descriptor

         ****************************************************************** */
      virtual std::string packPortDesc( PortData&  port )
        throw ();


      /**
         @brief
         unpackPortDesc

         This method is used to "unpack" a port descriptor into a Port.


         @param [ in ] desc
         String descriptor previously created with "packPort *".

         @param [ in ] pd
         Unpacked port descriptor.

         @retval bool true if method successful.

         ****************************************************************** */
      virtual PortData * unpackPortDesc( const std::string& desc, PortData* desc_storage )
        throw ();
      virtual int portDescSize();

     
      //!< Start/Stop the container
      virtual void start(DataTransfer::EventManager* event_manager)
        throw();
      virtual void stop(DataTransfer::EventManager* event_manager)
        throw();

      //! get the event manager for this container
      virtual DataTransfer::EventManager*  getEventManager(){return NULL;}

      bool hasName(const char *name);

      inline OCPI::OS::uint32_t getId(){return m_ourUID;}

    protected:
      const std::string m_name;

      //! This containers unique id
      OCPI::OS::uint32_t m_ourUID;

      // Start/Stop flag for this container
      bool m_start;

    };

    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline OCPI::Container::Interface::DispatchRetCode Interface::dispatch(DataTransfer::EventManager* event_manager)
      throw ( OCPI::Util::EmbeddedException ) 
    { 
      (void) event_manager;
       return OCPI::Container::Interface::DispatchNoMore;
    }
  }
}

#endif

