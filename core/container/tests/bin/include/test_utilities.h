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
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <CpiTransportServer.h>
#include <CpiTransportClient.h>
#include <CpiContainerInterface.h>
#include <CpiWorker.h>
#include <CpiContainerPort.h>
#include <CpiRDTInterface.h>
#include <CpiThread.h>
#include <CpiPValue.h>
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
      signal (SIGTSTP,my_catcher);
    }

  static void my_catcher( int signum )
    {
      if ( once ) return;
      printf("Got a signal, number = %d\n", signum );
      if ( m_cb ) {
        once = true;
        CPI::Time::Emit::shutdown();
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
  CPI::Container::Interface*         container;
  CPI::WCI::Worker*                  wci_worker;
  CPI::Container::Application *      app;
};

struct CWorker {
  int                            cid;
  CPI::Container::Worker *       worker;
  struct ConData {
    CWorker*     worker;
    int          pid;
  };
  struct Pdata {
    CPI::Util::PValue            *props;
    bool                          input;
    int                           bufferCount;
    CPI::Container::Port *        port;
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
  catch ( CPI::Util::EmbeddedException& eex ) {                                \
    TUPRINTF(" gpp: Caught an embedded exception while %s:\n", msg);        \
    TUPRINTF( " error number = %d", eex.m_errorCode );                        \
    TUPRINTF( " aux info = %s\n", eex.m_auxInfo.c_str() );                \
    throw;                                                                \
  }                                                                        \
  catch( ... ) {                                                        \
    TUPRINTF("gpp: Caught an unknown exception while %s\n",msg );        \
    throw;                                                                \
  }



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

namespace CPI {
  namespace CONTAINER_TEST {
    void  dumpPortData( CPI::Container::PortData * pd );
    void testDispatch(CPI::Container::Interface* rcc_container, DataTransfer::EventManager* event_manager);
    void initWorkers(std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void enableWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void disableWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void disconnectPorts( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void destroyWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void destroyContainers( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void createPorts( std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    void connectWorkers(std::vector<CApp>& ca, std::vector<CWorker*>& workers );
    std::vector<CApp> createContainers( std::vector<char*>& eps, 
                                        DataTransfer::EventManager*& event_manager, bool use_polling );

    std::vector<CApp> createContainers( std::vector<ContainerDesc>& eps, 
                                        DataTransfer::EventManager*& event_manager, bool use_polling );

    struct DThreadData {
      bool run;
      std::vector<CApp> containers;
      DataTransfer::EventManager* event_manager;
    };
    CPI::Util::Thread* runTestDispatch( DThreadData& tdata );

  }

}




