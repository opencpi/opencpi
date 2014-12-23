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

#include <unistd.h>
#include <climits>
#include "OCL_Worker.h"

#include "ContainerManager.h"
#include "OclPlatformManager.h"

namespace OCPI
{
  namespace OCL
  {
    namespace OS = OCPI::OS;
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OC = OCPI::Container;

    const size_t OCLDP_LOCAL_BUFFER_ALIGN ( 16 );

    typedef struct
    {
      uint32_t length;
      uint8_t opcode;
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

      OcpiOclOpcodes_t controlOp2Opcode ( OU::Worker::ControlOperation op )
      {
        switch ( op )
        {
	case OU::Worker::OpInitialize:
            return OCPI_OCL_INITIALIZE;
	case OU::Worker::OpStart:
            return OCPI_OCL_START;
	case OU::Worker::OpStop:
            return OCPI_OCL_STOP;
	case OU::Worker::OpRelease:
            return OCPI_OCL_RELEASE;
	case OU::Worker::OpTest:
            return OCPI_OCL_TEST;
	case OU::Worker::OpBeforeQuery:
            return OCPI_OCL_BEFORE_QUERY;
	case OU::Worker::OpAfterConfigure:
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
				   OC::Worker *slave,
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
#if 0
        struct OCLPortInternal
        {
          OCLBuffer current;
          size_t dataValueWidthInBytes;
        };
#endif

        bool isEnabled;
        Container& myContainer;
        const char* implName;
        OU::Worker metadataImpl;
        const char* instName;
      //        OU::Worker metadataInst;
        std::string myEntryPoint;
        uint8_t* myProperties;
        uint32_t nConnectedPorts;
        OCLPort* myPorts;
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
          instName ( ezxml_attr ( instXml, "name" ) ),
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
          const char *err = metadataImpl.parse(implXml);
	  if (err) // || (err = metadataInst.parse(instXml)))
	    throw OU::Error("Error processing worker metadata %s", err);
	  
          initializeContext ( );

          setControlOperations ( ezxml_cattr ( implXml, "controlOperations" ) );

          setControlMask ( getControlMask() | ( 1 << OU::Worker::OpStart ) );
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

          size_t n_ports = metadataImpl.nPorts ( );

          for ( size_t n = 0; n < n_ports; n++ )
          {
              bool connected = myPorts [ n ].isConnected;

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

                device_worker.setKernelArg ( arg++, &myPorts [ n ] ); // ptr
                device_worker.syncPtr ( &myPorts [ n ],
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

          for ( size_t n = 0; n < metadataImpl.nPorts ( ); n++ )
          {
            if ( myPorts [ n ].isConnected && myPorts [ n ].current.data )
            {
              device_worker.syncPtr ( &myPorts [ n ],
                                      OCPI::OCL::DeviceWorker::DEVICE_TO_HOST );
              // No need to sync myPorts [ n ].current.data map/unmap is used
            }
            myPorts [ n ].current.data = 0;
          }
       }

        void initializeContext ( )
        {
          uint32_t n_ports = metadataImpl.nPorts ( );

          myRunCondition = ( OCLRunCondition* ) calloc ( 1,  sizeof ( OCLRunCondition ) );

          device_worker.registerPtr ( (void*)myRunCondition,
                                      sizeof ( OCLRunCondition ) );

          // Default run condtion for ports
          myRunCondition->usePorts = true;
          myRunCondition->portMasks[0] = ~(-1 << n_ports);
	  myRunCondition->portMasks[1] = 0;
          myNewRunCondition = ( OCLBoolean* ) calloc ( 1,  sizeof ( OCLBoolean ) );

          device_worker.registerPtr ( (void*)myNewRunCondition,
                                      sizeof ( OCLBoolean ) );

          dummyBuffer = ( uint8_t* ) calloc ( 1, sizeof ( uint32_t ) );

          if ( !dummyBuffer )
          {
            throw OU::Error( "OCL failed to allocate worker dummy buffer." );
          }

          device_worker.registerPtr ( (void*)dummyBuffer, sizeof ( uint32_t ) );

          myPorts = new OCLPort[n_ports];
	  size_t length = sizeof(OCLPort) * n_ports;
	  memset(myPorts, 0, length);
	  device_worker.registerPtr ((void*)myPorts, length);

	  length = metadataImpl.totalPropertySize( ) + 4;
          myProperties = new uint8_t[length];
	  memset(myProperties, 0, length);
          device_worker.registerPtr ((void*)myProperties, length);
          myResult = new OCLResult;
          device_worker.registerPtr ( (void*)myResult, sizeof ( OCLResult ) );

          unsigned int nLocalMemories;
          OU::Memory* local_memories =
                         metadataImpl.memories ( nLocalMemories );
          if ( nLocalMemories )
          {
            for ( size_t n = 0; n < nLocalMemories; n++ )
            {
              void* p = calloc ( local_memories [ n ].m_nBytes, sizeof ( uint8_t ) );
              if ( !p )
              {
                throw OU::Error("OCL failed to allocate local memory.");
              }
              myLocalMemories.push_back ( p );
              device_worker.registerPtr ( p, local_memories [ n ].m_nBytes );
            }
          }
        }

        void finalizeContext ( )
        {
	  device_worker.unregisterPtr (myPorts);
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
              controlOperation ( OU::Worker::OpStop );
              controlOperation ( OU::Worker::OpRelease );
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

      void controlOperation (OU::Worker::ControlOperation op )
        {
          if ( !( getControlMask () & ( 1 << op ) ) )
          {
            return;
          }

          if ( op == OU::Worker::OpStart )
          {
            if ( nConnectedPorts != metadataImpl.nPorts ( ) )
            {
               throw OU::Error( "OCL worker cannot be started until all ports are connected." );
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
	  case OU::Worker::OpStart:
              isEnabled = true;
              runTimer.reset();
              runTimer.start();
              break;
	  case  OU::Worker::OpStop:
	  case OU::Worker::OpRelease:
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

          if (!run_timed_out && myRunCondition->usePorts) {
	    for (OCLPortMask *p = myRunCondition->portMasks; *p; p++)
	      if ((*p & readyPorts ) == *p)
		goto ok;
	    return;
          }
	ok:
          /* Set the arguments to the worker */
          kernelProlog ( OCPI_OCL_RUN );

          /* Local work group size comes fro OCL_WG_XYZ defines in worker */
          Grid grid ( 0, myPorts [ 0 ].current.length / myPorts [ 0 ].dataValueWidthInBytes, 0 );
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
              setControlState ( OU::Worker::UNUSABLE );
          }
          updatePortsPostRun();
        }

        void read ( size_t, size_t, void* )
        {
          ocpiAssert ( 0 );
        }

        void write (size_t, size_t, const void* )
        {
          ocpiAssert ( 0 );
        }

        OC::Port& createPort ( const OU::Port& metaport,
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
                 metadataImpl.totalPropertySize( ) )
            {
               throw OU::Error( "OCL property is out of bounds." );
            }
            readVaddr = (uint8_t*) myProperties + md.m_offset;
            writeVaddr = (uint8_t*) myProperties + md.m_offset;
          }
        }


        OC::Port& createOutputPort ( OU::PortOrdinal portId,
                                     size_t bufferCount,
                                     size_t bufferSize,
                                     const OA::PValue* props ) throw();

        OC::Port&createInputPort ( OU::PortOrdinal portId,
                                   size_t bufferCount,
                                   size_t bufferSize,
                                   const OA::PValue* props ) throw();

#undef OCPI_DATA_TYPE_S
      // Set a scalar property value
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      void set##pretty##Property(unsigned ordinal, const run val) const { \
	OA::PropertyInfo &info = properties()[ordinal];		  \
        if (info.m_writeError ) \
          throw; /*"worker has errors before write */ \
        volatile store *pp = (volatile store *)(myProperties + info.m_offset); \
        if (bits > 32) { \
          assert(bits == 64); \
          volatile uint32_t *p32 = (volatile uint32_t *)pp; \
          p32[1] = ((const uint32_t *)&val)[1]; \
          p32[0] = ((const uint32_t *)&val)[0]; \
        } else \
          *pp = *(const store *)&val; \
        if (info.m_writeError) \
          throw; /*"worker has errors after write */ \
      } \
      void set##pretty##SequenceProperty(const OA::Property &p,const run *vals, size_t length) const { \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors before write */ \
        memcpy((void *)(myProperties + p.m_info.m_offset + p.m_info.m_align), vals, length * sizeof(run)); \
        *(volatile uint32_t *)(myProperties + p.m_info.m_offset) = (uint32_t)length; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */ \
      }
      // Set a string property value FIXME redundant length check???
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) \
      virtual void set##pretty##Property(unsigned ordinal, const run val) const { \
	OA::PropertyInfo &info = properties()[ordinal];		  \
        size_t ocpi_length; \
        if (!val || (ocpi_length = strlen(val)) > info.m_stringLength) \
          throw; /*"string property too long"*/; \
        if (info.m_writeError) \
          throw; /*"worker has errors before write */ \
        uint32_t *p32 = (uint32_t *)(myProperties + info.m_offset); \
        /* if length to be written is more than 32 bits */ \
        if (++ocpi_length > 32/CHAR_BIT) \
          memcpy(p32 + 1, val + 32/CHAR_BIT, ocpi_length - 32/CHAR_BIT); \
        uint32_t i; \
        memcpy(&i, val, 32/CHAR_BIT); \
        p32[0] = i; \
        if (info.m_writeError) \
          throw; /*"worker has errors after write */ \
      } \
      void set##pretty##SequenceProperty(const OA::Property &p, const run *vals, size_t length) const { \
        if (length > p.m_info.m_sequenceLength) \
          throw; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors before write */ \
        char *cp = (char *)(myProperties + p.m_info.m_offset + 32/CHAR_BIT); \
        for (size_t i = 0; i < length; i++) { \
          size_t len = strlen(vals[i]); \
          if (len > p.m_info.m_sequenceLength) \
            throw; /* "string in sequence too long" */ \
          memcpy(cp, vals[i], len+1); \
        } \
        *(uint32_t *)(myProperties + p.m_info.m_offset) = (uint32_t)length; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */ \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      virtual run get##pretty##Property(unsigned ordinal) const { \
	OA::PropertyInfo &info = properties()[ordinal];		  \
        if (info.m_readError) \
          throw; /*"worker has errors before read "*/ \
        uint32_t *pp = (uint32_t *)(myProperties + info.m_offset); \
        union { \
                run r; \
                uint32_t u32[bits/32]; \
        } u; \
        if (bits > 32) \
          u.u32[1] = pp[1]; \
        u.u32[0] = pp[0]; \
        if (info.m_readError) \
          throw; /*"worker has errors after read */ \
        return u.r; \
      } \
      unsigned get##pretty##SequenceProperty(const OA::Property &p, run *vals, size_t length) const { \
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
      virtual void get##pretty##Property(unsigned ordinal, char *cp, size_t length) const { \
	OA::PropertyInfo &info = properties()[ordinal];			\
        size_t stringLength = info.m_stringLength; \
        if (length < stringLength + 1) \
          throw; /*"string buffer smaller than property"*/; \
        if (info.m_readError) \
          throw; /*"worker has errors before write */ \
        uint32_t i32, *p32 = (uint32_t *)(myProperties + info.m_offset); \
        memcpy(cp + 32/CHAR_BIT, p32 + 1, stringLength + 1 - 32/CHAR_BIT); \
        i32 = *p32; \
        memcpy(cp, &i32, 32/CHAR_BIT); \
        if (info.m_readError) \
          throw; /*"worker has errors after write */ \
      } \
      unsigned get##pretty##SequenceProperty \
      (const OA::Property &p, char **vals, size_t length, char *buf, size_t space) const { \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors before read */ \
        uint32_t n = *(uint32_t *)(myProperties + p.m_info.m_offset);	       \
        size_t wlen = p.m_info.m_stringLength + 1;		       \
        if (n > length) \
          throw; /* sequence longer than provided buffer */ \
        char *cp = (char *)(myProperties + p.m_info.m_offset + 32/CHAR_BIT); \
        for (unsigned i = 0; i < n; i++) { \
          if (space < wlen) \
            throw; \
          memcpy(buf, cp, wlen); \
          cp += wlen; \
          vals[i] = buf; \
          size_t slen = strlen(buf) + 1; \
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
      void setPropertyBytes(const OA::PropertyInfo &, size_t,
			    const uint8_t *, size_t ) const {}
      void getPropertyBytes(const OA::PropertyInfo &, size_t,
			    uint8_t *, size_t ) const {}

    }; // End: class Worker

    OC::Worker& Application::createWorker ( OC::Artifact* art,
                                            const char* appInstName,
                                            ezxml_t impl,
                                            ezxml_t inst,
					    OC::Worker *slave,
                                            const OCPI::Util::PValue* wParams )
    {
      assert(!slave);
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

        void put ( size_t dataLength,
                   uint8_t opcode,
                   bool endOfData )
        {
          (void)endOfData;
          ocpiAssert(dataLength <= length);
          metadata->opcode = opcode;
          metadata->length = (uint32_t)dataLength;
          release();
        }

    }; // End: class InternalBuffer

    class Port : public OC::PortBase<Worker,Port,ExternalPort>
    {
      friend class Worker;
      friend class ExternalPort;
      friend class InternalBuffer;

      private:
      //        uint32_t remoteIndex;
        ezxml_t m_connection;
      //        ExternalPort* myExternalPort;
        OU::PortOrdinal myPortOrdinal;

      //        uint32_t* flags;
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
          throw OU::Error( "OCL disconnect not yet implemented." );
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
               const OU::Port& mPort, // the parsed port metadata
               bool argIsProvider )
          : OC::PortBase<Worker,Port,ExternalPort> ( w, *this, mPort, argIsProvider,
						     ( 1 << OCPI::RDT::ActiveFlowControl ) |
						     ( 1 << OCPI::RDT::ActiveMessage ),
						     params ),
	    //            remoteIndex ( 0 ),
            m_connection ( 0 ),
            myPortOrdinal ( mPort.m_ordinal )
        {
          m_canBeExternal = true;

          parent().myPorts [ myPortOrdinal ].isConnected = false;
          parent().myPorts [ myPortOrdinal ].dataValueWidthInBytes =
	    OCPI_UTRUNCATE(uint32_t, (mPort.m_dataValueWidth + 7) / 8);
	  //          parent().myPorts [ myPortOrdinal ].attr->optional = mPort.m_optional;

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

          size_t nAlloc =
              OU::roundUp( myDesc.dataBufferPitch * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN) +
              OU::roundUp( myDesc.metaDataPitch * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN) +
              OU::roundUp( sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN) + // local flags
              // These might actually be remote
              OU::roundUp( sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN) + // remote flags
              // These might not be needed if we are ActiveFlowControl
              OU::roundUp( sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          // FIXME how do we set these?
          uint16_t mailbox = 6;
          uint16_t max_mailbox = 10;

          int pid = getpid ( );

          myDesc.oob.port_id = myPortOrdinal;

          snprintf ( myDesc.oob.oep,
                     sizeof ( myDesc.oob.oep ),
                     "ocpi-smb-pio:pioXfer%d:%zu.%hu.%hu",
                     pid,
                     nAlloc,
                     mailbox,
                     max_mailbox );

          uint8_t* allocation = ( uint8_t* ) calloc ( nAlloc,
                                                      sizeof ( uint8_t ) );
          if ( !allocation )
          {
            throw OU::Error( "OCL failed to allocate external port buffers." );
          }

          myDesc.dataBufferBaseAddr = OCPI_UTRUNCATE(DtOsDataTypes::Offset, allocation);
          uint8_t* buffer = allocation;
          allocation += OU::roundUp(myDesc.dataBufferPitch * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          for ( size_t n = 0; n < myDesc.nBuffers; n++ )
          {
            w.device_worker.registerPtr ( (void*)buffer, myDesc.dataBufferPitch );
            buffer += myDesc.dataBufferPitch;
          }

          myDesc.metaDataBaseAddr = OCPI_UTRUNCATE(DtOsDataTypes::Offset, allocation);
          allocation += OU::roundUp(myDesc.metaDataPitch * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          localFlags = (uint32_t*)allocation;
          local = localFlags;
          allocation += OU::roundUp(sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          remoteFlags = (uint32_t*)allocation;
          remote = remoteFlags;
          allocation += OU::roundUp(sizeof(uint32_t) * myDesc.nBuffers, OCLDP_LOCAL_BUFFER_ALIGN);

          farFlags = (uint32_t*)allocation;
          shadow = farFlags;

          myDesc.emptyFlagBaseAddr = 0;
          myDesc.fullFlagBaseAddr = OCPI_UTRUNCATE(DtOsDataTypes::Offset, local);

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
            throw OU::Error("OCL other port's endpoint description wrong: \"%s\"",
			    other.desc.oob.oep);
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
                  throw OU::Error("At consumer, remote buffer size is larger than mine");
                }
                else if (other.desc.dataBufferSize < myDesc.dataBufferSize )
                {
                  throw OU::Error("At producer, remote buffer size smaller than mine");
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
          parent().myPorts [ parent().nConnectedPorts++ ].isConnected = true;
	  return NULL;
        }
        // Connection between two ports inside this container
        // We know they must be in the same artifact, and have a metadata-defined connection
        void connectInside ( OC::Port& provider,
                             const OA::PValue* /*myProps*/,
                             const OA::PValue* /*otherProps*/)
        {
          // We're both in the same runtime artifact object, so we know the port class
          Port& pport = static_cast<Port&>(provider);

          if ( m_connection != pport.m_connection )
          {
            throw OU::Error ( "Ports are both local in artifact, but are not connected");
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
        OU::PortOrdinal portOrdinal ( )
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
        ocpiport->nextLocal->metadata->length = (uint32_t)myPorts [ n ].current.length;
	// FIXME: figure out a way to make the OCL stuff actually 8 bites..
        ocpiport->nextLocal->metadata->opcode = (uint8_t)myPorts [ n ].current.opCode;
        myPorts [ n ].current.data = 0;
        myPorts [ n ].current.maxLength = 0;
        myPorts [ n ].current.length = 0;
        myPorts [ n ].current.opCode = 0;
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
            myPorts [ n ].current.length = length;
            myPorts [ n ].current.opCode = opcode;
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
            myPorts [ n ].current.length = length;
            myPorts [ n ].current.opCode = 0;
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
          ocpiport->currentBuffer->put (myPorts [ n ].current.length,
					(uint8_t)myPorts [ n ].current.opCode, //FIXME
                                         end );
        }
      }
    }

    OC::Port& Worker::createPort ( const OU::Port& metaPort,
                                   const OA::PValue* props )
    {
      return *new Port ( *this, props, metaPort, metaPort.m_provider );
    }

    OC::Port& Worker::createOutputPort ( OU::PortOrdinal portId,
                                         size_t bufferCount,
                                         size_t bufferSize,
                                         const OA::PValue* props )
    throw()
    {
      ( void ) portId;
      ( void ) bufferCount;
      ( void ) bufferSize;
      ( void ) props;
      return *(Port *)0;
    }

    OC::Port& Worker::createInputPort ( OU::PortOrdinal portId,
                                        size_t bufferCount,
                                        size_t bufferSize,
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

        void put ( size_t dataLength,
                   uint8_t opcode,
                   bool endOfData )
        {
          (void)endOfData;
          ocpiAssert(dataLength <= length);
          metadata->opcode = opcode;
          metadata->length = (uint32_t)dataLength;
          release();
        }
    }; // End: class ExternalBuffer

    // Producer or consumer
    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort>
    {
      friend class Port;
      friend class ExternalBuffer;

      private:
      //        uint32_t* flags;
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
          size_t nAlloc =
              OU::roundUp( myDesc.dataBufferPitch * nLocal, OCLDP_LOCAL_BUFFER_ALIGN) +
              OU::roundUp( myDesc.metaDataPitch * nLocal, OCLDP_LOCAL_BUFFER_ALIGN) +
              OU::roundUp( sizeof(uint32_t) * nLocal, OCLDP_LOCAL_BUFFER_ALIGN) + // local flags
              // These might actually be remote
              OU::roundUp( sizeof(uint32_t) * nLocal, OCLDP_LOCAL_BUFFER_ALIGN) + // remote flags
              // These might not be needed if we are ActiveFlowControl
              OU::roundUp( sizeof(uint32_t) * nFar, OCLDP_LOCAL_BUFFER_ALIGN);

          // FIXME where do we get the mailbox information?
          uint16_t mailbox = 7;
          uint16_t max_mailbox = 10;
          int pid = getpid ( );
          myDesc.oob.port_id = port.metaPort().m_ordinal;
          snprintf ( myDesc.oob.oep,
                     sizeof ( myDesc.oob.oep ),
                     "ocpi-smb-pio:pioXfer%d:%zu.%hu.%hu",
                     pid,
                     nAlloc,
                     mailbox,
                     max_mailbox );

          uint8_t* allocation = ( uint8_t* ) calloc ( nAlloc, sizeof ( uint8_t ) );

          if ( !allocation )
          {
            throw OU::Error( "OCL failed to allocate external port buffers." );
          }

          // Resuing the far data buffers
          localData = reinterpret_cast<uint8_t*>(myDesc.dataBufferBaseAddr);
          allocation += OU::roundUp(myDesc.dataBufferPitch * nLocal, OCLDP_LOCAL_BUFFER_ALIGN);

          // Resuing the far metadata buffers
          metadata = reinterpret_cast<OclDpMetadata*>(myDesc.metaDataBaseAddr);
          allocation += OU::roundUp(myDesc.metaDataPitch * nLocal, OCLDP_LOCAL_BUFFER_ALIGN);

          uint32_t* localFlags = (uint32_t*)allocation;
          local = localFlags;
          allocation += OU::roundUp(sizeof(uint32_t) * nLocal, OCLDP_LOCAL_BUFFER_ALIGN);

          uint32_t* remoteFlags = (uint32_t*)allocation;
          remote = remoteFlags;

          allocation += OU::roundUp(sizeof(uint32_t) * nLocal, OCLDP_LOCAL_BUFFER_ALIGN);
          uint32_t* farFlags = (uint32_t*)allocation;
          shadow = farFlags;

          myDesc.emptyFlagBaseAddr = 0;
          myDesc.fullFlagBaseAddr  = OCPI_UTRUNCATE(DtOsDataTypes::Offset, local );

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
                                        size_t& length,
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
                                        size_t& length )
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
        throw OU::Error ( "OCL For external port \"", extName, "\", port \"",
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

