
#include "ocpi_adapter.h"

#include <CpiApi.h>
#include <CpiThread.h>
#include <CpiDriver.h>
#include <CpiPValue.h>
#include <CpiWorker.h>
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <CpiProperty.h>
#include <CpiRDTInterface.h>
#include <CpiContainerPort.h>
#include <CpiUtilException.h>
#include <DtIntEventHandler.h>
#include <CpiOsThreadManager.h>
#include <CpiContainerInterface.h>

#include <cmath>
#include <string>
#include <memory>
#include <cstdio>
#include <iostream>
#include <algorithm>

#define ACQUIRE_WITH_ADC 0

#define BYPASS_PSD 1
#define BYPASS_DELAY 0
#define BYPASS_HDL_FRAMEGATE 1
#define BYPASS_RCC_FRAMEGATE 0

namespace CPI
{
  namespace RPL
  {
    class Driver : public CPI::Util::Driver
    {
      private:

        // The fd for mapped memory, until we have a driver to restrict it.
        static int pciMemFd;

      public:
        // This constructor simply registers itself. This class has no state.
        Driver ( )
          : CPI::Util::Driver ( "OCFRP", "Global", true )
        {
          // Empty
        }

        // This driver method is called when container-discovery happens,
        // to see if there are any container devices supported by this driver
        // It uses a generic PCI scanner to find candidates, and when found,
        // calls the "found" method below.
        virtual unsigned search ( const CPI::Util::PValue*,
                                  const char** exclude )
        throw ( CPI::Util::EmbeddedException )
        {
          ( void ) exclude;
          if ( getenv ( "CPI_OCFRP_DUMMY" ) )
          {
            probe ( 0, "0" );
          }
          return 0;
      }

      // This driver method is called to see if a particular container
      // device exists, and if so, to instantiate a container
      virtual CPI::Util::Device* probe ( const CPI::Util::PValue*,
                                         const char* which )
      throw ( CPI::Util::EmbeddedException );

      virtual ~Driver ( )
        throw ( )
      {
        // Empty
      }
    };

  } // End: namespace RPL
} // End: namespace CPI

namespace
{
  const std::size_t buffer_n_bytes = 4096;
  const std::size_t sw_buffer_count = 2048;
  const std::size_t n_frames_to_drop = 24;

  CPI::Util::DriverManager* get_rcc_driver_manager ( )
  {
    static bool first_time = true;

    static CPI::Util::DriverManager dm ( "Container" );

    if ( first_time )
    {
      first_time = false;
      dm.discoverDevices ( 0, 0 );
    }

    return &dm;
  }

  CPI::Container::Interface* get_rcc_interface ( )
  {
    static CPI::Util::PValue props [ 2 ] = { CPI::Util::PVBool ( "polling", 1 ),
                                             CPI::Util::PVEnd };

    CPI::Util::Device* d =
                      get_rcc_driver_manager ( )->getDevice ( props, "RCC" );

    if ( ! d )
    {
      throw CPI::Util::EmbeddedException ( "No RCC containers found.\n" );
    }

    return static_cast<CPI::Container::Interface*> ( d );
  }

  class DispatchThread : public CPI::Util::Thread
  {
    public:

      DispatchThread ( std::vector<CPI::Container::Interface*>& containers );

      ~DispatchThread ( );

      void run ( );

      void stop ( );

    private:

      volatile bool d_run;
      std::vector<CPI::Container::Interface*>& d_containers;
      DataTransfer::EventManager* d_event_manager;

  }; // End: class DispatchThread : public CPI::Util::Thread


  DispatchThread::DispatchThread (
  std::vector<CPI::Container::Interface*>& containers )
    : CPI::Util::Thread ( ),
      d_run ( true ),
      d_containers ( containers ),
      d_event_manager ( d_containers [ 0 ]->getEventManager ( ) )
  {
    start ( );
  }

  DispatchThread::~DispatchThread ( )
  {
    if ( d_run )
    {
      stop ( );
    }
  }

  void DispatchThread::run ( )
  {
   std::cout << "\n\n DispatchThread is running\n" << std::endl;
    while ( d_run )
    {
      for ( std::vector<CPI::Container::Interface*>::iterator it =
                                                      d_containers.begin ( );
            it != d_containers.end ( );
            ++it )
      {
        CPI::Container::Interface* rcc_container = ( *it );
        // Block here until we get a wakeup notification
        // We will get a notification for the following reasons
        // 1) One of our target buffers has been filled
        // 2) A DMA has completed and we have a source buffer that is now empty
        // 3) A timeout has occured
        if ( d_event_manager )
        {
          do
          {
            rcc_container->dispatch ( d_event_manager );

            if ( d_event_manager->waitForEvent ( 100 ) ==
                 DataTransfer::EventTimeout )
            {
              std::cout << "We have not recieved an event for 100 uSec."
                        << std::endl;
            }
            else
            {
              rcc_container->dispatch ( d_event_manager );
              break;
            }
          }
          while ( 1 );
        }
        else
        {
          rcc_container->dispatch ( d_event_manager );
        }
      }

      CPI::OS::sleep ( 10 );

    } // End: while ( d_run )
  }

  void DispatchThread::stop ( )
  {
    d_run = false;
    join ( );
  }

  template <typename Object, typename MemberFunc, typename Param>
  class ThreadableMemberFunc
  {
    public:

      ThreadableMemberFunc ( Object& instance,
                             MemberFunc member )
        : d_instance ( instance ),
          d_member ( member )
      {
        // Empty
      }

      void run ( CPI::OS::ThreadManager& thread,
                 const Param& param )
      {
        ThreadArg* thread_arg = new ThreadArg ( d_instance,
                                                d_member,
                                                param );
        try
        {
          thread.start ( &thread_func,
                         reinterpret_cast<void*> ( thread_arg ) );
        }
        catch ( ... )
        {
          delete thread_arg;
          throw;
        }
      }

    private:

      Object& d_instance;

      MemberFunc  d_member;

      struct ThreadArg
      {
        Object* object;
        MemberFunc member_func;
        Param param;

        ThreadArg ( Object& o,
                    MemberFunc m,
                    const Param& p )
          : object( &o ),
            member_func ( m ),
            param ( p )
        {
          // Empty
        }
      };

      static void thread_func ( void* v )
      {
        std::auto_ptr<ThreadArg> arg ( reinterpret_cast<ThreadArg*> ( v ) );

        ( ( arg->object )->*( arg->member_func ) ) ( arg->param );
      }
  };

  // Provided to support automatic detection of types
  template <typename Object, typename MemberFunc, typename Param>
  inline void run_in_thread ( CPI::OS::ThreadManager& thread,
                              Object& instance,
                              MemberFunc member,
                              const Param& param )
  {
    ThreadableMemberFunc<Object,MemberFunc,Param> m ( instance, member );
    m.run ( thread, param );
  }

  class XmBhAdapter
  {
    public:

      explicit XmBhAdapter ( );

      ~XmBhAdapter ( );

      void get_undelayed_buffer ( uint8_t* buffer, std::size_t n_bytes );

      void get_delayed_buffer ( uint8_t* buffer, std::size_t n_bytes );

      void put_buffer ( const uint8_t* buffer, std::size_t n_bytes );

      void counter_measure ( double frequency );

    private:

      CPI::Container::ExternalPort* d_ex_delayed;

      CPI::Container::ExternalPort* d_ex_undelayed;

      CPI::Container::ExternalPort* d_target_port; // XM to OCPI

      std::size_t d_dumped;

      CPI::OS::Mutex d_buf_lock_adc;
      CPI::OS::Mutex d_buf_lock_delay;

      CPI::OS::ThreadManager d_thread;

      CPI::OS::ThreadManager d_dumper_thread;

      volatile bool d_done;

      volatile bool d_do_cm;

      void run_app ( void* arg );

      void dumper ( void* arg );
  };

  XmBhAdapter::XmBhAdapter ( )
    : d_ex_delayed ( 0 ),
      d_ex_undelayed ( 0 ),
      d_target_port ( 0 ),
      d_dumped ( 0 ),
      d_buf_lock_adc ( false ),
      d_buf_lock_delay ( false ),
      d_thread ( ),
      d_dumper_thread ( ),
      d_done ( false ),
      d_do_cm ( false )
  {
    run_in_thread ( d_thread, *this, &XmBhAdapter::run_app, ( void* ) 0 );
    run_in_thread ( d_dumper_thread, *this, &XmBhAdapter::dumper, ( void* ) 0 );
  }

  XmBhAdapter::~XmBhAdapter ( )
  {
    d_done = true;
    d_thread.join ( );
    d_dumper_thread.join ( );
  }

  void XmBhAdapter::get_undelayed_buffer ( uint8_t* xm_buffer,
                                     std::size_t n_bytes )
  {
    CPI::Util::AutoMutex guard ( d_buf_lock_adc, true );

    uint8_t opcode ( 0 );
    uint8_t* ocpi_buffer ( 0 );
    uint32_t ocpi_n_bytes ( 0 );

    bool is_end_of_data ( false );

    while ( !d_ex_undelayed )
    {
      CPI::OS::sleep ( 0 );
    }

    CPI::Container::ExternalBuffer* ex_buffer  ( 0 );

    ex_buffer = d_ex_undelayed->getBuffer ( opcode,
                                             ocpi_buffer,
                                             ocpi_n_bytes,
                                             is_end_of_data );
    if ( ex_buffer )
    {
      if ( xm_buffer )
      {
        ::memcpy ( ( void* ) xm_buffer, ocpi_buffer, n_bytes );
      }

      ex_buffer->release ( );

      if ( ocpi_n_bytes != n_bytes )
      {
        std::cout << __PRETTY_FUNCTION__
                  << "() buffer size mismatch expected "
                  << n_bytes
                  << " got "
                  << ocpi_n_bytes
                  << std::endl;
      }
    }
  }

  void XmBhAdapter::get_delayed_buffer ( uint8_t* xm_buffer,
                                       std::size_t n_bytes )
  {
    CPI::Util::AutoMutex guard ( d_buf_lock_delay, true );

    uint8_t opcode ( 0 );
    uint8_t* ocpi_buffer ( 0 );
    uint32_t ocpi_n_bytes ( 0 );

    bool is_end_of_data ( false );

    while ( !d_ex_delayed )
    {
      CPI::OS::sleep ( 0 );
    }

    CPI::Container::ExternalBuffer* ex_buffer  ( 0 );

    ex_buffer = d_ex_delayed->getBuffer ( opcode,
                                           ocpi_buffer,
                                           ocpi_n_bytes,
                                           is_end_of_data );
    if ( ex_buffer )
    {
      if ( xm_buffer )
      {
        ::memcpy ( ( void* ) xm_buffer, ocpi_buffer, n_bytes );
      }

      ex_buffer->release ( );

      if ( ocpi_n_bytes != n_bytes )
      {
        std::cout << __PRETTY_FUNCTION__
                  << "() buffer size mismatch expected "
                  << n_bytes
                  << " got "
                  << ocpi_n_bytes
                  << std::endl;
      }
    }
  }

  void XmBhAdapter::put_buffer ( const uint8_t* xm_buffer,
                                 std::size_t n_bytes )
  {
    while ( !d_target_port )
    {
      CPI::OS::sleep ( 0 );
    }

    uint8_t opcode ( 0 );
    uint8_t* ocpi_buffer ( 0 );
    uint32_t ocpi_n_bytes ( 0 );

    bool is_end_of_data ( false );

    CPI::Container::ExternalBuffer* ex_buffer  ( 0 );

    ex_buffer = d_target_port->getBuffer ( ocpi_buffer,
                                           ocpi_n_bytes );

    if ( ex_buffer )
    {
      if ( ocpi_n_bytes != n_bytes )
      {
        std::cout << __PRETTY_FUNCTION__
                  << "() buffer size mismatch expected "
                  << n_bytes
                  << " got "
                  << ocpi_n_bytes
                  << std::endl;
      }

      ::memcpy ( ocpi_buffer, ( void* ) xm_buffer, n_bytes );

      ex_buffer->put ( opcode, n_bytes, is_end_of_data );
    }
  }

  void XmBhAdapter::dumper ( void* arg )
  {
    ( void ) arg;

    try
    {
      while ( !d_done )
      {
        get_undelayed_buffer ( 0, buffer_n_bytes );

        get_delayed_buffer ( 0, buffer_n_bytes );

        d_dumped++;

      } // End: while ( !d_done )
    }
    catch ( const std::string& s )
    {
      std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
    }
    catch ( const CPI::Util::EmbeddedException& e )
    {
      std::cerr << "\nException(e): " << e.getAuxInfo ( ) << "\n" << std::endl;
    }
    catch ( ... )
    {
      std::cerr << "\n\nException(u): unknown\n" << std::endl;
    }

    return;
  }

  void XmBhAdapter::counter_measure ( double frequency )
  {
    d_do_cm = true;
    std::cout << "OCPI: Performed CM at frequency " << frequency << std::endl;
  }

  /* ---- Wrapper to simplify using RCC and HDL workers ------------------ */

  class WorkerFacade
  {
    public:

      WorkerFacade ( CPI::Container::Interface* container,
                     CPI::Container::Application* application )
        : d_container ( container ),
          d_application ( application )
      {
        // Empty
      }

      virtual ~WorkerFacade ( )
      {
        // Empty
      }

      CPI::Container::Interface* interface ( ) const
      {
        return d_container;
      }

      virtual void print_status ( ) = 0;

      virtual void set_properties ( ) = 0;

      virtual CPI::Container::Worker* worker ( ) const = 0;

    protected:

      CPI::Container::Interface* d_container;

      CPI::Container::Application* d_application;

  }; // End: class WorkerFacade

  CPI::Util::PValue rcc_worker_props [ 2 ] =
  {
    CPI::Util::PVString ( "DLLEntryPoint", "ocpi_EntryTable" ),
    CPI::Util::PVEnd
  };

  class RccFacade : public WorkerFacade
  {
    public:

      RccFacade ( CPI::Container::Interface* container,
                  CPI::Container::Application* application,
                  const char* uri,
                  const char* worker_name )
        : WorkerFacade ( container, application ),
          d_artifact ( d_application->loadArtifact ( uri ) ),
          d_worker ( d_application->createWorker ( d_artifact,
                                                   worker_name,
                                                   0,
                                                   rcc_worker_props ) )
      {
        d_worker.initialize ( );
      }

      virtual ~RccFacade ( )
      {
        // Empty
      }

      CPI::Container::Worker* worker ( ) const
      {
        return &d_worker;
      }

    protected:

      CPI::Container::Artifact& d_artifact;

      CPI::Container::Worker& d_worker;

  }; // End: class RccFacade : public WorkerFacade

  class RccFrameGate : public RccFacade
  {
    public:

      enum
      {
        BYPASS = 0,
        GATE = 1
      };

      RccFrameGate ( CPI::Container::Interface* container,
                     CPI::Container::Application* application,
                     const char* uri,
                     const char* worker_name,
                     uint32_t frame_gate_control,
                     uint32_t frame_n_bytes,
                     uint32_t gate_n_bytes )
        : RccFacade ( container, application, uri, worker_name ),
          d_port_wsi_in ( d_worker.getPort ( "in" ) ),
          d_port_wsi_out ( d_worker.getPort ( "out" ) ),
          d_port_ex_out ( 0 )
      {
#if BYPASS_RCC_FRAMEGATE
        ( void ) frame_gate_control;
        ( void ) frame_n_bytes;
        ( void ) gate_n_bytes;
        CPI::Container::Property frameGateCtrl ( d_worker, "frameGateCtrl" );
        frameGateCtrl.setULongValue ( BYPASS );
#else
        CPI::Container::Property frameGateCtrl ( d_worker, "frameGateCtrl" );
        CPI::Container::Property frameSize ( d_worker, "frameSize" );
        CPI::Container::Property gateSize ( d_worker, "gateSize" );

        frameGateCtrl.setULongValue ( frame_gate_control );
        frameSize.setULongValue ( frame_n_bytes );
        gateSize.setULongValue ( gate_n_bytes );
#endif

      }

      CPI::Container::Port& wsi_in ( ) const
      {
        return d_port_wsi_in;
      }

      CPI::Container::Port& wsi_out ( ) const
      {
        return d_port_wsi_out;
      }

      CPI::Container::ExternalPort* external_target_port (
                                    CPI::Container::Port& port,
                                    const char* name ) const
      {
        if ( !d_port_ex_out )
        {
          CPI::Util::PValue p01 [ 4 ] =
          {
            CPI::Util::PVULong ( "bufferCount", sw_buffer_count ),
            CPI::Util::PVString ( "xferRole", "flowcontrol" ),
            CPI::Util::PVULong ( "bufferSize", buffer_n_bytes ),
            CPI::Util::PVEnd
          };

          CPI::Util::PValue p10 [ 4 ] =
          {
            CPI::Util::PVULong ( "bufferCount", 2 ),
            CPI::Util::PVString ( "xferRole", "active" ),
            CPI::Util::PVULong ( "bufferSize", buffer_n_bytes ),
            CPI::Util::PVEnd
          };

          d_port_ex_out =
                  &( port.connectExternal ( name,
                                            p01, // External
                                            p10 ) ); // to port

        } // End: if ( !d_port_ex_out )

        return d_port_ex_out;
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        // Empty
      }

      virtual ~RccFrameGate ( )
      {
        // Empty
      }

    private:

      CPI::Container::Port& d_port_wsi_in;

      CPI::Container::Port& d_port_wsi_out;

      mutable CPI::Container::ExternalPort* d_port_ex_out;

  }; // End: class RccFrameGate : public RccFacade

  class HdlFacade : public WorkerFacade
  {
    public:

      HdlFacade ( CPI::Container::Interface* container,
                  CPI::Container::Application* application,
                  const char* uri,
                  const char* worker_name,
                  const char* impl_name )
        : WorkerFacade ( container, application ),
          d_worker ( d_application->createWorker ( uri,
                                                   0,
                                                   worker_name,
                                                   impl_name ) )
      {
        // Empty
      }

      virtual ~HdlFacade ( )
      {
        // Empty
      }

      CPI::Container::Worker* worker ( ) const
      {
        return &d_worker;
      }

    protected:

      CPI::Container::Worker& d_worker;

  }; // End: class HdlFacade : public WorkerFacade

  class HdlSMA : public HdlFacade
  {
    public:

      enum
      {
        PASS_WSI_IN_TO_WSI_OUT = 0,
        PASS_WMI_DMA_IN_TO_WSI_OUT = 1,
        PASS_WSI_IN_TO_WMI_DMA_OUT = 2,
        SPLIT_WSI_IN_TO_WSI_OUT_AND_WMI_DMA_OUT = 3,
      };

      HdlSMA ( const char* msg,
               CPI::Container::Interface* container,
               CPI::Container::Application* application,
               const char* uri,
               const char* worker_name,
               const char* impl_name,
               uint32_t control )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg ),
          d_port_wsi_in ( 0 ),
          d_port_wsi_out ( 0 ),
          d_port_wmi_in ( 0),
          d_port_wmi_out ( 0),
          d_port_ex_out ( 0 )
      {
        CPI::Container::Property pfc ( d_worker, "control" );

        pfc.setULongValue ( control );
      }

      virtual ~HdlSMA ( )
      {
        // Empty
      }

      CPI::Container::Port& wsi_in ( ) const
      {
        if ( !d_port_wsi_in )
        {
          d_port_wsi_in = &( d_worker.getPort ( "WSIin" ) );
        }

        return *d_port_wsi_in;
      }

      CPI::Container::Port& wsi_out ( ) const
      {
        if ( !d_port_wsi_out )
        {
          d_port_wsi_out = &( d_worker.getPort ( "WSIout" ) );
        }

        return *d_port_wsi_out;
      }

      CPI::Container::Port& wmi_in ( ) const
      {
        if ( !d_port_wmi_in )
        {
          d_port_wmi_in = &( d_worker.getPort ( "WMIin" ) );
        }

        return *d_port_wmi_in;
      }

      CPI::Container::Port& wmi_out ( ) const
      {
        if ( !d_port_wmi_out )
        {
          d_port_wmi_out = &( d_worker.getPort ( "WMIout" ) );
        }

        return *d_port_wmi_out;
      }

      CPI::Container::ExternalPort* external_target_port (
                                    CPI::Container::Port& port,
                                    const char* name ) const
      {
        if ( !d_port_ex_out )
        {
          CPI::Util::PValue p01 [ 4 ] =
          {
            CPI::Util::PVULong ( "bufferCount", sw_buffer_count ),
            CPI::Util::PVString ( "xferRole", "flowcontrol" ),
            CPI::Util::PVULong ( "bufferSize", buffer_n_bytes ),
            CPI::Util::PVEnd
          };

          CPI::Util::PValue p10 [ 4 ] =
          {
            CPI::Util::PVULong ( "bufferCount", 2 ),
            CPI::Util::PVString ( "xferRole", "active" ),
            CPI::Util::PVULong ( "bufferSize", buffer_n_bytes ),
            CPI::Util::PVEnd
          };

          d_port_ex_out = &( port.connectExternal ( name,
                                                    p01, // External
                                                    p10 ) ); // to port
        } // End: if ( !d_port_ex_out )

        return d_port_ex_out;
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        CPI::Container::Property control ( d_worker, "control" );
        CPI::Container::Property mesgCount ( d_worker, "mesgCount" );
        CPI::Container::Property abortCount ( d_worker, "abortCount" );
        CPI::Container::Property dummy0 ( d_worker, "dummy0" );
        CPI::Container::Property thisMesg ( d_worker, "thisMesg" );
        CPI::Container::Property lastMesg ( d_worker, "lastMesg" );
        CPI::Container::Property portStatus ( d_worker, "portStatus" );
        CPI::Container::Property WSIpMesgCount ( d_worker, "WSIpMesgCount" );
        CPI::Container::Property WSIiMesgCount ( d_worker, "WSIiMesgCount" );
        CPI::Container::Property WMIpMesgCount ( d_worker, "WMIpMesgCount" );
        CPI::Container::Property WMIiMesgCount ( d_worker, "WMIiMesgCount" );

        printf ( "\n\nSMA worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "control       0x%08x\n", control.getULongValue ( ) );
        printf ( "mesgCount     0x%08x\n", mesgCount.getULongValue ( ) );
        printf ( "abortCount    0x%08x\n", abortCount.getULongValue ( ) );
        printf ( "dummy0        0x%08x\n", dummy0.getULongValue ( ) );
        printf ( "thisMesg      0x%08x\n", thisMesg.getULongValue ( ) );
        printf ( "lastMesg      0x%08x\n", lastMesg.getULongValue ( ) );
        printf ( "portStatus    0x%08x\n", portStatus.getULongValue ( ) );
        printf ( "WSIpMesgCount 0x%08x\n", WSIpMesgCount.getULongValue ( ) );
        printf ( "WSIiMesgCount 0x%08x\n", WSIiMesgCount.getULongValue ( ) );
        printf ( "WMIpMesgCount 0x%08x\n", WMIpMesgCount.getULongValue ( ) );
        printf ( "WMIiMesgCount 0x%08x\n", WMIiMesgCount.getULongValue ( ) );
      }

    private:

      std::string d_msg;

      mutable CPI::Container::Port* d_port_wsi_in;
      mutable CPI::Container::Port* d_port_wsi_out;

      mutable CPI::Container::Port* d_port_wmi_in;
      mutable CPI::Container::Port* d_port_wmi_out;

      mutable CPI::Container::ExternalPort* d_port_ex_out;

  }; // End: class HdlSMASplit : public HdlFacade

  class HdlDac : public HdlFacade
  {
    public:

      HdlDac ( const char* msg,
               CPI::Container::Interface* container,
               CPI::Container::Application* application,
               const char* uri,
               const char* worker_name,
               const char* impl_name )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg ),
          d_port_wsi_in ( d_worker.getPort ( "WSIin" ) )
      {
        // Empty
      }

      virtual ~HdlDac ( )
      {
        // Empty
      }

      CPI::Container::Port& wsi_in ( ) const
      {
        return d_port_wsi_in;
      }

      void counter_measure ( )
      {
        d_worker.start ( );
        sleep ( 1 );
        d_worker.stop ( );
      }

      void set_properties ( )
      {
        CPI::Container::Property dacControl ( d_worker, "dacControl" );
        // Emit CM when started.
        dacControl.setULongValue ( 0x19 );
      }

      void print_status ( )
      {
        CPI::Container::Property dacStatusMS ( d_worker, "dacStatusMS" );
        CPI::Container::Property dacStatusLS ( d_worker, "dacStatusLS" );
        CPI::Container::Property dummy0 ( d_worker, "dummy0" );
        CPI::Container::Property dacControl ( d_worker, "dacControl" );
        CPI::Container::Property fcDac ( d_worker, "fcDac" );
        CPI::Container::Property dacSampleDeq ( d_worker, "dacSampleDeq" );
        CPI::Container::Property mesgCount ( d_worker, "mesgCount" );
        CPI::Container::Property samplesSinceSync ( d_worker, "samplesSinceSync" );
        CPI::Container::Property underflowCount ( d_worker, "underflowCount" );
        CPI::Container::Property lastUnderflowMesg ( d_worker, "lastUnderflowMesg" );
        CPI::Container::Property popCount ( d_worker, "popCount" );
        CPI::Container::Property unrollCount ( d_worker, "unrollCount" );
        CPI::Container::Property syncCount ( d_worker, "syncCount" );
        CPI::Container::Property mesgStart ( d_worker, "mesgStart" );
        CPI::Container::Property underFlowCount ( d_worker, "underFlowCount" );
        CPI::Container::Property stageCount ( d_worker, "stageCount" );
        CPI::Container::Property dummy1 ( d_worker, "dummy1" );
        CPI::Container::Property dummy2 ( d_worker, "dummy2" );
        CPI::Container::Property extStatus_pMesgCount ( d_worker, "extStatus_pMesgCount" );
        CPI::Container::Property extStatus_iMesgCount ( d_worker, "extStatus_iMesgCount" );

        printf ( "\n\nDAC worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "dacStatusMS          0x%08x\n", dacStatusMS.getULongValue ( ) );
        printf ( "dacStatusLS          0x%08x\n", dacStatusLS.getULongValue ( ) );
        printf ( "dummy0               0x%08x\n", dummy0.getULongValue ( ) );
        printf ( "dacControl           0x%08x\n", dacControl.getULongValue ( ) );
        printf ( "fcDac                0x%08x\n", fcDac.getULongValue ( ) );
        printf ( "dacSampleDeq         0x%08x\n", dacSampleDeq.getULongValue ( ) );
        printf ( "mesgCount            0x%08x\n", mesgCount.getULongValue ( ) );
        printf ( "samplesSinceSync     0x%08x\n", samplesSinceSync.getULongValue ( ) );
        printf ( "underflowCount       0x%08x\n", underflowCount.getULongValue ( ) );
        printf ( "lastUnderflowMesg    0x%08x\n", lastUnderflowMesg.getULongValue ( ) );
        printf ( "popCount             0x%08x\n", popCount.getULongValue ( ) );
        printf ( "unrollCount          0x%08x\n", unrollCount.getULongValue ( ) );
        printf ( "syncCount            0x%08x\n", syncCount.getULongValue ( ) );
        printf ( "mesgStart            0x%08x\n", mesgStart.getULongValue ( ) );
        printf ( "underFlowCount       0x%08x\n", underFlowCount.getULongValue ( ) );
        printf ( "stageCount           0x%08x\n", stageCount.getULongValue ( ) );
        printf ( "dummy1               0x%08x\n", dummy1.getULongValue ( ) );
        printf ( "dummy2               0x%08x\n", dummy2.getULongValue ( ) );
        printf ( "extStatus_pMesgCount 0x%08x\n", extStatus_pMesgCount.getULongValue ( ) );
        printf ( "extStatus_iMesgCount 0x%08x\n", extStatus_iMesgCount.getULongValue ( ) );
      }

    private:

      std::string d_msg;

      CPI::Container::Port& d_port_wsi_in;

  }; // End: class HdlDac : public HdlFacade

  class HdlAdc : public HdlFacade
  {
    public:

      HdlAdc ( const char* msg,
               CPI::Container::Interface* container,
               CPI::Container::Application* application,
               const char* uri,
               const char* worker_name,
               const char* impl_name )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg ),
          d_port_adc_out ( d_worker.getPort ( "WSIout" ) ),
          d_stats_dwellFails ( d_worker, "stats_dwellFails" )
      {
        CPI::Container::Property maxMesgLength ( d_worker, "maxMesgLength" );

        maxMesgLength.setULongValue ( buffer_n_bytes );

#if ACQUIRE_WITH_ADC
        CPI::Container::Property spiAdc0 ( d_worker, "spiAdc0" );

        // Normal acquisition
        spiAdc0.setULongValue ( 0x6200 );
#endif
      }

      virtual ~HdlAdc ( )
      {
        // Empty
      }

      CPI::Container::Port& adc_out ( ) const
      {
        return d_port_adc_out;
      }

      uint32_t stats_dwell_fails ( )
      {
        return d_stats_dwellFails.getULongValue ( );
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        CPI::Container::Property wsiMStatus ( d_worker, "wsiMStatus" );
        CPI::Container::Property adcStatusLS ( d_worker, "adcStatusLS" );
        CPI::Container::Property maxMesgLength ( d_worker, "maxMesgLength" );
        CPI::Container::Property adcControl ( d_worker, "adcControl" );
        CPI::Container::Property fcAdc ( d_worker, "fcAdc" );
        CPI::Container::Property adcSampleEng ( d_worker, "adcSampleEng" );
        CPI::Container::Property sampleSpy0 ( d_worker, "sampleSpy0" );
        CPI::Container::Property sampleSpy1 ( d_worker, "sampleSpy1" );
        CPI::Container::Property spiResponse ( d_worker, "spiResponse" );
        CPI::Container::Property mesgCount ( d_worker, "mesgCount" );
        CPI::Container::Property stats_dwellStarts ( d_worker, "stats_dwellStarts" );
        CPI::Container::Property stats_dwellFails ( d_worker, "stats_dwellFails" );
        CPI::Container::Property lastOverflowMesg ( d_worker, "lastOverflowMesg" );
        CPI::Container::Property extStatus_pMesgCount ( d_worker, "extStatus_pMesgCount" );
        CPI::Container::Property extStatus_iMesgCount ( d_worker, "extStatus_iMesgCount" );
        CPI::Container::Property stats_dropCount ( d_worker, "stats_dropCount" );
        CPI::Container::Property dwellFails ( d_worker, "dwellFails" );

        printf ( "\n\nADC worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "wsiMStatus           0x%08x\n", wsiMStatus.getULongValue ( ) );
        printf ( "adcStatusLS          0x%08x\n", adcStatusLS.getULongValue ( ) );
        printf ( "maxMesgLength        0x%08x\n", maxMesgLength.getULongValue ( ) );
        printf ( "adcControl           0x%08x\n", adcControl.getULongValue ( ) );
        printf ( "fcAdc                0x%08x\n", fcAdc.getULongValue ( ) );
        printf ( "adcSampleEng         0x%08x\n", adcSampleEng.getULongValue ( ) );
        printf ( "sampleSpy0           0x%08x\n", sampleSpy0.getULongValue ( ) );
        printf ( "sampleSpy1           0x%08x\n", sampleSpy1.getULongValue ( ) );
        printf ( "spiResponse          0x%08x\n", spiResponse.getULongValue ( ) );
        printf ( "mesgCount            0x%08x\n", mesgCount.getULongValue ( ) );
        printf ( "stats_dwellStarts    0x%08x\n", stats_dwellStarts.getULongValue ( ) );
        printf ( "stats_dwellFails     0x%08x\n", stats_dwellFails.getULongValue ( ) );
        printf ( "lastOverflowMesg     0x%08x\n", lastOverflowMesg.getULongValue ( ) );
        printf ( "extStatus_pMesgCount 0x%08x\n", extStatus_pMesgCount.getULongValue ( ) );
        printf ( "extStatus_iMesgCount 0x%08x\n", extStatus_iMesgCount.getULongValue ( ) );
        printf ( "stats_dropCount      0x%08x\n", stats_dropCount.getULongValue ( ) );
        printf ( "dwellFails           0x%08x\n", dwellFails.getULongValue ( ) );
      }

    private:

      std::string d_msg;

      CPI::Container::Port& d_port_adc_out;

      CPI::Container::Property d_stats_dwellFails;

  }; // End: class HdlAdc : public HdlFacade

  class HdlDelay : public HdlFacade
  {
    public:

      HdlDelay ( const char* msg,
                 CPI::Container::Interface* container,
                 CPI::Container::Application* application,
                 const char* uri,
                 const char* worker_name,
                 const char* impl_name )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg ),
          d_port_in ( d_worker.getPort ( "WSIin" ) ),
          d_port_out ( d_worker.getPort ( "WSIout" ) )
      {
#if BYPASS_DELAY
        CPI::Container::Property dlyCtrl ( d_worker, "dlyCtrl" );

        dlyCtrl.setULongValue ( 0 ); // No delay
#else
        CPI::Container::Property dlyCtrl ( d_worker, "dlyCtrl" );

        dlyCtrl.setULongValue ( 7 ); // Delay

        CPI::Container::Property dlyHoldoffBytes ( d_worker, "dlyHoldoffBytes" );
        CPI::Container::Property dlyHoldoffCycles ( d_worker, "dlyHoldoffCycles" );

        dlyHoldoffBytes.setULongValue ( 0 );

        dlyHoldoffCycles.setULongValue ( 100 * 1000 * 1000 );
#endif
      }

      virtual ~HdlDelay ( )
      {
        // Empty
      }

      CPI::Container::Port& wsi_in ( ) const
      {
        return d_port_in;
      }

      CPI::Container::Port& wsi_out ( ) const
      {
        return d_port_out;
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        CPI::Container::Property dlyCtrl ( d_worker, "dlyCtrl" );
        CPI::Container::Property dlyHoldoffBytes ( d_worker, "dlyHoldoffBytes" );
        CPI::Container::Property dlyHoldoffCycles ( d_worker, "dlyHoldoffCycles" );
        CPI::Container::Property mesgWtCount ( d_worker, "mesgWtCount" );
        CPI::Container::Property mesgRdCount ( d_worker, "mesgRdCount" );
        CPI::Container::Property bytesWritten ( d_worker, "bytesWritten" );
        CPI::Container::Property portStatus ( d_worker, "portStatus" );

        CPI::Container::Property dummy ( d_worker, "dummy" );
        CPI::Container::Property WSI_S_pMesgCount ( d_worker, "WSI_S_pMesgCount" );
        CPI::Container::Property WSI_S_iMesgCount ( d_worker, "WSI_S_iMesgCount" );
        CPI::Container::Property WSI_S_tBusyCount ( d_worker, "WSI_S_tBusyCount" );
        CPI::Container::Property WSI_M_pMesgCount ( d_worker, "WSI_M_pMesgCount" );
        CPI::Container::Property WSI_M_iMesgCount ( d_worker, "WSI_M_iMesgCount" );
        CPI::Container::Property WSI_M_tBusyCount ( d_worker, "WSI_M_tBusyCount" );
        CPI::Container::Property wmemiWrReq ( d_worker, "wmemiWrReq" );
        CPI::Container::Property wmemiRdReq ( d_worker, "wmemiRdReq" );
        CPI::Container::Property wmemiRdResp ( d_worker, "wmemiRdResp" );
        CPI::Container::Property dlyWordsStored ( d_worker, "dlyWordsStored" );
        CPI::Container::Property dlyRdCredit ( d_worker, "dlyRdCredit" );
        CPI::Container::Property dlyWAG ( d_worker, "dlyWAG" );
        CPI::Container::Property dlyRAG ( d_worker, "dlyRAG" );

        printf ( "\n\nDelay worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "dlyCtrl          0x%08x\n", dlyCtrl.getULongValue ( ) );
        printf ( "dlyHoldoffBytes  0x%08x\n", dlyHoldoffBytes.getULongValue ( ) );
        printf ( "dlyHoldoffCycles 0x%08x\n", dlyHoldoffCycles.getULongValue ( ) );
        printf ( "mesgWtCount      0x%08x\n", mesgWtCount.getULongValue ( ) );
        printf ( "mesgRdCount      0x%08x\n", mesgRdCount.getULongValue ( ) );
        printf ( "bytesWritten     0x%08x\n", bytesWritten.getULongValue ( ) );
        printf ( "portStatus       0x%08x\n", portStatus.getULongValue ( ) );
        printf ( "dummy            0x%08x\n", dummy.getULongValue ( ) );
        printf ( "WSI_S_pMesgCount 0x%08x\n", WSI_S_pMesgCount.getULongValue ( ) );
        printf ( "WSI_S_iMesgCount 0x%08x\n", WSI_S_iMesgCount.getULongValue ( ) );
        printf ( "WSI_S_tBusyCount 0x%08x\n", WSI_S_tBusyCount.getULongValue ( ) );
        printf ( "WSI_M_pMesgCount 0x%08x\n", WSI_M_pMesgCount.getULongValue ( ) );
        printf ( "WSI_M_iMesgCount 0x%08x\n", WSI_M_iMesgCount.getULongValue ( ) );
        printf ( "WSI_M_tBusyCount 0x%08x\n", WSI_M_tBusyCount.getULongValue ( ) );
        printf ( "wmemiWrReq       0x%08x\n", wmemiWrReq.getULongValue ( ) );
        printf ( "wmemiRdReq       0x%08x\n", wmemiRdReq.getULongValue ( ) );
        printf ( "wmemiRdResp      0x%08x\n", wmemiRdResp.getULongValue ( ) );
        printf ( "dlyWordsStored   0x%08x\n", dlyWordsStored.getULongValue ( ) );
        printf ( "dlyRdCredit      0x%08x\n", dlyRdCredit.getULongValue ( ) );
        printf ( "dlyWAG           0x%08x\n", dlyWAG.getULongValue ( ) );
        printf ( "dlyRAG           0x%08x\n\n", dlyRAG.getULongValue ( ) );
      }

    private:

      std::string d_msg;

      CPI::Container::Port& d_port_in;
      CPI::Container::Port& d_port_out;

  }; // End: class HdlDelay : public HdlFacade

  class HdlDramServer : public HdlFacade
  {
    public:

      HdlDramServer ( const char* msg,
                      CPI::Container::Interface* container,
                      CPI::Container::Application* application,
                      const char* uri,
                      const char* worker_name,
                      const char* impl_name )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg )
      {
        while ( status ( ) == 0 )
        {
          CPI::OS::sleep ( 100000 );
        }
      }

      virtual ~HdlDramServer ( )
      {
        // Empty
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        CPI::Container::Property dramStatus ( d_worker, "dramStatus" );
        CPI::Container::Property drmCtrl ( d_worker, "drmCtrl" );

        CPI::Container::Property dbg_calib_done ( d_worker, "dbg_calib_done" );
        CPI::Container::Property dbg_calib_err ( d_worker, "dbg_calib_err" );
        CPI::Container::Property dbg_calib_dq_tap_cnt ( d_worker, "dbg_calib_dq_tap_cnt" );
        CPI::Container::Property dbg_calib_dqs_tap_cnt ( d_worker, "dbg_calib_dqs_tap_cnt" );
        CPI::Container::Property dbg_calib_gate_tap_cnt ( d_worker, "dbg_calib_gate_tap_cnt" );
        CPI::Container::Property dbg_calib_rd_data_sel ( d_worker, "dbg_calib_rd_data_sel" );
        CPI::Container::Property dbg_calib_ren_delay ( d_worker, "dbg_calib_ren_delay" );
        CPI::Container::Property dbg_calib_gate_delay ( d_worker, "dbg_calib_gate_delay" );
        CPI::Container::Property hcode_babe ( d_worker, "32hcode_babe" );
        CPI::Container::Property wmemiWrReq ( d_worker, "wmemiWrReq" );
        CPI::Container::Property wmemiRdReq ( d_worker, "wmemiRdReq" );
        CPI::Container::Property wmemiRdResp ( d_worker, "wmemiRdResp" );
        CPI::Container::Property wmemi_status ( d_worker, "wmemi_status" );
        CPI::Container::Property wmemi_ReadInFlight ( d_worker, "wmemi_ReadInFlight" );
        CPI::Container::Property dummy0 ( d_worker, "dummy0" );
        CPI::Container::Property dummy1 ( d_worker, "dummy1" );
        CPI::Container::Property requestCount ( d_worker, "requestCount" );
        CPI::Container::Property dummy2 ( d_worker, "dummy2" );
        CPI::Container::Property pReg ( d_worker, "pReg" );
        CPI::Container::Property mReg ( d_worker, "mReg" );
        CPI::Container::Property wdReg_0 ( d_worker, "wdReg_0" );
        CPI::Container::Property wdReg_1 ( d_worker, "wdReg_1" );
        CPI::Container::Property wdReg_2 ( d_worker, "wdReg_2" );
        CPI::Container::Property wdReg_3 ( d_worker, "wdReg_3" );
        CPI::Container::Property rdReg_0 ( d_worker, "rdReg_0" );
        CPI::Container::Property rdReg_1 ( d_worker, "rdReg_1" );
        CPI::Container::Property rdReg_2 ( d_worker, "rdReg_2" );
        CPI::Container::Property rdReg_3 ( d_worker, "rdReg_3" );

        printf ( "\n\nDramServer worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "dramStatus             0x%08x\n", dramStatus.getULongValue ( ) );
        printf ( "drmCtrl                0x%08x\n", drmCtrl.getULongValue ( ) );
        printf ( "dbg_calib_done         0x%08x\n", dbg_calib_done.getULongValue ( ) );
        printf ( "dbg_calib_err          0x%08x\n", dbg_calib_err.getULongValue ( ) );
        printf ( "dbg_calib_dq_tap_cnt   0x%08x\n", dbg_calib_dq_tap_cnt.getULongValue ( ) );
        printf ( "dbg_calib_dqs_tap_cnt  0x%08x\n", dbg_calib_dqs_tap_cnt.getULongValue ( ) );
        printf ( "dbg_calib_gate_tap_cnt 0x%08x\n", dbg_calib_gate_tap_cnt.getULongValue ( ) );
        printf ( "dbg_calib_rd_data_sel  0x%08x\n", dbg_calib_rd_data_sel.getULongValue ( ) );
        printf ( "dbg_calib_ren_delay    0x%08x\n", dbg_calib_ren_delay.getULongValue ( ) );
        printf ( "dbg_calib_gate_delay   0x%08x\n", dbg_calib_gate_delay.getULongValue ( ) );
        printf ( "32hcode_babe           0x%08x\n", hcode_babe.getULongValue ( ) );
        printf ( "wmemiWrReq             0x%08x\n", wmemiWrReq.getULongValue ( ) );
        printf ( "wmemiRdReq             0x%08x\n", wmemiRdReq.getULongValue ( ) );
        printf ( "wmemiRdResp            0x%08x\n", wmemiRdResp.getULongValue ( ) );
        printf ( "wmemi_status           0x%08x\n", wmemi_status.getULongValue ( ) );
        printf ( "wmemi_ReadInFlight     0x%08x\n", wmemi_ReadInFlight.getULongValue ( ) );
        printf ( "dummy0                 0x%08x\n", dummy0.getULongValue ( ) );
        printf ( "dummy1                 0x%08x\n", dummy1.getULongValue ( ) );
        printf ( "requestCount           0x%08x\n", requestCount.getULongValue ( ) );
        printf ( "dummy2                 0x%08x\n", dummy2.getULongValue ( ) );
        printf ( "pReg                   0x%08x\n", pReg.getULongValue ( ) );
        printf ( "4BWritePIO             Write only\n" );
        printf ( "4BReadPIO              Write only\n" );
        printf ( "mReg                   0x%08x\n", mReg.getULongValue ( ) );
        printf ( "wdReg_0                0x%08x\n", wdReg_0.getULongValue ( ) );
        printf ( "wdReg_1                0x%08x\n", wdReg_1.getULongValue ( ) );
        printf ( "wdReg_2                0x%08x\n", wdReg_2.getULongValue ( ) );
        printf ( "wdReg_3                0x%08x\n", wdReg_3.getULongValue ( ) );
        printf ( "rdReg_0                0x%08x\n", rdReg_0.getULongValue ( ) );
        printf ( "rdReg_1                0x%08x\n", rdReg_1.getULongValue ( ) );
        printf ( "rdReg_2                0x%08x\n", rdReg_2.getULongValue ( ) );
        printf ( "rdReg_3                0x%08x\n\n", rdReg_3.getULongValue ( ) );
      }

      uint32_t status ( )
      {
        CPI::Container::Property dramStatus ( d_worker, "dramStatus" );

        return dramStatus.getULongValue ( );
      }

    private:

      std::string d_msg;

  }; // End: class HdlDramServer : public HdlFacade

  class HdlBias : public HdlFacade
  {
    public:

      HdlBias ( const char* msg,
                CPI::Container::Interface* container,
                CPI::Container::Application* application,
                const char* uri,
                const char* worker_name,
                const char* impl_name )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg ),
          d_port_in ( d_worker.getPort ( "WSIin" ) ),
          d_port_out ( d_worker.getPort ( "WSIout" ) )
      {
        // Empty
      }

      virtual ~HdlBias ( )
      {
        // Empty
      }

      CPI::Container::Port& wsi_in ( ) const
      {
        return d_port_in;
      }

      CPI::Container::Port& wsi_out ( ) const
      {
        return d_port_out;
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        CPI::Container::Property biasValue ( d_worker, "biasValue" );
        CPI::Container::Property controlReg ( d_worker, "controlReg" );
        CPI::Container::Property messagePush ( d_worker, "messagePush" );
        CPI::Container::Property lastOpcode0 ( d_worker, "lastOpcode0" );
        CPI::Container::Property portStatus ( d_worker, "portStatus" );
        CPI::Container::Property WSISpMesgCount ( d_worker, "WSISpMesgCount" );
        CPI::Container::Property WSISiMesgCount ( d_worker, "WSISiMesgCount" );
        CPI::Container::Property WSIMpMesgCount ( d_worker, "WSIMpMesgCount" );
        CPI::Container::Property WSIMiMesgCount ( d_worker, "WSIMiMesgCount" );

        printf ( "\n\nBias worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "biasValue      0x%08x\n", biasValue.getULongValue ( ) );
        printf ( "controlReg     0x%08x\n", controlReg.getULongValue ( ) );
        printf ( "messagePush    0x%08x\n", messagePush.getULongValue ( ) );
        printf ( "lastOpcode0    0x%08x\n", lastOpcode0.getULongValue ( ) );
        printf ( "portStatus     0x%08x\n", portStatus.getULongValue ( ) );
        printf ( "WSISpMesgCount 0x%08x\n", WSISpMesgCount.getULongValue ( ) );
        printf ( "WSISiMesgCount 0x%08x\n", WSISiMesgCount.getULongValue ( ) );
        printf ( "WSIMpMesgCount 0x%08x\n", WSIMpMesgCount.getULongValue ( ) );
        printf ( "WSIMiMesgCount 0x%08x\n", WSIMiMesgCount.getULongValue ( ) );
     }

    private:

      std::string d_msg;

      CPI::Container::Port& d_port_in;
      CPI::Container::Port& d_port_out;

  }; // End: class HdlBias : public HdlFacade
  class FrameGate : public HdlFacade
  {
    public:

      enum
      {
        BYPASS = 0,
        GATE = 1
      };

      FrameGate ( const char* msg,
                  CPI::Container::Interface* container,
                  CPI::Container::Application* application,
                  const char* uri,
                  const char* worker_name,
                  const char* impl_name,
                  uint32_t frame_gate_control,
                  uint32_t frame_n_bytes,
                  uint32_t gate_n_bytes )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg ),
          d_port_in ( d_worker.getPort ( "WSIin" ) ),
          d_port_out ( d_worker.getPort ( "WSIout" ) )
      {
#if BYPASS_HDL_FRAMEGATE
        ( void ) frame_gate_control;
        ( void ) frame_n_bytes;
        ( void ) gate_n_bytes;
        CPI::Container::Property frameGateCtrl ( d_worker, "frameGateCtrl" );
        frameGateCtrl.setULongValue ( BYPASS );

#else
        CPI::Container::Property frameGateCtrl ( d_worker, "frameGateCtrl" );
        CPI::Container::Property frameSize ( d_worker, "frameSize" );
        CPI::Container::Property gateSize ( d_worker, "gateSize" );

        frameGateCtrl.setULongValue ( frame_gate_control );
        frameSize.setULongValue ( frame_n_bytes );
        gateSize.setULongValue ( gate_n_bytes );
#endif
      }

      virtual ~FrameGate ( )
      {
        // Empty
      }

      CPI::Container::Port& wsi_in ( ) const
      {
        return d_port_in;
      }

      CPI::Container::Port& wsi_out ( ) const
      {
        return d_port_out;
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        CPI::Container::Property frameGateStatus ( d_worker, "frameGateStatus" );
        CPI::Container::Property frameGateCtrl ( d_worker, "frameGateCtrl" );
        CPI::Container::Property frameSize ( d_worker, "frameSize" );
        CPI::Container::Property gateSize ( d_worker, "gateSize" );
        CPI::Container::Property reserved ( d_worker, "reserved" );
        CPI::Container::Property WSI_S_pMesgCount ( d_worker, "WSI_S_pMesgCount" );
        CPI::Container::Property WSI_S_iMesgCount ( d_worker, "WSI_S_iMesgCount" );
        CPI::Container::Property WSI_S_tBusyCount ( d_worker, "WSI_S_tBusyCount" );
        CPI::Container::Property WSI_M_pMesgCount ( d_worker, "WSI_M_pMesgCount" );
        CPI::Container::Property WSI_M_iMesgCount ( d_worker, "WSI_M_iMesgCount" );
        CPI::Container::Property WSI_M_tBusyCount ( d_worker, "WSI_M_tBusyCount" );
        CPI::Container::Property Op0MesgCnt ( d_worker, "Op0MesgCnt" );
        CPI::Container::Property otherMesgCnt ( d_worker, "otherMesgCnt" );

        printf ( "\n\nFrameGate worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "frameGateStatus  0x%08x\n", frameGateStatus.getULongValue ( ) );
        printf ( "frameGateCtrl    0x%08x\n", frameGateCtrl.getULongValue ( ) );
        printf ( "frameSize        0x%08x\n", frameSize.getULongValue ( ) );
        printf ( "gateSize         0x%08x\n", gateSize.getULongValue ( ) );
        printf ( "reserved         0x%08x\n", reserved.getULongValue ( ) );
        printf ( "WSI_S_pMesgCount 0x%08x\n", WSI_S_pMesgCount.getULongValue ( ) );
        printf ( "WSI_S_iMesgCount 0x%08x\n", WSI_S_iMesgCount.getULongValue ( ) );
        printf ( "WSI_S_tBusyCount 0x%08x\n", WSI_S_tBusyCount.getULongValue ( ) );
        printf ( "WSI_M_pMesgCount 0x%08x\n", WSI_M_pMesgCount.getULongValue ( ) );
        printf ( "WSI_M_iMesgCount 0x%08x\n", WSI_M_iMesgCount.getULongValue ( ) );
        printf ( "WSI_M_tBusyCount 0x%08x\n", WSI_M_tBusyCount.getULongValue ( ) );
        printf ( "Op0MesgCnt       0x%08x\n", Op0MesgCnt.getULongValue ( ) );
        printf ( "otherMesgCnt     0x%08x\n", otherMesgCnt.getULongValue ( ) );
     }

    private:

      std::string d_msg;

      CPI::Container::Port& d_port_in;
      CPI::Container::Port& d_port_out;

  }; // End: class HdlFrameGate : public HdlFacade

  class HdlWsiSplitter2x2 : public HdlFacade
  {
    public:

      HdlWsiSplitter2x2 ( const char* msg,
                          CPI::Container::Interface* container,
                          CPI::Container::Application* application,
                          const char* uri,
                          const char* worker_name,
                          const char* impl_name )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg ),
          d_port_in_a ( d_worker.getPort ( "WSIinA" ) ),
          d_port_in_b ( d_worker.getPort ( "WSIinB" ) ),
          d_port_out_c ( d_worker.getPort ( "WSIoutC" ) ),
          d_port_out_d ( d_worker.getPort ( "WSIoutD" ) )
      {
        CPI::Container::Property splitCtrl ( d_worker, "splitCtrl" );
        splitCtrl.setULongValue ( 0x00000101 );
      }

      virtual ~HdlWsiSplitter2x2 ( )
      {
        // Empty
      }

      CPI::Container::Port& wsi_in_a ( ) const
      {
        return d_port_in_a;
      }

      CPI::Container::Port& wsi_in_b ( ) const
      {
        return d_port_in_b;
      }

      CPI::Container::Port& wsi_out_c ( ) const
      {
        return d_port_out_c;
      }

      CPI::Container::Port& wsi_out_d ( ) const
      {
        return d_port_out_d;
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        CPI::Container::Property reserved0 ( d_worker, "reserved0" );
        CPI::Container::Property splitCtrl ( d_worker, "splitCtrl" );
        CPI::Container::Property reserved1 ( d_worker, "reserved1" );
        CPI::Container::Property reserved2 ( d_worker, "reserved2" );
        CPI::Container::Property reserved3 ( d_worker, "reserved3" );
        CPI::Container::Property reserved4 ( d_worker, "reserved4" );
        CPI::Container::Property reserved5 ( d_worker, "reserved5" );
        CPI::Container::Property status ( d_worker, "status" );
        CPI::Container::Property S0_pMesgCount ( d_worker, "S0_pMesgCount" );
        CPI::Container::Property S0_iMesgCount ( d_worker, "S0_iMesgCount" );
        CPI::Container::Property S1_pMesgCount ( d_worker, "S1_pMesgCount" );
        CPI::Container::Property S1_iMesgCount ( d_worker, "S1_iMesgCount" );
        CPI::Container::Property M0_pMesgCount ( d_worker, "M0_pMesgCount" );
        CPI::Container::Property M0_iMesgCount ( d_worker, "M0_iMesgCount" );
        CPI::Container::Property M1_pMesgCount ( d_worker, "M1_pMesgCount" );
        CPI::Container::Property M1_iMesgCount ( d_worker, "M1_iMesgCount" );

        printf ( "\n\nWSISplitter2x2 worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "reserved0     0x%08x\n", reserved0.getULongValue ( ) );
        printf ( "splitCtrl     0x%08x\n", splitCtrl.getULongValue ( ) );
        printf ( "reserved1     0x%08x\n", reserved1.getULongValue ( ) );
        printf ( "reserved2     0x%08x\n", reserved2.getULongValue ( ) );
        printf ( "reserved3     0x%08x\n", reserved3.getULongValue ( ) );
        printf ( "reserved4     0x%08x\n", reserved4.getULongValue ( ) );
        printf ( "reserved5     0x%08x\n", reserved5.getULongValue ( ) );
        printf ( "status        0x%08x\n", status.getULongValue ( ) );
        printf ( "S0_pMesgCount 0x%08x\n", S0_pMesgCount.getULongValue ( ) );
        printf ( "S0_iMesgCount 0x%08x\n", S0_iMesgCount.getULongValue ( ) );
        printf ( "S1_pMesgCount 0x%08x\n", S1_pMesgCount.getULongValue ( ) );
        printf ( "S1_iMesgCount 0x%08x\n", S1_iMesgCount.getULongValue ( ) );
        printf ( "M0_pMesgCount 0x%08x\n", M0_pMesgCount.getULongValue ( ) );
        printf ( "M0_iMesgCount 0x%08x\n", M0_iMesgCount.getULongValue ( ) );
        printf ( "M1_pMesgCount 0x%08x\n", M1_pMesgCount.getULongValue ( ) );
        printf ( "M1_iMesgCount 0x%08x\n", M1_iMesgCount.getULongValue ( ) );
     }

    private:

      std::string d_msg;

      CPI::Container::Port& d_port_in_a;
      CPI::Container::Port& d_port_in_b;
      CPI::Container::Port& d_port_out_c;
      CPI::Container::Port& d_port_out_d;

  }; // End: class HdlWsiSplitter2x2 : public HdlFacade

  class HdlPSD : public HdlFacade
  {
    public:

      HdlPSD ( const char* msg,
               CPI::Container::Interface* container,
               CPI::Container::Application* application,
               const char* uri,
               const char* worker_name,
               const char* impl_name )
        : HdlFacade ( container, application, uri, worker_name, impl_name ),
          d_msg ( msg ),
          d_port_in ( d_worker.getPort ( "WSIin" ) ),
          d_port_out ( d_worker.getPort ( "WSIout" ) )
      {
#if BYPASS_PSD
        CPI::Container::Property psdCtrl ( d_worker, "psdCtrl" );

        psdCtrl.setULongValue ( 0 );
#endif
      }

      virtual ~HdlPSD ( )
      {
        // Empty
      }

      CPI::Container::Port& wsi_in ( ) const
      {
        return d_port_in;
      }

      CPI::Container::Port& wsi_out ( ) const
      {
        return d_port_out;
      }

      void set_properties ( )
      {
        // Empty
      }

      void print_status ( )
      {
        CPI::Container::Property debugStatus ( d_worker, "debugStatus" );
        CPI::Container::Property psdCtrl ( d_worker, "psdCtrl" );
        CPI::Container::Property status ( d_worker, "status" );
        CPI::Container::Property WSI_S_pMesgCount ( d_worker, "WSI_S_pMesgCount" );
        CPI::Container::Property WSI_S_iMesgCount ( d_worker, "WSI_S_iMesgCount" );
        CPI::Container::Property WSI_S_tBusyCount ( d_worker, "WSI_S_tBusyCount" );
        CPI::Container::Property WSI_M_pMesgCount ( d_worker, "WSI_M_pMesgCount" );
        CPI::Container::Property WSI_M_iMesgCount ( d_worker, "WSI_M_iMesgCount" );
        CPI::Container::Property WSI_M_tBusyCount ( d_worker, "WSI_M_tBusyCount" );

        printf ( "\n\nPSD worker properties (%s)\n", d_msg.c_str ( ) );
        printf ( "debugStatus      0x%08x\n", debugStatus.getULongValue ( ) );
        printf ( "psdCtrl          0x%08x\n", psdCtrl.getULongValue ( ) );
        printf ( "status           0x%08x\n", status.getULongValue ( ) );
        printf ( "WSI_S_pMesgCount 0x%08x\n", WSI_S_pMesgCount.getULongValue ( ) );
        printf ( "WSI_S_iMesgCount 0x%08x\n", WSI_S_iMesgCount.getULongValue ( ) );
        printf ( "WSI_S_tBusyCount 0x%08x\n", WSI_S_tBusyCount.getULongValue ( ) );
        printf ( "WSI_M_pMesgCount 0x%08x\n", WSI_M_pMesgCount.getULongValue ( ) );
        printf ( "WSI_M_iMesgCount 0x%08x\n", WSI_M_iMesgCount.getULongValue ( ) );
        printf ( "WSI_M_tBusyCount 0x%08x\n", WSI_M_tBusyCount.getULongValue ( ) );
     }

    private:

      std::string d_msg;

      CPI::Container::Port& d_port_in;
      CPI::Container::Port& d_port_out;

  }; // End: class HdlPSD : public HdlFacade

  void start ( CPI::Container::Worker* worker )
  {
    worker->start ( );
  }

  void stop ( CPI::Container::Worker* worker )
  {
    worker->stop ( );
  }

  void release ( CPI::Container::Worker* worker )
  {
    worker->release ( );
  }

  void set_properties ( WorkerFacade* facade )
  {
    facade->set_properties ( );
  }

  void XmBhAdapter::run_app ( void* arg )
  {
    ( void ) arg;

    try
    {
      std::vector<WorkerFacade*> facades;
      // Holds facades for group operations

      std::vector<CPI::Container::Worker*> workers;
      // Holds workers used for group lifecycle management

      std::vector<CPI::Container::Interface*> interfaces;
      // Holds RCC worker interfaces passed to the RCC dispatch thread

      /* ---- Create the RCC Delay worker -------------------------------- */

      const char* rcc_framegate_uri = "/home/mpepe/projects/jcrew/i2wd/examples/components/framegate/lib/rcc/Linux-MCS_864x/framegate.so";

      CPI::Container::Interface* framegate_container ( get_rcc_interface ( ) );

      CPI::Container::Application*
        framegate_application ( framegate_container->createApplication ( ) );

      RccFrameGate rcc_framegate ( framegate_container,
                                   framegate_application,
                                   rcc_framegate_uri,
                                  "framegate",
#if BYPASS_RCC_FRAMEGATE
                                   RccFrameGate::BYPASS,
#else
                                   RccFrameGate::GATE,
#endif
                                   buffer_n_bytes,
                                   n_frames_to_drop * buffer_n_bytes );

      facades.push_back ( &rcc_framegate );

      workers.push_back ( rcc_framegate.worker ( ) );

      interfaces.push_back ( rcc_framegate.interface ( ) );

      /* ---- Create the shared HDL containers and applications ---------- */

      CPI::RPL::Driver& driver = *( new CPI::RPL::Driver ( ) );

      CPI::Util::DriverManager& dm =
                               *( new CPI::Util::DriverManager ( "OCFRP" ) );

      dm.discoverDevices ( 0, 0 );

      CPI::Container::Interface* hdl_container_sx95t
      ( static_cast<CPI::Container::Interface*> ( dm.getDevice ( 0, "0000:06:00.0"  ) ) );

      CPI::Container::Application*
        hdl_application_sx95t ( hdl_container_sx95t->createApplication ( ) );

      const char* sx95t_uri = "/home/mpepe/projects/jcrew/i2wd/examples/components/sx95t/lib/hdl/ocpi_sx95t.bin";

      CPI::Container::Interface* hdl_container_lx330
      ( static_cast<CPI::Container::Interface*> ( dm.getDevice ( 0, "0000:09:00.0"  ) ) );

      CPI::Container::Application*
        hdl_application_lx330 ( hdl_container_lx330->createApplication ( ) );

      const char* lx330_uri = "/home/mpepe/projects/jcrew/i2wd/examples/components/lx330/lib/hdl/ocpi_lx330.bin";

      /* ---- Create the sx95t DAC worker -------------------------------- */

      HdlDac sx95t_dac ( "SX95T",
                         hdl_container_sx95t,
                         hdl_application_sx95t,
                         sx95t_uri,
                         "DAC",
                         "DACi" );

      facades.push_back ( &sx95t_dac );

      // Do not add DAC to worker array. Only want to start and
      // stop it for CM emission.

      /* ---- Create the sx95t DramServer worker ------------------------- */

      HdlDramServer sx95t_dram ( "SX95T",
                                 hdl_container_sx95t,
                                 hdl_application_sx95t,
                                 sx95t_uri,
                                 "DramServer",
                                 "DramServeri" );

      facades.push_back ( &sx95t_dram );

      workers.push_back ( sx95t_dram.worker ( ) );

      /* ---- Create the lx330 SMA worker -------------------------------- */

      HdlSMA lx330_sma1 ( "LX330 SMA1 (aka FP)",
                         hdl_container_lx330,
                         hdl_application_lx330,
                         lx330_uri,
                         "SMA",
                         "SMA1",
                         HdlSMA::PASS_WSI_IN_TO_WMI_DMA_OUT );

      facades.push_back ( &lx330_sma1 );

      workers.push_back ( lx330_sma1.worker ( ) );

      /* ---- Create the lx330 BIAS worker ------------------------------- */

      HdlBias lx330_bias ( "LX330",
                           hdl_container_lx330,
                           hdl_application_lx330,
                           lx330_uri,
                           "Bias",
                           "BIASi" );

      facades.push_back ( &lx330_bias );

      workers.push_back ( lx330_bias.worker ( ) );

      /* ---- Create the lx330 SMA worker -------------------------------- */

      HdlSMA lx330_sma0 ( "LX330 SMA0 (aka FC)",
                          hdl_container_lx330,
                          hdl_application_lx330,
                          lx330_uri,
                          "SMA",
                          "SMA0",
                          HdlSMA::PASS_WMI_DMA_IN_TO_WSI_OUT );

      facades.push_back ( &lx330_sma0 );

      workers.push_back ( lx330_sma0.worker ( ) );

      /* ---- Create the sx95t SMA worker -------------------------------- */

      HdlSMA sx95t_sma1 ( "SX95T SMA1 (aka FP)",
                          hdl_container_sx95t,
                          hdl_application_sx95t,
                          sx95t_uri,
                          "SMA",
                          "SMA1",
                          HdlSMA::PASS_WSI_IN_TO_WMI_DMA_OUT );

      facades.push_back ( &sx95t_sma1 );

      workers.push_back ( sx95t_sma1.worker ( ) );

      /* ---- Create the sx95t Delay worker ------------------------------ */

      HdlDelay sx95t_delay ( "SX95T",
                             hdl_container_sx95t,
                             hdl_application_sx95t,
                             sx95t_uri,
                             "Delay",
                             "DELAYi" );

      facades.push_back ( &sx95t_delay );

      workers.push_back ( sx95t_delay.worker ( ) );

      /* ---- Create the sx95t SMA worker -------------------------------- */

      HdlSMA sx95t_sma0 ( "SX95T SMA0 (aka FC)",
                           hdl_container_sx95t,
                           hdl_application_sx95t,
                           sx95t_uri,
                           "SMA",
                           "SMA0",
                           HdlSMA::PASS_WSI_IN_TO_WMI_DMA_OUT );

      facades.push_back ( &sx95t_sma0 );

      workers.push_back ( sx95t_sma0.worker ( ) );

      /* ---- Create the sx95t PSD worker -------------------------------- */

      HdlPSD sx95t_psd ( "SX95T",
                          hdl_container_sx95t,
                          hdl_application_sx95t,
                          sx95t_uri,
                          "PSD",
                          "PSDi" );

      facades.push_back ( &sx95t_psd );

      workers.push_back ( sx95t_psd.worker ( ) );

      /* ---- Create the sx95t Frame Gate worker ------------------------- */

      FrameGate sx95t_framegate ( "SX95T",
                                  hdl_container_sx95t,
                                  hdl_application_sx95t,
                                  sx95t_uri,
                                  "FrameGate",
                                  "FrameGatei",
#if BYPASS_HDL_FRAMEGATE
                                  FrameGate::BYPASS,
#else
                                  FrameGate::GATE,
#endif
                                  buffer_n_bytes,
                                  n_frames_to_drop * buffer_n_bytes );

      facades.push_back ( &sx95t_framegate );

      workers.push_back ( sx95t_framegate.worker ( ) );

      /* ---- Create the sx95t 2x2 WSI Splitter worker ------------------- */

      HdlWsiSplitter2x2 sx95t_wsi_split ( "SX95T",
                                          hdl_container_sx95t,
                                          hdl_application_sx95t,
                                          sx95t_uri,
                                          "WsiSplitter2x2",
                                          "WsiSplitter2x2i" );

      facades.push_back ( &sx95t_wsi_split );

      workers.push_back ( sx95t_wsi_split.worker ( ) );

      /* ---- Create the sx95t ADC worker -------------------------------- */

      HdlAdc sx95t_adc ( "SX95T",
                         hdl_container_sx95t,
                         hdl_application_sx95t,
                         sx95t_uri,
                         "ADC",
                         "ADCi" );

      facades.push_back ( &sx95t_adc );

      workers.push_back ( sx95t_adc.worker ( ) );

      /* ---- Connect the workers ---------------------------------------- */

      /* Undelayed data path */

      d_ex_undelayed =
              rcc_framegate.external_target_port ( rcc_framegate.wsi_out ( ),
                                                   "undelayed" );

      sx95t_sma0.wsi_out ( ).connect ( rcc_framegate.wsi_in ( ) );

      sx95t_psd.wsi_out ( ).connect ( sx95t_sma0.wsi_in ( ) );

      sx95t_framegate.wsi_out ( ).connect ( sx95t_psd.wsi_in ( ) );

      sx95t_wsi_split.wsi_out_d ( ).connect ( sx95t_framegate.wsi_in ( ) );

      sx95t_adc.adc_out ( ).connect ( sx95t_wsi_split.wsi_in_a ( ) );

      /* Delayed data path */

      sx95t_wsi_split.wsi_out_c ( ).connect ( sx95t_delay.wsi_in ( ) );

      sx95t_delay.wsi_out ( ).connect ( sx95t_sma1.wsi_in ( ) );

      sx95t_sma1.wmi_out ( ).connect ( lx330_sma0.wmi_in ( ) );

      lx330_sma0.wsi_out ( ).connect ( lx330_bias.wsi_in ( ) );

      lx330_bias.wsi_out ( ).connect ( lx330_sma1.wsi_in ( ) );

      d_ex_delayed =
                     lx330_sma1.external_target_port ( lx330_sma1.wmi_out ( ),
                                                      "delayed" );

      /* ---- Setup dispatch thread for RCC workers ---------------------- */

      std::auto_ptr<DispatchThread>
                            dispatcher ( new DispatchThread ( interfaces ) );

      /* ---- Set the worker's properties -------------------------------- */

      std::for_each ( facades.begin ( ), facades.end ( ), set_properties );

      /* ---- Start all of the workers ----------------------------------- */

      // Above workers were inserted so that they would be started
      // in the reverse order of the data flow (ADC is last to start).
      std::for_each ( workers.begin ( ), workers.end ( ), start );

     /* ---- Let the workers run until they are done --------------------- */

      while ( !d_done )
      {
        sleep ( 1 );

        if ( d_do_cm )
        {
          d_do_cm = false;
          sx95t_dac.counter_measure ( );
        }
      }

      /* ---- Stop the workers-- ----------------------------------------- */

      std::for_each ( workers.begin ( ), workers.end ( ), stop );

      /* ---- Release the workers-- -------------------------------------- */

      std::for_each ( workers.begin ( ), workers.end ( ), release );

    }
    catch ( const std::string& s )
    {
      std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
    }
    catch ( const CPI::Util::EmbeddedException& e )
    {
      std::cerr << "\nException(e): "
                << e.getAuxInfo ( )
                << "\n"
                << std::endl;
    }
    catch ( ... )
    {
      std::cerr << "\n\nException(u): unknown\n" << std::endl;
    }

    return;
  }

  XmBhAdapter* create_xm_application_adapter ( )
  {
    return new XmBhAdapter ( );
  }

} // End: namespace<unamed>

/* ---- Implementation of adapter functions used by XM primitive --------- */

int ocpi_adapter_init ( void** pp_adapter, uint32_t n_delay_samples )
{
  ( void ) n_delay_samples;

  try
  {
    XmBhAdapter* p_adapter = create_xm_application_adapter ( );

    *pp_adapter = reinterpret_cast<void*> ( p_adapter );

    return 0;
  }
  catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
  catch ( const CPI::Util::EmbeddedException& e )
  {
    std::cerr << "\nException(e): " << e.getAuxInfo ( ) << "\n" << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }

  return -1;
}

int ocpi_adapter_fini ( void** pp_adapter )
{
  try
  {
    XmBhAdapter* p_adapter = reinterpret_cast<XmBhAdapter*> ( *pp_adapter );

    delete p_adapter;

    return 0;
  }
  catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
  catch ( const CPI::Util::EmbeddedException& e )
  {
    std::cerr << "\nException(e): " << e.getAuxInfo ( ) << "\n" << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }

  return -1;
}

int ocpi_adapter_put_buffer ( void* p_adapter,
                              const uint8_t* p_buffer,
                              size_t n_bytes )
{
  try
  {
    XmBhAdapter* adapter = reinterpret_cast<XmBhAdapter*> ( p_adapter );

    adapter->put_buffer ( p_buffer, n_bytes );

    return 0;
  }
  catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
  catch ( const CPI::Util::EmbeddedException& e )
  {
    std::cerr << "\nException(e): " << e.getAuxInfo ( ) << "\n" << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }

  return -1;
}

int ocpi_adapter_undelayed_get_buffer ( void* p_adapter,
                                        uint8_t* p_buffer,
                                        size_t n_bytes )
{
  try
  {
    static int n_iters = 0;
    XmBhAdapter* adapter = reinterpret_cast<XmBhAdapter*> ( p_adapter );

    adapter->get_undelayed_buffer ( p_buffer, n_bytes );

    n_iters++;

    return 0;
  }
  catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
  catch ( const CPI::Util::EmbeddedException& e )
  {
    std::cerr << "\nException(e): " << e.getAuxInfo ( ) << "\n" << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }

  return -1;
}

int ocpi_adapter_delayed_get_buffer ( void* p_adapter,
                                      uint8_t* p_buffer,
                                      size_t n_bytes )
{
  try
  {
    XmBhAdapter* adapter = reinterpret_cast<XmBhAdapter*> ( p_adapter );

    adapter->get_delayed_buffer ( p_buffer, n_bytes );

    return 0;
  }
  catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
  catch ( const CPI::Util::EmbeddedException& e )
  {
    std::cerr << "\nException(e): " << e.getAuxInfo ( ) << "\n" << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }

  return -1;
}


int ocpi_adapter_counter_measure ( void* p_adapter,
                                   double frequency )
{
  try
  {
    XmBhAdapter* adapter = reinterpret_cast<XmBhAdapter*> ( p_adapter );

    adapter->counter_measure ( frequency );

    return 0;
  }
  catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
  catch ( const CPI::Util::EmbeddedException& e )
  {
    std::cerr << "\nException(e): " << e.getAuxInfo ( ) << "\n" << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }

  return -1;
}
