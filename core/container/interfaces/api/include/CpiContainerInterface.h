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
   The CPI::Container::Interface class is used to defined the API for Container implementations.

   Revision History:

   06/15/09 - John Miller
   Added getLastControlError to interface.

   10/13/08 - John Miller
   Initial version.

   ************************************************************************** */

#ifndef CPI_CONTAINER_INTERFACE_H_
#define CPI_CONTAINER_INTERFACE_H_

#include <vector>
#include <CpiOsDataTypes.h>
#include <CpiDriver.h>
#include <CpiParentChild.h>
#include <CpiContainerDataTypes.h>
#include <CpiApplication.h>


#include <DtIntEventHandler.h>


namespace CPI {

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

       @sa CPI::Container::Interface

       ********************************************************************** */
    class Interface : public CPI::Util::Parent<Application>, public CPI::Util::Parent<Artifact>, public CPI::Util::Device
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
      Interface(CPI::Util::Driver &, const char *)
	throw ( CPI::Util::EmbeddedException );

      /**
	 @brief
	 Constructor

	 @param [ in ] properties
	 Container startup properties

	 @throw  CPI::Util::EmbeddedException  If an error is detected during construction

	 ****************************************************************** */
      Interface( CPI::Util::Driver &, const char *, const CPI::Util::PValue* props )
	throw ( CPI::Util::EmbeddedException );

      //!< Destructor
      virtual ~Interface()
	throw ( );

      /**
	 @brief
	 createApplication

	 Creates an application  for all subsequent method calls.

	 @throw CPI::Util::EmbeddedException  If an error is detected during the creation of the .

	 ****************************************************************** */	
      virtual Application * createApplication()
	throw ( CPI::Util::EmbeddedException )=0;


      /**
	 @brief
	 getSupportedEndpoints

	 This is the method that gets called by the creator to get tge list of endpoints that this container supports.

	 ****************************************************************** */	
      virtual std::vector<std::string> getSupportedEndpoints()
	throw ();


      CPI::Util::PValue *getProperties();
      CPI::Util::PValue *getProperty(const char *);


      /**
	 @brief
	 dispatch

	 This is the method that gets called by the creator to provide thread time to the container.  If this method
	 returns "true" the caller must continue to call this method.  If the return is "false" the method no longer needs
	 to be called.

	 @param [ in ] event_manager
	 Event Manager object that is associated with this container.  This parameter can be NULL if the container is
	 being used in polled mode.

	 @throw CPI::Util::EmbeddedException  If an error is detected during dispatch

	 ****************************************************************** */	
      virtual DispatchRetCode dispatch(DataTransfer::EventManager* event_manager)
	throw ( CPI::Util::EmbeddedException );


#ifdef NEEDED
      /**
	 @brief
	 loadArtifacts

	 Loads a worker artifact that can later be used to create a worker instance.

	 @param [ in ] app
	 Application.

	 @param [ in ] artifactData
	 Array of artifact information.

	 @param [ in ] artifactCount
         Number of elements in artifactData.

	 @throw CPI::Util::EmbeddedException  If an error is detected

	 ****************************************************************** */	
      virtual void loadArtifacts( 
				 Application&   app,	       
				 CPI::Util::PValue*    artifactData,	
				 CPI::OS::uint32_t     artifactCount 
				  )
	throw ( CPI::Util::EmbeddedException ){};
#endif


      virtual Artifact & loadArtifact(const char *url, CPI::Util::PValue *artifactParams = 0);



#ifdef NEEDED

      /**
	 @brief
	 unLoadArtifacts

	 Unloads all previously loaded artifacts withing the context.

	 @param [ in ] app
	 Application.

	 @throw CPI::Util::EmbeddedException  If the unload operation fails.

	 ****************************************************************** */	
      virtual void unLoadArtifacts( 
				   Application& app 
				    )
	throw ( CPI::Util::EmbeddedException ){}
#endif


		

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

      inline CPI::OS::uint32_t getId(){return m_ourUID;}

    protected:
      const std::string m_name;

      //! This containers unique id
      CPI::OS::uint32_t m_ourUID;

      // Start/Stop flag for this container
      bool m_start;

    };

    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline CPI::Container::Interface::DispatchRetCode Interface::dispatch(DataTransfer::EventManager* event_manager)
      throw ( CPI::Util::EmbeddedException ) {return CPI::Container::Interface::DispatchNoMore;}

  }
}

#endif

