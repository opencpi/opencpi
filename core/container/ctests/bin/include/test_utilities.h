
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


/*
 * Unit test utilities include file.
 *
 * 06/01/09 - John Miller 
 * Added RPL support.
 *
 * 06/01/09 - John Miller
 * Initial Version
 */
#include <stdio.h>
#include <signal.h>
#include <sstream>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <OcpiTransportServer.h>
#include <OcpiTransportClient.h>
#include <OcpiContainerInterface.h>
#include <OcpiContainerErrorCodes.h>
#include <OcpiWorker.h>
#include <OcpiContainerPort.h>
#include <OcpiRDTInterface.h>
#include <OcpiThread.h>
#include <OcpiPValue.h>
#include <RCC_Worker.h>
#include <list>

extern bool g_testUtilVerbose;
#define TUPRINTF if(g_testUtilVerbose) printf

typedef void SignalCb(int);

class SignalHandler {
 public:
  SignalHandler( SignalCb * callback=NULL ) 
    {
      m_cb = callback;
      once = false;
      signal (SIGTERM,my_catcher);
      signal (SIGILL,my_catcher);
      signal (SIGABRT,my_catcher);
      signal (SIGINT,my_catcher);
      signal (SIGHUP,my_catcher);
      signal (SIGQUIT,my_catcher);
      //      signal (SIGTSTP,my_catcher);
    }

  static void my_catcher( int signum )
    {
      if ( once ) return;
      printf("Got a signal, number = %d\n", signum );
      if ( m_cb ) {
        once = true;
        OCPI::Time::Emit::shutdown();
        m_cb(signum);
      }
    }

 private:
  static SignalCb * m_cb;
  static bool once;
};


struct ContainerDesc {
  std::string             ep;
  std::string             type;
  ContainerDesc(const char* pep, const char* ptype )
  :ep(pep),type(ptype){}
};

struct CApp {
  OCPI::API::Container*         container;
  OCPI::Container::Worker*      worker;
  OCPI::API::ContainerApplication *      app;
};

struct CWorker {
  int                            cid;
  OCPI::Container::Worker *       worker;
  struct ConData {
    CWorker*     worker;
    int          pid;
  };
  struct Pdata {
    OCPI::Util::PValue            *props;
    bool                          input;
    int                           bufferCount;
    OCPI::Container::Port *        port;
    ConData                       down_stream_connection;
    Pdata():props(0),bufferCount(2),port(0){};
  };
  int sPortCount;
  int tPortCount;
  Pdata                  pdata[32];
CWorker(int tports, int sports):sPortCount(sports), tPortCount(tports){};
  int operator=(int i)
  {
    worker=0; cid=i; return i;
    for ( int n=0; n<32; n++ ) {
      pdata[n].down_stream_connection.worker = NULL;
    }
  }
};

#define PORT_0 0
#define PORT_1 1
#define PORT_2 2
#define PORT_3 3
#define PORT_4 4
#define PORT_5 5

#define CATCH_ALL_RETHROW( msg )                                        \
  catch ( int& ii ) {                                                        \
    TUPRINTF("gpp: Caught an int exception while %s = %d\n", msg,ii );        \
    throw;                                                                \
  }                                                                        \
  catch( std::string& stri ) {                                                \
    TUPRINTF("gpp: Caught a string exception while %s = %s\n", msg, stri.c_str() ); \
    throw;                                                                \
  }                                                                        \
  catch ( OCPI::Util::EmbeddedException& eex ) {                                \
    TUPRINTF(" gpp: Caught an embedded exception while %s:\n", msg);        \
    TUPRINTF( " error number = %d", eex.m_errorCode );                        \
    TUPRINTF( " aux info = %s\n", eex.m_auxInfo.c_str() );                \
    throw;                                                                \
  }                                                                        \
  catch( ... ) {                                                        \
    TUPRINTF("gpp: Caught an unknown exception while %s\n",msg );        \
    throw;                                                                \
  }

#if 0

#define CHECK_WCI_CONROL_ERROR(err, op, ca, w)                                \
  if ( err != WCI_SUCCESS ) {                                                \
    std::string err_str = w.worker->getLastControlError(); \
    TUPRINTF("ERROR: WCI control(%d) returned %d, error string = %s\n", op, err, err_str.c_str() ); \
    throw std::string("WCI control error");                                \
  }



#define CHECK_WCI_WRITE_ERROR(err, ca, w )                                \
  if ( err != WCI_SUCCESS ) {                                                \
    std::string err_str = w.worker->getLastControlError(); \
    TUPRINTF("ERROR: WCI write returned %d, error string = %s\n", err, err_str.c_str() ); \
    throw std::string("WCI Write error");                                \
  }
#endif


#define TRY_AND_SET(var, str, exp, code)		 \
  do {							 \
    var = OC::NO_ERROR_;				 \
    try {						 \
      code;						 \
    }  catch (OCPI::Util::EmbeddedException &ee_) {	 \
      var = ee_.getErrorCode(); str = ee_.m_auxInfo;	 \
    }  catch (...) {					 \
      var = OC::LAST_ERROR_ID;				 \
    }							 \
    TUPRINTF("Expected error string (%s) got %u (%s)\n", \
	     exp, var, str.c_str() );			 \
  } while (0)

namespace OCPI {
  namespace CONTAINER_TEST {
    void  dumpPortData( OCPI::Container::PortData * pd );
    void testDispatch(OCPI::API::Container* rcc_container);
    void initWorkers(std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void enableWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void disableWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void disconnectPorts( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void destroyWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void destroyContainers( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void createPorts( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void connectWorkers(std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    std::vector<CApp> createContainers( std::vector<const char*>& eps, 
                                        DataTransfer::EventManager*& event_manager, bool use_polling );

    std::vector<CApp> createContainers( std::vector<ContainerDesc>& eps, 
                                        DataTransfer::EventManager*& event_manager, bool use_polling );

    struct DThreadData {
      bool run;
      std::vector<CApp> containers;
      DataTransfer::EventManager* event_manager;
    };
    OCPI::Util::Thread* runTestDispatch( DThreadData& tdata );

    OCPI::Container::Worker *createWorker(CApp &capp, ::RCCDispatch *rccd);
    OCPI::Container::Worker *createWorker(OCPI::API::ContainerApplication *app, ::RCCDispatch *rccd);
  }

}




