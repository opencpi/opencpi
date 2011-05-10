
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

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <memory>
#include <sys/uio.h>
#include <OcpiDriver.h>
#include "OcpiApi.h"
#include <OcpiThread.h>
#include <OcpiOsMisc.h>
#include <string>
#include <signal.h>

#define W(s) write(2, s, sizeof(s) - 1)

namespace CC = OCPI::Container;
namespace CM = OCPI::Metadata;


CC::Interface * rcc_container;
CC::Application * rcc_application;

namespace CU = OCPI::Util;

namespace OCPI {
  namespace RPL {

    class Driver : public OCPI::Util::Driver {
      // The fd for mapped memory, until we have a driver to restrict it.
      static int pciMemFd;
    public:
      // This constructor simply registers itself. This class has no state.
      Driver() :
        OCPI::Util::Driver("OCFRP","Global",true) {
      }
      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      // It uses a generic PCI scanner to find candidates, and when found, calls the "found" method below.
      virtual unsigned search(const OCPI::Util::PValue*, const char **exclude)
        throw (OCPI::Util::EmbeddedException)
      {
         ( void ) exclude;

        if (getenv("OCPI_OCFRP_DUMMY"))
          probe(0,"0");

#ifdef DONT_COMPILE
        unsigned n = 0;
        const char *err = PCI::search(exclude, OCFRP0_VENDOR, OCFRP0_DEVICE,
                                      OCFRP0_CLASS, OCFRP0_SUBCLASS, found, n);

        if (err)
          fprintf(stderr, "PCI Scanner Error: %s\n", err);
        return n;
#else
        return 0;
#endif
      }

      // This driver method is called to see if a particular container device exists,
      // and if so, to instantiate a container
      virtual OCPI::Util::Device *probe(const OCPI::Util::PValue*, const char *which  )
        throw (OCPI::Util::EmbeddedException);

      virtual ~Driver()
        throw() {};

    };

  }
}



namespace {

    class  DThread : public OCPI::Util::Thread
  {
  public:
    bool m_run;
    CC::Interface & m_interface;
    DThread(  CC::Interface & i )
      :m_run(true),m_interface(i){};

    void run() {
      while(m_run) {
	m_interface.dispatch(NULL);
        OCPI::OS::sleep(0);
      }
    }

  };

}

typedef void SignalCb(int);
static SignalCb * m_cb;
class SignalHandler {
 public:
  SignalHandler( SignalCb * callback=NULL ) 
    {
      m_cb = callback;
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
      printf("Got a signal, number = %d\n", signum );
      if ( m_cb ) {
        m_cb(signum);
      }
    }

 private:

};


void mh(int sn )
{
  printf("got signal %d\n", sn );
  exit(-1);
}


int main(int argc, char *argv[])
{
  //  SignalHandler sh(mh);

  try
  {
  unsigned long bufferCount[10][2] =
    {{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2}};
  const char *active[10][2] = {{0}};
  unsigned long bufferSize = 2048, ioCount = 1, ioSize = 16, memorySize = 0;
  uint64_t memoryBase = 0;
  bool probe = false, loop = false, doread = true, two = false, same = true,
    acquire = false, emit = false, use_rcc = false;
  char *firstarg = 0, *secondarg = 0, *file = 0, *ofile = 0;
  
  if (argc == 1) {
    fprintf(stderr, "Usage is: testRpl <options> [<container-name>]\n");
    fprintf(stderr, "  Options are:\n"
            "-z - include loopback rcc workers in test\n"
            "-rW[io][mfap]\t\tRole for DMA for worker W in or out\n"
            "-d\tProbe specific device, don't discover all\n"
            "-f <file>\tSpecify file to use for test data content\n"
            "-i <iocoiunt>\tSpecify number of message to send(default == 1)\n"
            "-l\t\tEnter the emulator loopback mode\n"
            "-m <memory> <size>\tSpecify pinned DMA memory address\n"
            "-n <nbufs>\tSpecify number of buffers (default == 2)\n"
            "-NW[io] <nbufs>\tSpecify number of buffers for worker W in or out\n"
            "\t\tOptions are: -n0o -n1i -n3o  -n4i -n6o -n0i\n"
            "\t\tWorker 0 is this test program's input and output\n"
            "-s <bufsize>\tSpecify size of buffers (default == 2048)\n"
            "-D <shmname>\tSpecify emulated hardware buffer shm\n"
            "-I <msgsize>\tSpecify size of messages (default == 16 bytes)\n"
            "-P <pdname>\tSpecify file to dump port data\n"
            "-R\tSuppress the read-back and test of what is written\n"
            "-o\tOutput file to write data into\n"
            "-a\tAcquire data from acquistion source, not file or pattern\n"
            "-e\tEmit data to DAC (not implemented)\n"
            );
    return 1;
  }
  for (char **ap = &argv[1]; *ap; ap++)
    if (**ap == '-')
      switch ((*ap)[1]) {
      case 'f':
        file = *++ap;
        break;
      case 'R':
        doread = false;
        break;
      case 'd':
        probe = true;
        break;
      case 'n':
        {
          unsigned n = atoi(*++ap);
          for (unsigned i = 0; i < 10; i++)
            bufferCount[i][0] = bufferCount[i][1] = n;
        }
        break;
      case 'N':
        {
          unsigned n = 0, i = 0;
          if ((*ap)[2]) {
            same = false;
            n = (*ap)[2] - '0';
            if ((*ap)[3] == 'o')
              i = 1;
          }
          bufferCount[n][i] = atoi(*++ap);
        }
        break;
      case 'D':
        {
          static char buf[100];
          snprintf(buf, sizeof(buf), "OCPI_OCFRP_DUMMY=%s", *++ap);
          putenv(buf);
        }
        break;
      case 'P':
        {
          static char buf[100];
          snprintf(buf, sizeof(buf), "OCPI_DUMP_PORTS=%s", *++ap);
          putenv(buf);
        }
        break;
      case 'i':
        ioCount = atoi(*++ap);
        break;
      case 'z':
        use_rcc = true;
        break;
      case 'I':
        ioSize = atoi(*++ap);
        break;
      case 's':
        bufferSize = atoi(*++ap);
        break;
      case '2':
        two = true;
        break;
      case 'l':
        loop = true;
        break;
      case 'm':
        memoryBase = atoll(*++ap);
        memorySize = atoi(*++ap);
        break;
      case 'a':
        acquire = true;
        break;
      case 'o':
        ofile = *++ap;
        break;
      case 'r':
        {
          unsigned n = 0, i = 0;
          if ((*ap)[2]) {
            n = (*ap)[2] - '0';
            if ((*ap)[3] == 'o')
              i = 1;
          }
          if ((*ap)[4])
            switch ((*ap)[4]) {
            case 'm':
              active[n][i] = "active";
              break;
            case 'f':
              active[n][i] = "flowcontrol";
              break;
            case 'p':
              active[n][i] = "passive";
              break;
            case 'a':
              active[n][i] = "activeonly";
              break;
            }
          else
            active[n][i] = "active";
        }
        break;
      default:;
      }
    else if (!firstarg)
      firstarg = *ap;
    else if (!secondarg) {
      two = true;
      secondarg = *ap;
    }
  if (ioSize > bufferSize)
    bufferSize = ioSize;
  printf("Starting: sending %lu messages of %lu (buffer %lu)\n", ioCount, ioSize, bufferSize);
  printf("Buffer counts: 0o %lu 1i %lu 3o %lu 6i %lu 8o %lu 0i %lu\n",
         bufferCount[0][1],
         bufferCount[1][0], bufferCount[3][1], bufferCount[6][0],
         bufferCount[8][1], bufferCount[0][0]);
  printf("Active indicators: 0o %s 1i %s 3o %s 6i %s 8o %s 0i %s\n",
         active[0][1],
         active[1][0], active[3][1], active[6][0],
         active[8][1], active[0][0]);
  if (two)
    printf("Using two boards: %s and %s\n", firstarg, secondarg);
  // If we know there is one, try to create it.
  CC::Interface* rplContainer = 0;
  CC::Interface* rplContainer2 = 0;
  OCPI::RPL::Driver* driver = (new OCPI::RPL::Driver());
  if ( !driver )
  {
    printf ( "Failed to allocate OCPI::RPL::Driver\n" );
    return -1;
  }
  OCPI::Util::DriverManager & dm = *(new OCPI::Util::DriverManager("OCFRP"));

  if (probe) {
    rplContainer = static_cast<CC::Interface*>(dm.getDevice( 0, "0" ));
  } else {
    
    dm.discoverDevices(0,0);

    rplContainer = static_cast<CC::Interface*>(dm.getDevice( 0, firstarg ));
    if (two) {
      rplContainer2 = static_cast<CC::Interface*>(dm.getDevice( 0, secondarg ));
    }
  }


//  CC::Worker * rcc_producer;
//  CC::Worker * rcc_consumer;
  CC::Worker * rcc_loopback = 0;  
  CC::Artifact * rcc_art1;
//  CC::Port     *rccPOut, *rccCIn;
  CC::Port *rccLbOut = 0, *rccLbIn = 0;
  DThread * rcc_dThread = 0;



  if ( use_rcc ) {
    
    printf("Using an RCC loopback worker between fabric ports 3 and 6 \n");
    
    // Create the container
    try { 
      static OCPI::Util::DriverManager rcc_dm("Container");
      static OCPI::Util::PValue cprops[] = {
	OCPI::Util::PVBool("polling",1),
	OCPI::Util::PVEnd };
      rcc_dm.discoverDevices(0,0);
      OCPI::Util::Device* d = rcc_dm.getDevice( cprops, "RCC");
      if ( ! d ) {
	throw std::string("No RCC Containers found\n");
      }
      rcc_container = static_cast<OCPI::Container::Interface*>(d);
      rcc_application = rcc_container->createApplication();
    }
    catch ( ... ) {
      printf("Got an exception while trying to create the RCC container\n");
      throw;
    }    

    rcc_dThread = new DThread( *rcc_container );
    rcc_dThread->start();

    static  const char * w1Url = "/home/jmiller/projects/openocpi/core/container/linux-dll/test.dll";
    try {
      rcc_art1 = &rcc_application-> loadArtifact(w1Url);
    }
    catch ( ... ) {
      printf("Got an exception while trying to load the RCC artifact %s\n", 
	     w1Url );
      throw;
    }

    try {
      // rcc_consumer = &rcc_application->createWorker( *rcc_art1,"Consumer",0 );
      // rcc_producer = &rcc_application->createWorker( *rcc_art1,"Producer",0 );
      rcc_loopback = &rcc_application->createWorker( *rcc_art1,"Loopback",0 );
    }
    catch ( ... ) {
      printf("Got an exception while trying to create the RCC Workers\n");
      throw;
    }

    try {
      /*
      rccPOut = &rcc_producer->getPort( "Out" );
      rccCIn  = &rcc_consumer->getPort( "In" );
      */

      printf("About to create lb out port\n");
      rccLbOut = &rcc_loopback->getPort( "Out" );

      printf("About to create lb in port\n");
      rccLbIn  = &rcc_loopback->getPort( "In" );

    }
    catch ( std::string& oops ) {
      printf("Got exception %s while trying to create the RCC Worker ports\n", oops.c_str());
      throw;
    }
    catch ( OCPI::Util::EmbeddedException & opps ) {
      printf("Got exception %s while trying to create the RCC Worker ports\n", opps.getAuxInfo() );
      throw;
    }
    catch ( ... ) {
      printf("Got an exception while trying to create the RCC Worker ports\n");
      throw;
    }
  }

  




  if (rplContainer && (!two || rplContainer2)) {
    try {

      // Create an application on this container, no parameters at this time
      std::auto_ptr<CC::Application>
        ap(rplContainer->createApplication( /* "testRPL" */ )),
        ap2(two ? rplContainer2->createApplication( /* "testRpl2" */ ) : 0);


      CC::Application
        &a = *ap,
        &a2 = two ? *ap2 : a;

      CC::Worker *w[12] = {((CC::Worker *)0),
                           &a.createWorker("file", 0, "FC", "FCi"),
                           &a.createWorker("file", 0, "Bias", "BIASi"),
                           &a.createWorker("file", 0, "FP", "FPi"),
                           acquire ?
                           &a.createWorker("file", 0, "ADC", "ADCi") : 0,
                           emit ?
                           &a.createWorker("file", 0, "DAC", "DACi") : 0,
                          two ? &a2.createWorker("file", 0, "FC", "FCi") : 0,
                          two ? &a2.createWorker("file", 0, "Bias", "BIASi") : 0,
                          two ? &a2.createWorker("file", 0, "FP", "FPi") : 0,
                           ((CC::Worker *)0),
                           ((CC::Worker *)0),
                          ((CC::Worker *)0)
      };

      CC::Port &w1in = w[1]->getPort("WMIin");

      CC::Port &w1out = w[1]->getPort("WSIout");

      CC::Port &w1sin = w[1]->getPort("WSIin");

      CC::Port &w2in = w[2]->getPort("WSIin");

      CC::Port &w2out = w[2]->getPort("WSIout");

      CC::Port &w3in = w[3]->getPort("WSIin");

      CC::Port &w3out = w[3]->getPort("WMIout");

      CC::Port &w4out = acquire ? w[4]->getPort("ADCout") : *(CC::Port *)0;

      CC::Port &w6in = two ? w[6]->getPort("WMIin") : *(CC::Port *)0;
      CC::Port &w6out = two ? w[6]->getPort("WSIout") : *(CC::Port *)0;;
      CC::Port &w7in = two ? w[7]->getPort("WSIin") : *(CC::Port *)0;;
      CC::Port &w7out = two ? w[7]->getPort("WSIout") : *(CC::Port *)0;;
      CC::Port &w8in = two ? w[8]->getPort("WSIin") : *(CC::Port *)0;;
      CC::Port &w8out = two ? w[8]->getPort("WMIout") : *(CC::Port *)0;;


     #ifdef WAS
      OCPI::Util::PValue
        p00[] = {CU::PVULong("bufferCount", bufferCount[0][0]),
                 CU::PVString("xferRole", active[0][0]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p01[] = {CU::PVULong("bufferCount", bufferCount[0][1]),
                 CU::PVString("xferRole", active[0][1]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p10[] = {CU::PVULong("bufferCount", bufferCount[1][0]),
                 CU::PVString("xferRole", active[1][0]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p31[] = {CU::PVULong("bufferCount", bufferCount[3][1]),
                 CU::PVString("xferRole", active[3][1]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},

	  /*
        p60[] = {CU::PVULong("bufferCount", bufferCount[6][0]),
                 CU::PVString("xferRole", active[6][0]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
	  */
        p60[] = {CU::PVULong("bufferCount", bufferCount[6][0]),
                 CU::PVString("xferRole", "active"),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},


        p81[] = {CU::PVULong("bufferCount", bufferCount[8][1]),
                 CU::PVString("xferRole", active[8][1]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd};
#endif


      OCPI::Util::PValue
        p00[] = {CU::PVULong("bufferCount", 1),
                 CU::PVString("xferRole", active[0][0]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p01[] = {CU::PVULong("bufferCount", 1),
                 CU::PVString("xferRole", active[0][1]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p10[] = {CU::PVULong("bufferCount", 1),
                 CU::PVString("xferRole", active[1][0]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p31[] = {CU::PVULong("bufferCount", 1),
                 CU::PVString("xferRole", active[3][1]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p60[] = {CU::PVULong("bufferCount", 1),
                 CU::PVString("xferRole", active[6][0]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p81[] = {CU::PVULong("bufferCount", 1),
                 CU::PVString("xferRole", active[8][1]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd};







      CC::Property pfc(*w[1], "control");

      CC::Property pfp(*w[3], "control");

      // Caller knows data type, we can add a debug-mode runtime check
      pfc.setULongValue(acquire ? 0 : 1);

      pfp.setULongValue(emit ? 0 : 2);

      if (loop)
        w1in.loopback(w3out);

      if (acquire)
        w4out.connect(w1sin);


      // Inject a rcc loopback worker here
      w1out.connect(w2in);
      w2out.connect(w3in);


      CC::ExternalPort *myOut =
        acquire ? 0 : &w1in.connectExternal("w0out", p01, p10);


      printf("About to connect\n");

      try {

      if (two) {

	if ( use_rcc ) {

	  OCPI::Util::PValue
	    prcc[] = {CU::PVULong("bufferCount", 1),
		      CU::PVString("xferRole", "active"),
		      CU::PVEnd};

	  OCPI::Util::PValue

	    prcco[] = {CU::PVULong("bufferCount", 1),
		      CU::PVString("xferRole", "flowcontrol"),
		      CU::PVEnd};

	  w3out.connect(*rccLbIn,p31,prcc);
	  rccLbOut->connect(w6in,prcco,p60);

	  rcc_loopback->initialize();
	  rcc_loopback->start();

	}
	else {
	  // out of fpga1 and into fpga2
	  w3out.connect(w6in,p31,p60);
	}

        w6out.connect(w7in);
        w7out.connect(w8in);
      }
      }
      catch (CC::ApiError &e) {
	fprintf(stderr, "API Error: %s\n", e.m_auxInfo.c_str());
	exit(-1);
      }
      catch ( OCPI::Util::EmbeddedException & oops ) {
	printf("Got an exception trying to connect - %s\n", oops.getAuxInfo() );
	exit(-1);
      }
      catch ( std::string & oops ) {
	printf("Got an exception trying to connect - %s\n", oops.c_str() );
	exit(-1);
      }
      catch ( ... ) {
	printf("Got an unknown exception trying to connect\n");
	exit(-1);
      }

      CC::ExternalPort &myIn = two ? w8out.connectExternal ( "w0in", p00, p81 )
                                   : w3out.connectExternal ( "w0in", p00, p31 );

      w[1]->start();
      w[2]->start();
      w[3]->start();
      if (two) {
        w[6]->start();
        w[7]->start();
        w[8]->start();
      }

      printf("Connections complete\n");


      if (acquire)
        w[4]->start();
      unsigned outLeft, inLeft, inN = 0, outN = 0;
      int ifd = -1, cfd = -1, ofd = -1;
      off_t bytes;
      uint8_t *cbuf = (uint8_t*)malloc(ioSize);
      if (file) {
        if ((ifd = open(file, O_RDONLY)) < 0 ||
            (cfd = open(file, O_RDONLY)) < 0 ||
            (bytes = lseek(ifd, SEEK_END, 0)) < 0 ||
            lseek(ifd, SEEK_SET, 0) < 0) {
          fprintf(stderr, "Can't open file \"%s\" for input\n", file);
          return 1;
        }
        inLeft = bytes - (bytes % ioSize);
      } else
        inLeft = ioCount * ioSize;
      outLeft = acquire ? 0 : inLeft;
      if (ofile && (ofd = open(ofile, O_WRONLY|O_CREAT|O_TRUNC)) < 0) {
        fprintf(stderr, "Can't open file \"%s\" for output\n", ofile);
        return 1;
      }


      printf("About to go into the run loop\n");
      //      OCPI::OS::sleep( 3000 );

      // While anything to do
      while (outLeft || inLeft) {

	
	OCPI::OS::sleep( 1 );
	
        uint32_t length;
        uint8_t *data;
        CC::ExternalBuffer *cBuffer;
        if (!acquire) {
          CC::ExternalBuffer *pBuffer;
          // While output to do, do all that can be done
          for (;outLeft && (pBuffer = myOut->getBuffer(data, length));
               outLeft -= ioSize, outN++) {
            assert(length >= ioSize);
            if (file) {
              if (read(ifd, data, ioSize) != (int)ioSize) {
                fprintf(stderr, "Error reading input file\n");
                return 1;
              }
            } else
              for (unsigned w = 0; w < ioSize/sizeof(uint32_t); w++)
                ((uint32_t *)(data))[w] = outN * (ioSize/sizeof(uint32_t)) + w;

	    //	    printf("RPL is putting a buffer\n");
            pBuffer->put(outN, ioSize, false);

          }
          myOut->tryFlush(); // keep buffered output moving
        }

        if (!doread)
          continue;
        uint8_t opCode;
        bool end;
        for (;inLeft && (cBuffer = myIn.getBuffer(opCode, data, length, end));
             inLeft -= ioSize, inN++) {


	  //	  printf("RPL got a buffer of data !!\n");



          uint32_t *d32 = (uint32_t*)data;
          const char *oops = 0;
          if (acquire) {
#if 0
            if (inN == 0) {
              if (opCode != 1 || length != 0) {
                oops = "Initial Acquire opcode/length not 1/0";
                fprintf(stderr, "Bad opcode %d, len %d inN %d should be 1\n",
                      opCode, length, inN);
              }
            } else
#endif
              if (opCode != 0) {
                oops = "Acquire opcode after first not 0";
                fprintf(stderr, "Bad opcode %d, len %d inN %d should be 0\n",
                      opCode, length, inN);
            }
          } else {
            if (opCode != (inN & 0xff)) {
              fprintf(stderr, "Bad opcode %d, len %d inN %d should be 0x%x\n",
                      opCode, length, inN, inN & 0xff);
              oops = "Opcode mismatch on input";
            }
            if (length != ioSize) {
              fprintf(stderr, "Len (%d) should be %ld (d[0] %x, inN %d op %d)\n",
                      length, ioSize, d32[0], inN, opCode);
              oops = "Length mismatch on input";
            }
          }
          if (file) {
            if (read(cfd, cbuf, ioSize) != (int)ioSize) {
              fprintf(stderr, "Error reading input file\n");
              return 1;
            }
            if (memcmp(cbuf, data, ioSize))
              oops = "Data mismatch on file data";
          } else if (ofile) {
            struct {
              uint32_t length, opcode, pad[2];
            } head;
            head.length = length;
            head.opcode = opCode;
            struct iovec io[2] = {
              {&head, sizeof(head)},
              {data, ioSize}
            };
            unsigned n = ioSize ? 2 : 1;
            if (writev(ofd, io, n) != ( int )(sizeof(head) + ioSize)) {
              fprintf(stderr, "Error writing output file\n");
              return 1;
            }
          } else
            for (unsigned w = 0; w < ioSize/sizeof(uint32_t); w++)
              if (d32[w] != inN * ioSize/sizeof(uint32_t) + w) {
                fprintf(stderr, "Bad data 0x%x, len %d w %d inN %d should be 0x%lx\n",
                        d32[w], length, w, inN, inN * ioSize/sizeof(uint32_t) + w);
                oops = "Data mismatch on input";
              }

	  //	  printf("RPL Buffer len = %d, about to release the buffer\n", length );
          cBuffer->release();
          if (oops) {
	    printf("Oops EX\n");
            throw CC::ApiError(oops, NULL);
	  }
        }
      }
      printf("Successfully sent and received %ld messages\n", ioCount);
      if (file)
        printf("Contents of file \"%s\" successfully sent and received\n",
               file);
      if (ofile) {
        printf("File \"%s\" successfully written\n", ofile);
        close(ofd);
      }


      if ( use_rcc ) {
	rcc_dThread->m_run = false;
	rcc_dThread->join();
	delete rcc_application;	
      }


      return 0;
    } catch (CC::ApiError &e) {
      fprintf(stderr, "API Error: %s\n", e.m_auxInfo.c_str());
      return 1;
    }
  } else {
    fprintf(stderr, "No Rpl Containers found\n");
    return 1;
  }
}
      catch (CC::ApiError &e) {
	fprintf(stderr, "Main: API Error: %s\n", e.m_auxInfo.c_str());
	exit(-1);
      }
      catch ( OCPI::Util::EmbeddedException & oops ) {
	printf("Got an EE exception in main - %s\n", oops.getAuxInfo() );
	exit(-1);
      }
      catch ( std::string & oops ) {
	printf("Got a string exception in main - %s\n", oops.c_str() );
	exit(-1);
      }
      catch ( ... ) {
	printf("Got an unknown exception in main\n");
	exit(-1);
      }


return 0;
}
