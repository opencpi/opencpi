
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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
  @brief
  This file contains the implementation for the OpenCPI OpenCL (OCL)
  container.

************************************************************************** */

#include <climits>
#include "OCL_Worker.h"
#include "OcpiWorker.h"
#include "OcpiOsTimer.h"
#include "OcpiContainerMisc.h"
#include "OcpiContainerManager.h"
#include "OcpiOclPlatformManager.h"

namespace OCPI
{
  namespace OCL
  {
    namespace OS = OCPI::OS;
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OM = OCPI::Metadata;
    namespace OC = OCPI::Container;

    const size_t OCLDP_LOCAL_BUFFER_ALIGN ( 16 );

    typedef struct
    {
      uint32_t length;
      uint32_t opcode;
      uint32_t tag;
      uint32_t interval;

    } OclDpMetadata;

    namespace
    {
#if 0
      void printDesc ( const OCPI::RDT::Desc_t& desc,
                       const char* msg )
      {
        printf ( "RDT Descriptor: %s\n"
                 "  Address            = %p\n"
                 "  Number of Buffers  = %u\n"
                 "  DataBufferBaseAddr = 0x%llx\n"
                 "  DataBufferSize     = %u\n"
                 "  DataBufferPitch    = %u\n"
                 "  MetaDataBaseAddr   = 0x%llx\n"
                 "  MetaDataPitch      = %u\n"
                 "  FullFlagBaseAddr   = 0x%llx\n"
                 "  FullFlagPitch      = %u\n"
                 "  FullFlagSize       = %u\n"
                 "  FullFlagValue      = 0x%llx\n"
                 "  EmptyFlagBaseAddr  = 0x%llx\n"
                 "  EmptyFlagPitch     = %u\n"
                 "  EmptyFlagSize      = %u\n"
                 "  EmptyFlagValue     = 0x%llx\n"
                 "  OOB port ID        = %llu\n"
                 "  OOB OEP            = %s\n",
                 msg,
                 (void*)&desc,
                 desc.nBuffers,
                 (long long)desc.dataBufferBaseAddr,
                 desc.dataBufferPitch,
                 desc.dataBufferSize,
                 (long long)desc.metaDataBaseAddr,
                 desc.metaDataPitch,
                 (long long)desc.fullFlagBaseAddr,
                 desc.fullFlagPitch,
                 desc.fullFlagSize,
                 (long long)desc.fullFlagValue,
                 (long long)desc.emptyFlagBaseAddr,
                 desc.emptyFlagPitch,
                 desc.emptyFlagSize,
                 (long long)desc.emptyFlagValue,
                 (long long)desc.oob.port_id,
                 desc.oob.oep );
      }
#endif

      typedef enum
      {
        OCPI_OCL_INITIALIZE = 0,
        OCPI_OCL_START,
        OCPI_OCL_STOP,
        OCPI_OCL_RELEASE,
        OCPI_OCL_BEFORE_QUERY,
        OCPI_OCL_AFTER_CONFIGURE,
        OCPI_OCL_TEST,
        OCPI_OCL_RUN

      } OcpiOclOpcodes_t;

      OcpiOclOpcodes_t controlOp2Opcode ( OM::Worker::ControlOperation op )
      {
        switch ( op )
        {
          case OM::Worker::OpInitialize:
            return OCPI_OCL_INITIALIZE;
          case OM::Worker::OpStart:
            return OCPI_OCL_START;
          case OM::Worker::OpStop:
            return OCPI_OCL_STOP;
          case OM::Worker::OpRelease:
            return OCPI_OCL_RELEASE;
          case OM::Worker::OpTest:
            return OCPI_OCL_TEST;
          case OM::Worker::OpBeforeQuery:
            return OCPI_OCL_BEFORE_QUERY;
          case OM::Worker::OpAfterConfigure:
            return OCPI_OCL_AFTER_CONFIGURE;
          default:
            return OCPI_OCL_RUN;
        }

        return OCPI_OCL_RUN;
      }

    } // End: namespace<unamed>

    class Container;

    class ExternalPort;

    const char *ocl = "ocl";

    class Driver : public OC::DriverBase<Driver, Container, ocl>
    {
      friend class ExternalPort;

      public:
        unsigned search ( const OA::PValue*,
                          const char** exclude, bool discoveryOnly );

        OC::Container* probeContainer ( const char* which,
					std::string &error,
                                        const OA::PValue* props );
    }; // End: class Driver

    OC::RegisterContainerDriver<Driver> driver;

    class Port;
    class Artifact;
    class Application;

    class Container : public OC::ContainerBase<Driver, Container, Application, Artifact>
    {
        friend class Port;
        friend class Driver;
        friend class Artifact;

      private:
        DeviceContext* d_device;

      protected:
        Container ( const char* name,
                    DeviceContext* device,
                    const ezxml_t config = NULL,
                    const OU::PValue* props = NULL )
          : OC::ContainerBase<Driver,Container,Application,Artifact>(*this, name, config, props),
            d_device ( device )
        {
	  m_model = "ocl";
        }

      public:
        ~Container ()
        {
          OC::Container::shutdown();
          this->lock();
          OU::Parent<Application>::deleteChildren();
        }

        OC::Container::DispatchRetCode dispatch ( DataTransfer::EventManager* event_manager )
        throw ( OU::EmbeddedException );

        OC::Artifact& createArtifact ( OCPI::Library::Artifact& lart,
                                       const OA::PValue* artifactParams );

        OA::ContainerApplication* createApplication ( const char* name,
                                                      const OCPI::Util::PValue* props )
        throw ( OCPI::Util::EmbeddedException );

        bool needThread ()
        {
          return true;
        }

        DeviceContext& device ( )
        {
          return *d_device;
        }

        void loadArtifact ( const std::string& pathToArtifact,
                            const OA::PValue* artifactParams )
        {
          device().loadArtifact ( pathToArtifact, artifactParams );
        }

        void unloadArtifact ( const std::string& pathToArtifact )
        {
          device().unloadArtifact ( pathToArtifact );
        }

    }; // End: class Container

    unsigned Driver::search ( const OA::PValue* /* params */, const char** /* exclude */,
			      bool /*discoveryOnly*/ )
    {
      return 1;
    }

    // FIXME: leak of DeviceContext on constructor error
    // FIXME: return errors, don't throw.
    OC::Container* Driver::probeContainer ( const char* which, std::string &/*error*/,
                                            const OA::PValue* props )
    {
      return new Container ( which, new DeviceContext ( props ), 0, props );
    }

    class Artifact : public OC::ArtifactBase<Container,Artifact>
    {
      friend class Container;

      private:
        Artifact ( Container& c,
                   OCPI::Library::Artifact& lart,
                   const OA::PValue* artifactParams )
          : OC::ArtifactBase<Container,Artifact> ( c, *this, lart, artifactParams )
        {
          printf ( "OCL loading OpenCL artifact %s into OCL container %s\n",
                   name().c_str(),
                   c.name().c_str() );
          c.loadArtifact ( name(), artifactParams );
        }

      public:
        ~Artifact()
        {
          parent().unloadArtifact ( name() );
        }
    }; // End: class Artifact

    OC::Artifact& Container::createArtifact ( OCPI::Library::Artifact& lart,
                                              const OA::PValue* artifactParams )
    {
      return *new Artifact ( *this, lart, artifactParams );
    }

    class Worker;

    class Application : public OC::ApplicationBase<Container,Application,Worker>
    {
      friend class Container;

      private:
        Application ( Container& con,
                      const char* name,
                      const OA::PValue* props )
          : OC::ApplicationBase<Container, Application, Worker> ( con, *this, name, props )
        {
          // Empty
        }

        OC::Worker& createWorker ( OC::Artifact* art,
                                   const char* appInstName,
                                   ezxml_t impl,
                                   ezxml_t inst,
                                   const OCPI::Util::PValue* wParams );

        void run ( DataTransfer::EventManager* event_manager,
                   bool& more_to_do );

      public:
        DeviceWorker& loadWorker ( const std::string& entryPoint )
        {
          return parent().device().loadWorker ( entryPoint );
        }

        void unloadWorker ( DeviceWorker& worker )
        {
          parent().device().unloadWorker ( worker );
        }

    }; // End: class Application

    OA::ContainerApplication* Container::createApplication ( const char* name,
                                                             const OCPI::Util::PValue* props )
    throw ( OCPI::Util::EmbeddedException )
    {
      return new Application ( *this, name, props );
    }

    OC::Container::DispatchRetCode Container::dispatch ( DataTransfer::EventManager* event_manager )
    throw ( OU::EmbeddedException )
    {
      bool more_to_do = false;
      try
      {
        if ( !m_enabled )
        {
          return Stopped;
        }

        OU::SelfAutoMutex guard ( this );

        // Process the workers
        for ( Application* a = OU::Parent<Application>::firstChild();
              a;
              a = a->nextChild() )
        {
          a->run ( event_manager, more_to_do );
        }
      }
      catch ( const OCPI::Util::EmbeddedException& e )
      {
        std::cerr << "\nException(e): " << e.getAuxInfo ( ) << std::endl;
      }
      catch ( const OCPI::API::Error& e )
      {
        std::cerr << "\nException(a): " << e.error ( ) << std::endl;
      }
      catch ( const std::string& s )
      {
        std::cerr << "\nException(s): " << s << std::endl;
      }
      catch ( ... )
      {
        std::cerr << "\nException(u): unknown" << std::endl;
      }

      return more_to_do ? MoreWorkNeeded : Spin;
    }

    class Worker : public OC::WorkerBase<Application,Worker,Port>
    {
      friend class Port;
      friend class Application;
      friend class ExternalPort;
      friend class ExternalBuffer;

      private:
        struct OCLPortInternal
        {
          OCLBuffer current;
          OCLPortAttr* attr;
          uint32_t dataValueWidthInBytes;
        };

        bool isEnabled;
        Container& myContainer;
        const char* implName;
        OCPI::Metadata::Worker metadataImpl;
        const char* instName;
        OCPI::Metadata::Worker metadataInst;
        std::string myEntryPoint;
        uint8_t* myProperties;
        uint32_t nConnectedPorts;
        OCLPortInternal* myPorts;
        uint32_t readyPorts;
        OCLResult* myResult;
        OCLBoolean timedOut;
        OCLBoolean* myNewRunCondition;
        OCLRunCondition* myRunCondition;
        uint8_t* dummyBuffer; // Port buffer placed holder for control ops
        DeviceWorker& device_worker;
        OCPI::OS::Timer runTimer;
        std::vector<void*> myLocalMemories;

        Worker ( Application& app,
                 OC::Artifact* art,
                 const char* name,
                 ezxml_t implXml,
                 ezxml_t instXml,
                 const OA::PValue* execParams )
	  : OC::WorkerBase<Application, Worker, Port> ( app, *this, art, name, implXml, instXml, execParams ),
          isEnabled ( false ),
          myContainer ( app.parent() ),
          implName ( ezxml_attr ( implXml, "name" ) ),
          metadataImpl ( implXml ),
          instName ( ezxml_attr ( instXml, "name" ) ),
          metadataInst ( instXml ),
          myEntryPoint ( std::string ( implName ) + std::string ( "_entry_point" ) ),
          myProperties ( 0 ),
          nConnectedPorts ( 0 ),
          myPorts ( 0 ),
          readyPorts ( 0 ),
          myResult ( 0 ),
          timedOut ( false ),
          myRunCondition ( 0 ),
          dummyBuffer ( 0 ),
          device_worker ( parent().loadWorker ( myEntryPoint.c_str ( ) ) ),
          runTimer (),
          myLocalMemories ( )

        {
          initializeContext ( );

          setControlOperations ( ezxml_cattr ( implXml, "controlOperations" ) );

          setControlMask ( getControlMask() | ( 1 << OM::Worker::OpStart ) );
        }

        void updatePortsPreRun ( );

        void updatePortsPostRun ( );

        void advancedPortBuffers ( );

        void kernelProlog ( OcpiOclOpcodes_t opcode )
        {
          size_t arg ( 0 );

          device_worker.setKernelArg ( arg++, sizeof ( opcode ), &opcode );
          device_worker.setKernelArg ( arg++, sizeof ( timedOut ), &timedOut );

          device_worker.setKernelArg ( arg++, myProperties ); // ptr
          device_worker.syncPtr ( myProperties,
                                  OCPI::OCL::DeviceWorker::HOST_TO_DEVICE );

          device_worker.setKernelArg ( arg++, myRunCondition ); // ptr
          device_worker.syncPtr ( myRunCondition,
                                  OCPI::OCL::DeviceWorker::HOST_TO_DEVICE );

          device_worker.setKernelArg ( arg++, myNewRunCondition ); // ptr
          device_worker.syncPtr ( myNewRunCondition,
                                  OCPI::OCL::DeviceWorker::HOST_TO_DEVICE );

          for ( size_t n = 0; n < myLocalMemories.size ( ); n++ )
          {
            device_worker.setKernelArg ( arg++, myLocalMemories [ n ] ); // ptr
            // No need to call syncPtr. Host does not touch "local memory"
          }

          size_t n_ports = metadataImpl.getNumPorts ( );

          for ( size_t n = 0; n < n_ports; n++ )
          {
              bool connected = myPorts [ n ].attr->connected;

              if ( opcode != OCPI_OCL_RUN )
              {
                connected = true;
                myPorts [ n ].current.data = dummyBuffer;
              }

              if ( connected && myPorts [ n ].current.data  )
              {
                // ptr - uses map/unmap - no need to sync data buffer
                device_worker.setKernelArg ( arg++, myPorts [ n ].current.data );

                device_worker.setKernelArg ( arg++,
                                             sizeof ( myPorts [ n ].current.maxLength ),
                                             &myPorts [ n ].current.maxLength );

                device_worker.setKernelArg ( arg++, myPorts [ n ].attr ); // ptr
                device_worker.syncPtr ( myPorts [ n ].attr,
                                        OCPI::OCL::DeviceWorker::HOST_TO_DEVICE );
              }
          }
          *myResult = OCL_OK;
          device_worker.setKernelArg ( arg++, myResult ); // ptr
          device_worker.syncPtr ( myResult,
                                  OCPI::OCL::DeviceWorker::HOST_TO_DEVICE );
        }

        void kernelEpilog ( )
        {
          device_worker.syncPtr ( myResult,
                                  OCPI::OCL::DeviceWorker::DEVICE_TO_HOST );
          device_worker.syncPtr ( myProperties,
                                  OCPI::OCL::DeviceWorker::DEVICE_TO_HOST );
          device_worker.syncPtr ( myRunCondition,
                                  OCPI::OCL::DeviceWorker::DEVICE_TO_HOST );
          device_worker.syncPtr ( myNewRunCondition,
                                  OCPI::OCL::DeviceWorker::DEVICE_TO_HOST );

          for ( size_t n = 0; n < metadataImpl.getNumPorts ( ); n++ )
          {
            if ( myPorts [ n ].attr->connected && myPorts [ n ].current.data )
            {
              device_worker.syncPtr ( myPorts [ n ].attr,
                                      OCPI::OCL::DeviceWorker::DEVICE_TO_HOST );
              // No need to sync myPorts [ n ].current.data map/unmap is used
            }
            myPorts [ n ].current.data = 0;
          }
       }

        void initializeContext ( )
        {
          uint32_t n_ports = metadataImpl.getNumPorts ( );

          myRunCondition = ( OCLRunCondition* ) calloc ( 1,  sizeof ( OCLRunCondition ) );

          device_worker.registerPtr ( (void*)myRunCondition,
                                      sizeof ( OCLRunCondition ) );

          // Default run condtion for ports
          myRunCondition->portMasks = 0;
          for ( uint32_t n = 0; n < n_ports; n++ )
          {
            myRunCondition->portMasks |=  ( 1 << n );
          }

          myNewRunCondition = ( OCLBoolean* ) calloc ( 1,  sizeof ( OCLBoolean ) );

          device_worker.registerPtr ( (void*)myNewRunCondition,
                                      sizeof ( OCLBoolean ) );

          dummyBuffer = ( uint8_t* ) calloc ( 1, sizeof ( uint32_t ) );

          if ( !dummyBuffer )
          {
            throw OC::ApiError( "OCL failed to allocate worker dummy buffer." );
          }

          device_worker.registerPtr ( (void*)dummyBuffer, sizeof ( uint32_t ) );

          myPorts = ( OCLPortInternal* ) calloc ( n_ports, sizeof ( OCLPortInternal ) );

          if ( !myPorts )
          {
            throw OC::ApiError( "OCL failed to allocate worker ports." );
          }

          for ( size_t n = 0; n < n_ports; n++ )
          {
            myPorts [ n ].attr = ( OCLPortAttr*) calloc ( 1, sizeof ( OCLPortAttr ) );
            if ( !myPorts [ n ].attr )
            {
              throw OC::ApiError( "OCL failed to allocate worker port attributes." );
            }
            device_worker.registerPtr ( (void*)myPorts [ n ].attr,
                                        sizeof ( OCLPortAttr ) );
          }

          myProperties = ( uint8_t* ) calloc ( metadataImpl.getPropertySize( ) + 4,
                                               sizeof ( uint8_t ) );
          if ( !myProperties )
          {
            throw OC::ApiError( "OCL failed to allocate worker properties." );
          }
          device_worker.registerPtr ( (void*)myProperties,
                                      metadataImpl.getPropertySize( ) + 4 );

          myResult = ( OCLResult* ) calloc ( 1, sizeof ( OCLResult ) );

          if ( !myResult )
          {
            throw OC::ApiError( "OCL failed to allocate worker result." );
          }
          device_worker.registerPtr ( (void*)myResult, sizeof ( OCLResult ) );

          unsigned int nLocalMemories;
          OM::LocalMemory* local_memories =
                         metadataImpl.getLocalMemories ( nLocalMemories );
          if ( nLocalMemories )
          {
            for ( size_t n = 0; n < nLocalMemories; n++ )
            {
              void* p = calloc ( local_memories [ n ].n_bytes, sizeof ( uint8_t ) );
              if ( !p )
              {
                throw OC::ApiError( "OCL failed to allocate local memory." );
              }
              myLocalMemories.push_back ( p );
              device_worker.registerPtr ( p, local_memories [ n ].n_bytes );
            }
          }
        }

        void finalizeContext ( )
        {
          for ( size_t n = 0; n < metadataImpl.getNumPorts ( ); n++ )
          {
            device_worker.unregisterPtr ( myPorts [ n ].attr );
            free ( myPorts [ n ].attr );
          }
          device_worker.unregisterPtr ( myResult );
          device_worker.unregisterPtr ( myProperties );
          device_worker.unregisterPtr ( myRunCondition );
          device_worker.unregisterPtr ( myNewRunCondition );
          device_worker.unregisterPtr ( dummyBuffer );

          for ( size_t n = 0; n < myLocalMemories.size ( ); n++ )
          {
            device_worker.unregisterPtr ( myLocalMemories [ n ] );
            free ( myLocalMemories [ n ] );
          }

          free ( myPorts );
          free ( myResult );
          free ( myProperties );
          free ( myRunCondition );
          free ( myNewRunCondition );
          free ( dummyBuffer );
        }

      public:
        ~Worker()
        {
          try
          {
            if ( isEnabled )
            {
              controlOperation ( OM::Worker::OpStop );
              controlOperation ( OM::Worker::OpRelease );
              isEnabled = false;
            }

            finalizeContext ( );
            parent().unloadWorker ( device_worker );
          }
          catch ( const std::string& s )
          {
            std::cout << "Exception(s): " << __PRETTY_FUNCTION__ << s << std::endl;
          }
        }

        void controlOperation ( OCPI::Metadata::Worker::ControlOperation op )
        {
          if ( !( getControlMask () & ( 1 << op ) ) )
          {
            return;
          }

          if ( op == OM::Worker::OpStart )
          {
            if ( nConnectedPorts != metadataImpl.getNumPorts ( ) )
            {
               throw OC::ApiError( "OCL worker cannot be started until all ports are connected." );
            }
          }

          kernelProlog ( controlOp2Opcode ( op ) );

          size_t offset = 0; // Control op is just a single work item
          size_t gtid = 1;
          size_t ltid = 1;

          Grid grid ( offset, gtid, ltid );

          device_worker.run ( grid );

          kernelEpilog ( );

          switch ( op )
          {
            case OM::Worker::OpStart:
              isEnabled = true;
              runTimer.reset();
              runTimer.start();
              break;
            case  OM::Worker::OpStop:
            case OM::Worker::OpRelease:
              if ( isEnabled )
              {
                runTimer.stop();
                runTimer.reset();
              }
              isEnabled = false;
              break;
            default:
              break;
          }
        }

        bool has_run_timedout ( )
        {
          timedOut = false;
          if ( myRunCondition->timeout )
          {
            OS::ElapsedTime et;
            runTimer.stop();
            runTimer.getValue( et );
            unsigned int elapsed_usecs =
	      et.seconds() * 1000000 + et.nanoseconds() / 1000 ;
            runTimer.start();
            if ( elapsed_usecs > myRunCondition->usecs )
            {
              timedOut = true;
              return true;
            }
          }
          return false;
        }

        void run ( bool& anyone_run )
        {
          if ( !enabled ( ) )
          {
            return;
          }

          bool run_timed_out = has_run_timedout ( );

          updatePortsPreRun ( );

          if ( !run_timed_out )
          {
            if ( ( myRunCondition->portMasks & readyPorts ) != myRunCondition->portMasks )
            {
              return;
            }
          }

          /* Set the arguments to the worker */
          kernelProlog ( OCPI_OCL_RUN );

          /* Local work group size comes fro OCL_WG_XYZ defines in worker */
          Grid grid ( 0, myPorts [ 0 ].attr->length / myPorts [ 0 ].dataValueWidthInBytes, 0 );
          device_worker.run ( grid );

          kernelEpilog ( );
          anyone_run = true;

          switch ( *myResult )
          {
            case OCL_ADVANCE:
              advancedPortBuffers ( );
              break;
            case OCL_DONE:
              if ( isEnabled )
              {
                runTimer.stop();
              }
              isEnabled = false;
              break;
            case OCL_OK:
              /* Nothing to do */
              break;
            case OCL_FATAL:
            default:
              if ( isEnabled )
              {
                runTimer.stop();
              }
              isEnabled = false;
              setControlState ( OC::UNUSABLE );
          }
          updatePortsPostRun();
        }

        void read ( uint32_t, uint32_t, void* )
        {
          ocpiAssert ( 0 );
        }

        void write ( uint32_t, uint32_t, const void* )
        {
          ocpiAssert ( 0 );
        }

        OC::Port& createPort ( const OM::Port& metaport,
                               const OA::PValue* props );

        bool enabled ( ) const
        {
          return isEnabled;
        }

        virtual void prepareProperty ( OU::Property& md,
				   volatile void *&writeVaddr,
				   const volatile void *&readVaddr)
        {
          if ( md.m_baseType != OA::OCPI_Struct &&
               !md.m_isSequence &&
               ( md.m_baseType != OA::OCPI_String ) &&
               OU::baseTypeSizes [ md.m_baseType ] <= 32 &&
              !md.m_writeError )
          {
            if ( ( md.m_offset + md.m_nBytes ) >
                 metadataImpl.getPropertySize( ) )
            {
               throw OC::ApiError( "OCL property is out of bounds." );
            }
            readVaddr = (uint8_t*) myProperties + md.m_offset;
            writeVaddr = (uint8_t*) myProperties + md.m_offset;
          }
        }


        OC::Port& createOutputPort ( OM::PortOrdinal portId,
                                     OS::uint32_t bufferCount,
                                     OS::uint32_t bufferSize,
                                     const OA::PValue* props ) throw();

        OC::Port&createInputPort ( OM::PortOrdinal portId,
                                   OS::uint32_t bufferCount,
                                   OS::uint32_t bufferSize,
                                   const OA::PValue* props ) throw();

#undef OCPI_DATA_TYPE_S
      // Set a scalar property value
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      void set##pretty##Property(const OA::Property &p, const run val) const { \
        if (p.m_info.m_writeError ) \
          throw; /*"worker has errors before write */ \
        volatile store *pp = (volatile store *)(myProperties + p.m_info.m_offset); \
        if (bits > 32) { \
          assert(bits == 64); \
          volatile uint32_t *p32 = (volatile uint32_t *)pp; \
          p32[1] = ((const uint32_t *)&val)[1]; \
          p32[0] = ((const uint32_t *)&val)[0]; \
        } else \
          *pp = *(const store *)&val; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */ \
      } \
      void set##pretty##SequenceProperty(const OA::Property &p,const run *vals, unsigned length) const { \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors before write */ \
        memcpy((void *)(myProperties + p.m_info.m_offset + p.m_info.m_align), vals, length * sizeof(run)); \
        *(volatile uint32_t *)(myProperties + p.m_info.m_offset) = length; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */ \
      }
      // Set a string property value FIXME redundant length check???
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) \
      virtual void set##pretty##Property(const OA::Property &p, const run val) const { \
        unsigned ocpi_length; \
        if (!val || (ocpi_length = strlen(val)) > p.m_info.m_stringLength) \
          throw; /*"string property too long"*/; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors before write */ \
        uint32_t *p32 = (uint32_t *)(myProperties + p.m_info.m_offset); \
        /* if length to be written is more than 32 bits */ \
        if (++ocpi_length > 32/CHAR_BIT) \
          memcpy(p32 + 1, val + 32/CHAR_BIT, ocpi_length - 32/CHAR_BIT); \
        uint32_t i; \
        memcpy(&i, val, 32/CHAR_BIT); \
        p32[0] = i; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */ \
      } \
      void set##pretty##SequenceProperty(const OA::Property &p, const run *vals, unsigned length) const { \
        if (length > p.m_info.m_sequenceLength) \
          throw; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors before write */ \
        char *cp = (char *)(myProperties + p.m_info.m_offset + 32/CHAR_BIT); \
        for (unsigned i = 0; i < length; i++) { \
          unsigned len = strlen(vals[i]); \
          if (len > p.m_info.m_sequenceLength) \
            throw; /* "string in sequence too long" */ \
          memcpy(cp, vals[i], len+1); \
        } \
        *(uint32_t *)(myProperties + p.m_info.m_offset) = length; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */ \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      virtual run get##pretty##Property(const OA::Property &p) const { \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors before read "*/ \
        uint32_t *pp = (uint32_t *)(myProperties + p.m_info.m_offset); \
        union { \
                run r; \
                uint32_t u32[bits/32]; \
        } u; \
        if (bits > 32) \
          u.u32[1] = pp[1]; \
        u.u32[0] = pp[0]; \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors after read */ \
        return u.r; \
      } \
      unsigned get##pretty##SequenceProperty(const OA::Property &p, run *vals, unsigned length) const { \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors before read "*/ \
        uint32_t n = *(uint32_t *)(myProperties + p.m_info.m_offset); \
        if (n > length) \
          throw; /* sequence longer than provided buffer */ \
        memcpy(vals, (void*)(myProperties + p.m_info.m_offset + p.m_info.m_align), \
               n * sizeof(run)); \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors after read */ \
        return n; \
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this. FIXME redundant length check
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) \
      virtual void get##pretty##Property(const OA::Property &p, char *cp, unsigned length) const { \
        unsigned stringLength = p.m_info.m_stringLength; \
        if (length < stringLength + 1) \
          throw; /*"string buffer smaller than property"*/; \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors before write */ \
        uint32_t i32, *p32 = (uint32_t *)(myProperties + p.m_info.m_offset); \
        memcpy(cp + 32/CHAR_BIT, p32 + 1, stringLength + 1 - 32/CHAR_BIT); \
        i32 = *p32; \
        memcpy(cp, &i32, 32/CHAR_BIT); \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors after write */ \
      } \
      unsigned get##pretty##SequenceProperty \
      (const OA::Property &p, char **vals, unsigned length, char *buf, unsigned space) const { \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors before read */ \
        uint32_t \
          n = *(uint32_t *)(myProperties + p.m_info.m_offset), \
          wlen = p.m_info.m_stringLength + 1; \
        if (n > length) \
          throw; /* sequence longer than provided buffer */ \
        char *cp = (char *)(myProperties + p.m_info.m_offset + 32/CHAR_BIT); \
        for (unsigned i = 0; i < n; i++) { \
          if (space < wlen) \
            throw; \
          memcpy(buf, cp, wlen); \
          cp += wlen; \
          vals[i] = buf; \
          unsigned slen = strlen(buf) + 1; \
          buf += slen; \
          space -= slen; \
        } \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors after read */ \
        return n; \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#define PUT_GET_PROPERTY(n)						\
      void setProperty##n(const OA::PropertyInfo &, uint##n##_t) const {} \
      uint##n##_t getProperty##n(const OA::PropertyInfo &) const { return 0; }
      PUT_GET_PROPERTY(8)
      PUT_GET_PROPERTY(16)
      PUT_GET_PROPERTY(32)
      PUT_GET_PROPERTY(64)
      void setPropertyBytes(const OA::PropertyInfo &, uint32_t,
			    const uint8_t *, unsigned ) const {}
      void getPropertyBytes(const OA::PropertyInfo &, uint32_t,
			    uint8_t *, unsigned ) const {}

    }; // End: class Worker

    OC::Worker& Application::createWorker ( OC::Artifact* art,
                                            const char* appInstName,
                                            ezxml_t impl,
                                            ezxml_t inst,
                                            const OCPI::Util::PValue* wParams )
    {
      return *new Worker ( *this, art, appInstName, impl, inst, wParams );
    }

    void Application::run ( DataTransfer::EventManager* event_manager,
                            bool& more_to_do )
    {
      ( void ) event_manager;

      for ( Worker* w = firstChild (); w; w = w->nextChild ( ) )
      {
        w->run ( more_to_do );
      }
    }

    class InternalBuffer
    {
      friend class Port;
      friend class Worker;
      friend class ExternalPort;

      private:
        Port* myPort;
        OclDpMetadata* metadata;  // where is the metadata buffer
        uint8_t* data;  // where is the data buffer
        uint32_t length;  // length of the buffer (not message)
        bool last;  // last buffer in the set
        volatile uint32_t* local;  // Read locally and written remotely
        volatile uint32_t* remote; // Written remotely (never read)
        volatile uint32_t* shadow; // Read/write locally

        void release();

        void put ( uint32_t dataLength,
                   uint8_t opcode,
                   bool endOfData )
        {
          (void)endOfData;
          ocpiAssert(dataLength <= length);
          metadata->opcode = opcode;
          metadata->length = dataLength;
          release();
        }

    }; // End: class InternalBuffer

    class Port : public OC::PortBase<Worker,Port,ExternalPort>
    {
      friend class Worker;
      friend class ExternalPort;
      friend class InternalBuffer;

      private:
        uint32_t remoteIndex;
        ezxml_t m_connection;
        ExternalPort* myExternalPort;
        OCPI::Metadata::PortOrdinal myPortOrdinal;

        uint32_t* flags;
        InternalBuffer* currentBuffer;
        InternalBuffer* localBuffers;
        InternalBuffer* nextLocal;
        InternalBuffer* nextRemote;
        uint32_t* farFlags;
        uint32_t* remoteFlags;
        uint32_t* localFlags;

        uint32_t* local;
        uint32_t* remote;
        uint32_t* shadow;

      void setMode( ConnectionMode ){}

        void disconnect ()
        throw ( OCPI::Util::EmbeddedException )
        {
          throw OC::ApiError( "OCL disconnect not yet implemented." );
        }

      bool isLocal() const { return false; }
        void startConnect (const OCPI::RDT::Descriptors */*other*/, const OCPI::Util::PValue */*params*/ )
        {
          if ( !m_canBeExternal )
          {
            return;
          }
        }

        Port ( Worker& w,
               const OA::PValue* params,
               const OM::Port& mPort, // the parsed port metadata
               bool argIsProvider )
          : OC::PortBase<Worker,Port,ExternalPort> ( w, *this, mPort, argIsProvider,
						     ( 1 << OCPI::RDT::ActiveFlowControl ) |
						     ( 1 << OCPI::RDT::ActiveMessage ),
						     params ),
            remoteIndex ( 0 ),
            m_connection ( 0 ),
            myPortOrdinal ( mPort.ordinal )
        {
          m_canBeExternal = true;

          parent().myPorts [ myPortOrdinal ].attr->connected = false;
          parent().myPorts [ myPortOrdinal ].dataValueWidthInBytes = (mPort.m_dataValueWidth + 7) / 8;
          parent().myPorts [ myPortOrdinal ].attr->optional = mPort.optional;

          myDesc.dataBufferPitch = myDesc.dataBufferSize;

          myDesc.dataBufferBaseAddr = 0;

          myDesc.metaDataPitch = sizeof ( OclDpMetadata );
          myDesc.metaDataBaseAddr = 0;

          myDesc.fullFlagSize = sizeof ( uint32_t );
          myDesc.fullFlagPitch = sizeof ( uint32_t );
          myDesc.fullFlagValue = 1;
          myDesc.fullFlagBaseAddr = 0;

          myDesc.emptyFlagSize = sizeof ( uint32_t );
          myDesc.emptyFlagPitch = sizeof ( uint32_t );
          myDesc.emptyFlagValue = 1;
          myDesc.emptyFlagBaseAddr = 0;

          unsigned int nAlloc =
              OC::roundup ( myDesc.dataBufferPitch * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN) +
              OC::roundup ( myDesc.metaDataPitch * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN) +
              OC::roundup ( sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN) + // local flags
              // These might actually be remote
              OC::roundup ( sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN) + // remote flags
              // These might not be needed if we are ActiveFlowControl
              OC::roundup ( sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          // FIXME how do we set these?
          int mailbox = 6;
          int max_mailbox = 10;

          int pid = getpid ( );

          myDesc.oob.port_id = myPortOrdinal;

          snprintf ( myDesc.oob.oep,
                     sizeof ( myDesc.oob.oep ),
                     "ocpi-smb-pio:pioXfer%d:%d.%d.%d",
                     pid,
                     nAlloc,
                     mailbox,
                     max_mailbox );

          uint8_t* allocation = ( uint8_t* ) calloc ( nAlloc,
                                                      sizeof ( uint8_t ) );
          if ( !allocation )
          {
            throw OC::ApiError( "OCL failed to allocate external port buffers." );
          }

          myDesc.dataBufferBaseAddr = reinterpret_cast<uint64_t>(allocation);
          uint8_t* buffer = allocation;
          allocation += OC::roundup(myDesc.dataBufferPitch * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          for ( size_t n = 0; n < myDesc.nBuffers; n++ )
          {
            w.device_worker.registerPtr ( (void*)buffer, myDesc.dataBufferPitch );
            buffer += myDesc.dataBufferPitch;
          }

          myDesc.metaDataBaseAddr = reinterpret_cast<uint64_t>(allocation);
          allocation += OC::roundup(myDesc.metaDataPitch * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          localFlags = (uint32_t*)allocation;
          local = localFlags;
          allocation += OC::roundup(sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          remoteFlags = (uint32_t*)allocation;
          remote = remoteFlags;
          allocation += OC::roundup(sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          farFlags = (uint32_t*)allocation;
          shadow = farFlags;

          myDesc.emptyFlagBaseAddr = 0;
          myDesc.fullFlagBaseAddr = reinterpret_cast<uint64_t>( local );

          // Allow default connect params on port construction prior to connect
          applyConnectParams(NULL, params);
        }
    public:
      ~Port() {
      }
    private:
        // All the info is in.  Do final work to (locally) establish the connection
        const OCPI::RDT::Descriptors *finishConnect(const OCPI::RDT::Descriptors &other,
						    OCPI::RDT::Descriptors &/*feedback*/) {
          OCPI::RDT::PortRole myRole = (OCPI::RDT::PortRole) getData().data.role;

          // FIXME - can't we avoid string processing here?
          int pid;
          int nAlloc;
          int mailbox;
          int max_mailbox;

          if ( sscanf ( other.desc.oob.oep,
                        "ocpi-smb-pio:pioXfer%d;%d.%d.%d",
                        &pid,
                        &nAlloc,
                        &mailbox,
                        &max_mailbox ) != 4 )
          {
            throw OC::ApiError("OCL other port's endpoint description wrong: \"",
                               other.desc.oob.oep, "\"", NULL);
          }

          switch ( myRole )
          {
            case OCPI::RDT::ActiveFlowControl:
              // Nothing to do
              break;
            case OCPI::RDT::ActiveMessage:

              if ( isProvider())
              {
                if ( other.desc.dataBufferSize > myDesc.dataBufferSize )
                {
                  throw OC::ApiError("At consumer, remote buffer size is larger than mine", NULL);
                }
                else if (other.desc.dataBufferSize < myDesc.dataBufferSize )
                {
                  throw OC::ApiError("At producer, remote buffer size smaller than mine", NULL);
                }
              }
              break;
            case OCPI::RDT::Passive:
              break;
            default:
              ocpiAssert(0);
          }

          // Initialize our structures that keep track of LOCAL buffer status
          InternalBuffer* lb = nextLocal = nextRemote = localBuffers = new InternalBuffer[myDesc.nBuffers];

          OclDpMetadata* metadata = reinterpret_cast<OclDpMetadata*>( myDesc.metaDataBaseAddr );

          uint8_t* localData = reinterpret_cast<uint8_t*>( myDesc.dataBufferBaseAddr );

          uint32_t* otherRemote = reinterpret_cast<uint32_t*> ( other.desc.fullFlagBaseAddr );

          for ( unsigned int i = 0; i < myDesc.nBuffers; i++, lb++ )
          {
            lb->myPort = this;
            lb->metadata = metadata + i;
            lb->data = localData + i * myDesc.dataBufferPitch;
            lb->length = myDesc.dataBufferPitch;
            lb->last = false;
            lb->local = local + i;
            lb->remote = otherRemote + i;
            lb->shadow = shadow + i;
            *lb->shadow= 0;

            if ( isProvider( ) )
            {
              (*lb->remote) = 1;
            }
            else
            {
              (*lb->remote) = 0;
            }
          }
          (lb-1)->last = true;

          for ( size_t n = 0; n < myDesc.nBuffers; n++ )
          {
            OclDpMetadata* metadata = reinterpret_cast<OclDpMetadata*> ( myDesc.metaDataBaseAddr );
            metadata [ n ].length = myDesc.dataBufferSize;
          }
          parent().myPorts [ parent().nConnectedPorts++ ].attr->connected = true;
	  return NULL;
        }
        // Connection between two ports inside this container
        // We know they must be in the same artifact, and have a metadata-defined connection
        void connectInside ( OC::Port& provider,
                             const OA::PValue* /*myProps*/)
        {
          // We're both in the same runtime artifact object, so we know the port class
          Port& pport = static_cast<Port&>(provider);

          if ( m_connection != pport.m_connection )
          {
            throw OC::ApiError ( "Ports are both local in artifact, but are not connected", 0 );
          }
#if 0
	  pport.applyConnectParams(&getData().data, otherProps);
	  applyConnectParams(&provider.getData().data, myProps);
#endif
        }

#if 0
        // Connect to a port in a like container (same driver)
        bool connectLike ( const OA::PValue* uProps,
                           OC::Port& provider,
                           const OA::PValue* pProps )
        {
          Port& pport = static_cast<Port&> ( provider );

          ocpiAssert(m_canBeExternal && pport.m_canBeExternal);

          pport.applyConnectParams ( pProps );
          applyConnectParams ( uProps );
          determineRoles ( provider.getData().data );
          finishConnect ( provider.getData().data );
          pport.finishConnect ( getData().data );
          return true;
        }
#endif
        bool isLocalBufferReady ( )
        {
          if ( *nextLocal->local == *nextLocal->shadow )
          {
            return false;
          }

          return true;
        }

        bool getLocal ( )
        {
          if ( *nextLocal->local == *nextLocal->shadow )
          {
            return false;
          }

          (*nextLocal->shadow)++;

          return true;
        }

        // The input method = get a buffer that has data in it.
        InternalBuffer* getBuffer ( uint8_t*& bdata,
                                    uint32_t& length,
                                    uint8_t& opcode,
                                    bool& end )
        {
          ocpiAssert( isProvider() );

          if ( !getLocal() )
          {
            return 0;
          }
          bdata = nextLocal->data;
          length = nextLocal->metadata->length;
          opcode = nextLocal->metadata->opcode;
          end = false; // someday bit in metadata
          return static_cast<InternalBuffer*>( nextLocal );
        }

        InternalBuffer* getBuffer ( uint8_t*& bdata,
                                    uint32_t& length )
        {
          ocpiAssert(!isProvider());
          if ( !getLocal() )
          {
            return 0;
          }
          bdata = nextLocal->data;
          length = nextLocal->length;

          return static_cast<InternalBuffer*>( nextLocal );
        }

        void endOfData()
        {
          ocpiAssert( !isProvider() );
        }

        void advanceLocal ();

#if 0
        // Directly connect to this port
        // which creates a dummy user port
        OA::ExternalPort& connectExternal ( const char* name,
                                            const OA::PValue* userProps,
                                            const OA::PValue* props );
#endif
        OC::ExternalPort &createExternal(const char *extName, bool provider, 
					 const OU::PValue *extParams,
					 const OU::PValue *connParams);
      public:
        OCPI::Metadata::PortOrdinal portOrdinal ( )
        {
          return myPortOrdinal;
        }

    }; // End: class Port

    void InternalBuffer::release()
    {
      myPort->advanceLocal();
    }

    void Worker::updatePortsPostRun ( )
    {
      for ( Port* ocpiport = firstChild();
            ocpiport;
            ocpiport = ocpiport->nextChild() )
      {
        size_t n = ocpiport->portOrdinal ( );
        ocpiport->nextLocal->metadata->length = myPorts [ n ].attr->length;
        ocpiport->nextLocal->metadata->opcode = myPorts [ n ].attr->u.operation;
        myPorts [ n ].current.data = 0;
        myPorts [ n ].current.maxLength = 0;
        myPorts [ n ].attr->length = 0;
        myPorts [ n ].attr->u.operation = 0;
        readyPorts &= ~( 1 << n );
      }
    }

    void Worker::updatePortsPreRun ( )
    {
      for ( Port* ocpiport = firstChild();
            ocpiport;
            ocpiport = ocpiport->nextChild() )
      {
        size_t n = ocpiport->portOrdinal ( );

        if ( myPorts [ n ].current.data ) {
          continue;
        }

        if ( ocpiport->isProvider ( ) )
        {
          uint8_t* bdata;
          uint32_t length;
          uint8_t opcode;
          bool end;
          ocpiport->currentBuffer = ocpiport->getBuffer ( bdata,
                                                          length,
                                                          opcode,
                                                          end );
          if ( ocpiport->currentBuffer )
          {
            readyPorts |= ( 1 << n );
            myPorts [ n ].current.data = bdata;
            myPorts [ n ].current.maxLength = ocpiport->myDesc.dataBufferSize;
            myPorts [ n ].attr->length = length;
            myPorts [ n ].attr->u.operation = opcode;
          }
        }
        else
        {
          uint8_t* bdata;
          uint32_t length;
          ocpiport->currentBuffer = ocpiport->getBuffer ( bdata,
                                                          length );
          if ( ocpiport->currentBuffer )
          {
            readyPorts |= ( 1 << n );
            myPorts [ n ].current.data = bdata;
            myPorts [ n ].current.maxLength = ocpiport->myDesc.dataBufferSize;
            myPorts [ n ].attr->length = length;
            myPorts [ n ].attr->u.operation = 0;
          }
        }
      }
    }

    void Worker::advancedPortBuffers ( )
    {
      for ( Port* ocpiport = firstChild();
            ocpiport;
            ocpiport = ocpiport->nextChild() )
      {
        size_t n = ocpiport->portOrdinal ( );

        if ( ocpiport->isProvider ( ) )
        {
          ocpiport->currentBuffer->release ( );
        }
        else
        {
          bool end = false;
          ocpiport->currentBuffer->put ( myPorts [ n ].attr->length,
                                         myPorts [ n ].attr->u.operation,
                                         end );
        }
      }
    }

    OC::Port& Worker::createPort ( const OM::Port& metaPort,
                                   const OA::PValue* props )
    {
      return *new Port ( *this, props, metaPort, metaPort.provider );
    }

    OC::Port& Worker::createOutputPort ( OM::PortOrdinal portId,
                                         OS::uint32_t bufferCount,
                                         OS::uint32_t bufferSize,
                                         const OA::PValue* props )
    throw()
    {
      ( void ) portId;
      ( void ) bufferCount;
      ( void ) bufferSize;
      ( void ) props;
      return *(Port *)0;
    }

    OC::Port& Worker::createInputPort ( OM::PortOrdinal portId,
                                        OS::uint32_t bufferCount,
                                        OS::uint32_t bufferSize,
                                        const OA::PValue* props )
    throw()
    {
      ( void ) portId;
      ( void ) bufferCount;
      ( void ) bufferSize;
      ( void ) props;
      return *(Port *)0;
    }

    // Buffers directly used by the "user" (non-container/component) API
    class ExternalBuffer : OA::ExternalBuffer
    {
      friend class Port;
      friend class ExternalPort;

      private:
        ExternalPort* myExternalPort;
        OclDpMetadata* metadata; // where is the metadata buffer
        uint8_t* data; // where is the data buffer
        uint32_t length; // length of the buffer (not message)
        bool last; // last buffer in the set
        volatile uint32_t* local;  // Read locally and written remotely
        volatile uint32_t* remote; // Written remotely (never read)
        volatile uint32_t* shadow; // Read/write locally

        void release();

        void put ( uint32_t dataLength,
                   uint8_t opcode,
                   bool endOfData )
        {
          (void)endOfData;
          ocpiAssert(dataLength <= length);
          metadata->opcode = opcode;
          metadata->length = dataLength;
          release();
        }
    }; // End: class ExternalBuffer

    // Producer or consumer
    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort>
    {
      friend class Port;
      friend class ExternalBuffer;

      private:
        uint32_t* flags;
        ExternalBuffer* localBuffers;
        ExternalBuffer* nextLocal;
        ExternalBuffer* nextRemote;
        uint8_t *localData;
        OclDpMetadata *metadata;

        uint32_t* local;
        uint32_t* remote;
        uint32_t* shadow;

        ExternalPort( Port& port,
		      const char* name,
		      bool isProvider,
		      const OA::PValue* extParams,
		      const OA::PValue* connParams )
	  : OC::ExternalPortBase<Port,ExternalPort> ( port, *this, name, extParams, connParams, isProvider )
        {
          getData().data.options = ( 1 << OCPI::RDT::ActiveFlowControl ) |
                                        ( 1 << OCPI::RDT::ActiveMessage ) |
                                        ( 1 << OCPI::RDT::ActiveOnly );

          applyConnectParams (NULL, extParams);

          port.determineRoles ( getData().data );

          /*
              Buffer are shared between host and device so buffer count
             must match
          */
          myDesc.nBuffers = parent().getData().data.desc.nBuffers;
          unsigned int nFar = parent().getData().data.desc.nBuffers;
          unsigned int nLocal = myDesc.nBuffers;

          // Reuse the parent's buffer to avoid a copy
          myDesc.dataBufferBaseAddr = parent().getData().data.desc.dataBufferBaseAddr;
          myDesc.dataBufferSize = parent().getData().data.desc.dataBufferSize;
          myDesc.dataBufferPitch = parent().getData().data.desc.dataBufferPitch;
          myDesc.metaDataBaseAddr =  parent().getData().data.desc.metaDataBaseAddr;
          myDesc.metaDataPitch = parent().getData().data.desc.metaDataPitch;
          myDesc.fullFlagBaseAddr = 0;
          myDesc.fullFlagPitch = parent().getData().data.desc.fullFlagPitch;
          myDesc.fullFlagSize = parent().getData().data.desc.fullFlagSize;
          myDesc.fullFlagValue = parent().getData().data.desc.fullFlagValue;
          myDesc.emptyFlagBaseAddr = 0;
          myDesc.emptyFlagSize = parent().getData().data.desc.emptyFlagSize;
          myDesc.emptyFlagPitch = parent().getData().data.desc.emptyFlagPitch;
          myDesc.emptyFlagValue = parent().getData().data.desc.emptyFlagValue;

          // Allocate my local memory, making everything on a nice boundary.
          // (assume empty flag pitch same as full flag pitch)
          unsigned int nAlloc =
              OC::roundup ( myDesc.dataBufferPitch * nLocal, OCLDP_LOCAL_BUFFER_ALIGN) +
              OC::roundup ( myDesc.metaDataPitch * nLocal, OCLDP_LOCAL_BUFFER_ALIGN) +
              OC::roundup ( sizeof(uint32_t) * nLocal, OCLDP_LOCAL_BUFFER_ALIGN) + // local flags
              // These might actually be remote
              OC::roundup ( sizeof(uint32_t) * nLocal, OCLDP_LOCAL_BUFFER_ALIGN) + // remote flags
              // These might not be needed if we are ActiveFlowControl
              OC::roundup ( sizeof(uint32_t) * nFar, OCLDP_LOCAL_BUFFER_ALIGN);

          // FIXME where do we get the mailbox information?
          int mailbox = 7;
          int max_mailbox = 10;
          int pid = getpid ( );
          myDesc.oob.port_id = port.metaPort().ordinal;
          snprintf ( myDesc.oob.oep,
                     sizeof ( myDesc.oob.oep ),
                     "ocpi-smb-pio:pioXfer%d:%u.%d.%d",
                     pid,
                     nAlloc,
                     mailbox,
                     max_mailbox );

          uint8_t* allocation = ( uint8_t* ) calloc ( nAlloc, sizeof ( uint8_t ) );

          if ( !allocation )
          {
            throw OC::ApiError( "OCL failed to allocate external port buffers." );
          }

          // Resuing the far data buffers
          localData = reinterpret_cast<uint8_t*>(myDesc.dataBufferBaseAddr);
          allocation += OC::roundup(myDesc.dataBufferPitch * nLocal, OCLDP_LOCAL_BUFFER_ALIGN);

          // Resuing the far metadata buffers
          metadata = reinterpret_cast<OclDpMetadata*>(myDesc.metaDataBaseAddr);
          allocation += OC::roundup(myDesc.metaDataPitch * nLocal, OCLDP_LOCAL_BUFFER_ALIGN);

          uint32_t* localFlags = (uint32_t*)allocation;
          local = localFlags;
          allocation += OC::roundup(sizeof(uint32_t) * nLocal, OCLDP_LOCAL_BUFFER_ALIGN);

          uint32_t* remoteFlags = (uint32_t*)allocation;
          remote = remoteFlags;

          allocation += OC::roundup(sizeof(uint32_t) * nLocal, OCLDP_LOCAL_BUFFER_ALIGN);
          uint32_t* farFlags = (uint32_t*)allocation;
          shadow = farFlags;

          myDesc.emptyFlagBaseAddr = 0;
          myDesc.fullFlagBaseAddr  = reinterpret_cast<uint64_t>( local );

          uint32_t* otherRemote = reinterpret_cast<uint32_t*> ( parent().getData().data.desc.fullFlagBaseAddr );

          // Initialize our structures that keep track of LOCAL buffer status
          ExternalBuffer *lb = nextLocal = nextRemote = localBuffers = new ExternalBuffer[nLocal];
          for (unsigned i = 0; i < nLocal; i++, lb++) {
            lb->myExternalPort = this;
            lb->metadata = metadata + i;
            lb->data = localData + i * myDesc.dataBufferPitch;
            lb->length = myDesc.dataBufferPitch;
            lb->last = false;
            lb->local = local + i;
            lb->remote = otherRemote + i;
            lb->shadow = shadow + i;
            *lb->shadow = 0;

            if ( !parent().isProvider( ) )
            {
              (*lb->remote) = 1;
            }
            else
            {
              (*lb->remote) = 0;
            }
          }
          (lb-1)->last = true;
        }

      public:
        ~ExternalPort()
        {
          delete [] localBuffers;
        }

        bool getLocal ( )
        {
          if ( *nextLocal->local == *nextLocal->shadow )
          {
            return false;
          }

          (*nextLocal->shadow)++;

          return true;
        }

        OA::ExternalBuffer* getBuffer ( uint8_t*& bdata,
                                        uint32_t& length,
                                        uint8_t& opcode,
                                        bool& end )
        {
          ocpiAssert( !parent().isProvider() );

          if ( !getLocal() )
          {
            return 0;
          }

          bdata = (uint8_t*)parent().parent().device_worker.mapPtr ( nextLocal->data,
                                                           OCPI::OCL::DeviceWorker::MAP_TO_READ );
          length = nextLocal->metadata->length;
          opcode = nextLocal->metadata->opcode;
          end = false;

          return static_cast<OA::ExternalBuffer *>( nextLocal );
        }

        OA::ExternalBuffer* getBuffer ( uint8_t*& bdata,
                                        uint32_t& length )
        {
          ocpiAssert(parent().isProvider());
          if ( !getLocal() )
          {
            return 0;
          }

          bdata = (uint8_t*)parent().parent().device_worker.mapPtr ( nextLocal->data,
                                                           OCPI::OCL::DeviceWorker::MAP_TO_WRITE );

          length = nextLocal->length;

          return static_cast<OA::ExternalBuffer *>( nextLocal );
        }

        void advanceLocal ()
        {
          (*nextLocal->remote)++;

          if ( nextLocal->last )
          {
            nextLocal = localBuffers;
          }
          else
          {
            nextLocal++;
          }
        }

        void endOfData ( )
        {
          ocpiAssert(parent().isProvider());
        }

        bool tryFlush ( )
        {
          return false;
        }

    }; // End: class ExternalPort

    void ExternalBuffer::release()
    {
      myExternalPort->parent().parent().device_worker.unmapPtr ( (void*)myExternalPort->nextLocal->data );
      myExternalPort->advanceLocal();
    }

#if 1
    OC::ExternalPort &Port::createExternal(const char *extName, bool isProvider,
					   const OU::PValue *extParams, const OU::PValue *connParams) {
      return *new ExternalPort(*this, extName, !isProvider, extParams, connParams);
    }
#else
    OA::ExternalPort& Port::connectExternal ( const char* extName,
                                              const OA::PValue* userProps,
                                              const OA::PValue* props )
    {
      if ( !m_canBeExternal )
      {
        throw OC::ApiError ( "OCL For external port \"", extName, "\", port \"",
                             name().c_str(), "\" of worker \"",
                             parent().implTag().c_str(), "/", parent().instTag().c_str(), "/",
                             parent().name().c_str(),
                            "\" is locally connected in the OCL bitstream. ", NULL);
      }

      applyConnectParams ( props );

      myExternalPort = new ExternalPort ( *this,
                                           extName,
                                           !isProvider(),
                                           userProps );
      finishConnect(myExternalPort->getData().data);
      return *myExternalPort;
    }
#endif
    void Port::advanceLocal ()
    {
      (*nextLocal->remote)++;

      if ( nextLocal->last )
      {
        nextLocal = localBuffers;
      }
      else
      {
        nextLocal++;
      }
    }
  } // End: namespace OCL

} // End: namespace OCPI

