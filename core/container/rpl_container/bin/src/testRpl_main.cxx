#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <memory>
#include <math.h>
#include <time.h>
#include <sys/uio.h>
#include <sys/time.h>
#include "CpiOsMisc.h"
#include "CpiThread.h"
#include "CpiDriver.h"
#include "CpiApi.h"
#define _CPU_IA32
#include "fasttime_private.h"

#define W(s) write(2, s, sizeof(s) - 1)

namespace CC = CPI::Container;
namespace CM = CPI::Metadata;


namespace CU = CPI::Util;

namespace CPI {
  namespace RPL {

    class Driver : public CPI::Util::Driver {
      // The fd for mapped memory, until we have a driver to restrict it.
      static int pciMemFd;
    public:
      // This constructor simply registers itself. This class has no state.
      Driver() :
        CPI::Util::Driver("OCFRP","Global",true) {
      }
      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      // It uses a generic PCI scanner to find candidates, and when found, calls the "found" method below.
      virtual unsigned search(const CPI::Util::PValue*, const char **exclude)
        throw (CPI::Util::EmbeddedException)
      {

        if (getenv("CPI_OCFRP_DUMMY"))
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
      virtual CPI::Util::Device *probe(const CPI::Util::PValue*, const char *which  )
        throw (CPI::Util::EmbeddedException);

      virtual ~Driver()
        throw() {};

    };

  }
}

#define D2E7  128.0
#define D2E15 32768.0
#define NPTS  4096
#define PI    3.1415926536
#define FREQ  32.0  // In samples per radian, divide by 2 to get fraction of Nyquist
#define GAIN  0.99
// fill a buffer of "length" bytes with 16 bit sized cosine values
static double freq = FREQ;
static void
doCosine(int16_t *data, unsigned length) {
  unsigned npts = (length + sizeof(*data) - 1) / sizeof(*data);
  double phi = 0.0, dphi = 2.*PI / freq;

  for (unsigned i=0; i<npts ;i++, phi += dphi) {
    double d = cos(phi) * D2E15 * GAIN + 0.5;
    int16_t s = (int16_t)d;
    //    printf(" 0x%4x %4d %f\n", s & 0xffff, s, d);
    *data++ = s;
  }
}

static void memcpy64(uint64_t *to, uint64_t *from, unsigned nbytes)
{
  while (nbytes > 128) {
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    nbytes -= 128;
  }
  while (nbytes > 8) {
    *to++ = *from++;
    nbytes -= 8;
  }
  if (nbytes)
    memcpy(to, from, nbytes);
}

namespace {

  class  DThread : public CPI::Util::Thread
  {
  public:
    bool m_run;
    CC::Interface & m_interface;
    DThread(  CC::Interface & i )
      :m_run(true),m_interface(i){};

    void run() {
      while(m_run) {
	m_interface.dispatch(NULL);
        CPI::OS::sleep(0);
      }
    }

  };

}

int main(int argc, char *argv[])
{
  try
  {
  unsigned long bufferCount[10][2] =
    {{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2}};
  const char *active[10][2] = {{0}};
  unsigned long bufferSize = 2048, ioCount = 1, ioSize = 16, memorySize = 0, delay = 0;
#define NTICKS 100
  uint64_t memoryBase = 0;
  bool probe = false, loop = false, doread = true, two = false, same = true,
    acquire = false, emit = false, dummy = false, cosine = false, psd = false,
    test = false, doTicks = false;
  char *firstarg = 0, *secondarg = 0, *file = 0, *ofile = 0, *xfile = (char*)"file", *rccFile = 0, *rccName = 0;
  if (argc == 1) {
    fprintf(stderr, "Usage is: testRpl <options> [<container-name>][<second-container-name>]\n");
    fprintf(stderr, "  Options are:\n"
            "-a\t\tAcquire data from ADC, not file (-f) or pattern or cosien (-c)\n"
	    "-c\t\tCosine data generator used for test data\n"
            "-d\t\tProbe specific device, don't discover all\n"
            "-e\t\tEmit data to DAC, don't return it to SW\n"
            "-f <file>\tSpecify file to use for test data content\n"
            "-i <iocoiunt>\tSpecify number of message to send(default == 1)\n"
            "-l\t\tEnter the emulator loopback mode\n"
            "-m <addr> <size>Specify pinned DMA memory address and size\n"
            "-n <nbufs>\tSpecify number of buffers for all (default == 2)\n"
            "-o\t\tOutput file to write data into\n"
	    "-p\t\tUse the PSD bitstream configuration\n"
            "-rW[io][mfap]\tRole for DMA for worker W in or out\n"
            "-s <bufsize>\tSpecify size of buffers (default is larger of 2048 or -I)\n"
            "-u\t\tUse unit test bitstream configuration\n"
	    "-x <xfile>\tUse the <xfile> for xml bitstream file\n"
	    "-y\t\tSet delay hold off byte count, enable delay, start DRAM\n"
	    "-z\t\tDon't touch data at all, just send and receive junk\n"
            "-D <shmname>\tSpecify emulated hardware buffer shm\n"
            "-I <msgsize>\tSpecify size of messages (default == 16 bytes)\n"
            "-NW[io] <nbufs>\tSpecify number of buffers for worker W in or out\n"
            "\t\tOptions are: -n0o -n1i -n3o  -n4i -n6o -n0i\n"
            "\t\tWorker 0 is this test program's input and output\n"
            "-P <pdname>\tSpecify file to dump port data\n"
            "-R\t\tSuppress the read-back and test of what is written\n"
	    "-S\t\tInsert software worker (rcc) on input side\n"
	    "-T\t\tPrint timing information between I/Os\n"
            );
    return 1;
  }
  for (char **ap = &argv[1]; *ap; ap++)
    if (**ap == '-')
      switch ((*ap)[1]) {
      case 'a':
        acquire = true;
        break;
      case 'c':
	cosine = true;
	if ((*ap)[2])
	  freq = strtod(&(*ap)[2], NULL);
	break;
      case 'd':
        probe = true;
        break;
      case 'e':
	emit = true;
	doread = false;
	break;
      case 'f':
        file = *++ap;
        break;
      case 'i':
        ioCount = atoi(*++ap);
        break;
      case 'l':
        loop = true;
        break;
      case 'm':
        memoryBase = atoll(*++ap);
        memorySize = atoi(*++ap);
        break;
      case 'n':
        {
          unsigned n = atoi(*++ap);
          for (unsigned i = 0; i < 10; i++)
            bufferCount[i][0] = bufferCount[i][1] = n;
        }
        break;
      case 'o':
        ofile = *++ap;
        break;
      case 'p':
	psd = true;
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
      case 's':
        bufferSize = atoi(*++ap);
        break;
      case 'u':
	test = true;
	break;
      case 'x':
        xfile = *++ap;
        break;
      case 'y':
        delay = atoi(*++ap);
	break;
      case 'z':
	dummy = true;
	break;
      case 'D':
        {
          static char buf[100];
          snprintf(buf, sizeof(buf), "CPI_OCFRP_DUMMY=%s", *++ap);
          putenv(buf);
        }
        break;
      case 'I':
        ioSize = atoi(*++ap);
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
      case 'P':
        {
          static char buf[100];
          snprintf(buf, sizeof(buf), "CPI_DUMP_PORTS=%s", *++ap);
          putenv(buf);
        }
        break;
      case 'R':
        doread = false;
        break;
      case 'S':
        rccFile = *++ap;
	{
	  char *cp = strrchr(rccFile, '/');
	  rccName = strdup(cp ? cp + 1 : rccFile);
	  cp = strchr(rccName, '.');
	  if (cp)
	    *cp = 0;
	}
	break;
      case 'T':
	doTicks = true;
	break;
      case '2':
        two = true;
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
  printf("Buffer counts: 0o %lu 1i %lu 3o %lu 6i %lu 8o %lu 0i %lu 9i %lu 9o %lu\n",
         bufferCount[0][1],
         bufferCount[1][0], bufferCount[3][1], bufferCount[6][0],
         bufferCount[8][1], bufferCount[0][0], bufferCount[9][0], bufferCount[9][1]);
  printf("Active indicators: 0o %s 1i %s 3o %s 6i %s 8o %s 0i %s\n",
         active[0][1],
         active[1][0], active[3][1], active[6][0],
         active[8][1], active[0][0]);
  if (two)
    printf("Using two boards: %s and %s\n", firstarg, secondarg);
  // If we know there is one, try to create it.
  CC::Interface *rplContainer, *rplContainer2 = 0, *rccContainer = 0;

#ifdef WAS
  CPI::RPL::Driver driver();
  CPI::Util::DriverManager dm("OCFRP");
#else
  CPI::RPL::Driver & driver = *(new CPI::RPL::Driver());
  CPI::Util::DriverManager & dm = *(new CPI::Util::DriverManager("OCFRP"));

#endif
#ifdef WANTSTOBE
  // Find the container we want
  CPI::Container
    *rplContainer = CPI::ContainerManager::find("HDL",,,firstarg),
    *rplContainer2 = two ? CPI::ContainerManager::find("HDL",,,secondarg);
#endif
  if (probe) {
    rplContainer = static_cast<CC::Interface*>(dm.getDevice( 0, "0" ));
  } else {


    dm.discoverDevices(0,0);

    rplContainer = static_cast<CC::Interface*>(dm.getDevice( 0, firstarg ));


#ifdef WAS
    rplContainer = CU::Driver::getContainer("OCFRP", firstarg, "", 0);
#endif



    if (two) {

#ifdef WAS
      rplContainer2 = CU::Driver::getContainer("OCFRP", secondarg, "", 0);
#endif
      rplContainer2 = static_cast<CC::Interface*>(dm.getDevice( 0, secondarg ));

    }
  }

  if (rplContainer && (!two || rplContainer2)) {
    try {
      DThread * rcc_dThread;
      if (rccFile) {
	static CPI::Util::DriverManager rcc_dm("Container");
	static CPI::Util::PValue cprops[] = {
	  CPI::Util::PVBool("polling",1),
	  CPI::Util::PVEnd };
	rcc_dm.discoverDevices(0,0);
	CPI::Util::Device* d = rcc_dm.getDevice( cprops, "RCC");
	if ( ! d ) {
	  throw std::string("No RCC Containers found\n");
	}
	rccContainer = static_cast<CPI::Container::Interface*>(d);
	rcc_dThread = new DThread( *rccContainer );
	rcc_dThread->start();
      }

      // Create an application on this container, no parameters at this time
      std::auto_ptr<CC::Application>
        ap(rplContainer->createApplication( /* "testRPL" */ )),
        ap2(two ? rplContainer2->createApplication( /* "testRpl2" */ ) : 0),
	apRcc(rccFile ? rccContainer->createApplication() : 0);


      CC::Application
        &a = *ap,
        &a2 = two ? *ap2 : a,
	&rcca = rccFile ? *apRcc : a;

      CC::Worker *w[14] = {
	((CC::Worker *)0),
	&a.createWorker(xfile, 0, "FC", "FCi"),                                 // w2# 1
	&a.createWorker(xfile, 0, "Bias", "BIASi"),                             // w3# 2
	&a.createWorker(xfile, 0, "FP", "FPi"),                                 // w4# 3
	acquire ? &a.createWorker(xfile, 0, "ADC", "ADCi") : 0,                 //w10# 4
	emit ? &a.createWorker(xfile, 0, "DAC", "DACi") : 0,                    //w11# 5
	two ? &a2.createWorker(xfile, 0, "FC", "FCi") : 0,                      // w2# 6
	two ? &a2.createWorker(xfile, 0, "Bias", "BIASi") : 0,                  // w3# 7
	two ? &a2.createWorker(xfile, 0, "FP", "FPi") : 0,                      // w4# 8
	psd ? &a.createWorker(xfile, 0, "WsiSplitter2x2", "WsiSplitter2x2i") :
	(test ? &a.createWorker(xfile, 0, "splitter2x2", "split0") : 0 ),       // w5# 9
	psd ? &a.createWorker(xfile, 0, "FrameGate", "FrameGatei") :          
	(test ? &a.createWorker(xfile, 0, "psd", "psd") : 0),                   // w6#10
	psd ? &a.createWorker(xfile, 0, "PSD", "PSDi") :
	(test ? &a.createWorker(xfile, 0, "splitter2x2", "split1") : 0 ),       // w7#11
	psd ? &a.createWorker(xfile, 0, "DramServer", "DramServeri") : 0,       //w12#12
	rccFile ? &rcca.createWorker(rccFile, 0, rccName, 0) : 0             //   #13
      };

      CC::Port &w1in = w[1]->getPort("WMIin");

      CC::Port &w1out = w[1]->getPort("WSIout");

      CC::Port &w1sin = w[1]->getPort("WSIin");

      CC::Port &w2in = w[2]->getPort("WSIin");

      CC::Port &w2out = w[2]->getPort("WSIout");

      CC::Port &w3in = w[3]->getPort("WSIin");

      CC::Port &w3out = w[3]->getPort("WMIout");
      CC::Port &w3sout = w[3]->getPort("WSIout");

      CC::Port &w4out = acquire ? w[4]->getPort("ADCout") : *(CC::Port *)0;

      CC::Port &w5in = emit ? w[5]->getPort("DACin") : *(CC::Port *)0;

      CC::Port &w6in = two ? w[6]->getPort("WMIin") : *(CC::Port *)0;
      CC::Port &w6out = two ? w[6]->getPort("WSIout") : *(CC::Port *)0;
      CC::Port &w7in = two ? w[7]->getPort("WSIin") : *(CC::Port *)0;
      CC::Port &w7out = two ? w[7]->getPort("WSIout") : *(CC::Port *)0;
      CC::Port &w8in = two ? w[8]->getPort("WSIin") : *(CC::Port *)0;
      CC::Port &w8out = two ? w[8]->getPort("WMIout") : *(CC::Port *)0;
      CC::Port &w9in0 = psd ? w[9]->getPort("WSIinB") : *(CC::Port *)0;
      CC::Port &w9in1 = psd ? w[9]->getPort("WSIinA") : *(CC::Port *)0;
      CC::Port &w9out0 = psd ? w[9]->getPort("WSIoutC") : *(CC::Port *)0;
      // The issue here is that input ports get their protocol bound at
      // construction time rather than connection time.
      if (rccFile)
	putenv("CPI_DEFAULT_PROTOCOL=cpi-pci-pio");
      CC::Port &w13in = rccFile ? w[13]->getPort("in") : *(CC::Port*)0;
      CC::Port &w13out = rccFile ? w[13]->getPort("out") : *(CC::Port*)0;

      CPI::Util::PValue
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
        p60[] = {CU::PVULong("bufferCount", bufferCount[6][0]),
                 CU::PVString("xferRole", active[6][0]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p81[] = {CU::PVULong("bufferCount", bufferCount[8][1]),
                 CU::PVString("xferRole", active[8][1]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p90[] = {CU::PVULong("bufferCount", bufferCount[9][0]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd},
        p91[] = {CU::PVULong("bufferCount", bufferCount[9][1]),
                 CU::PVULong("bufferSize", bufferSize), CU::PVEnd};


      CC::Property pfc(*w[1], "control");
      CC::Property pfp(*w[3], "control");
      if (psd) {
	CC::Property splitCtrl(*w[9], "splitCtrl");
	const unsigned IN_FROM_ADC = 0x1, IN_FROM_SMA0 = 0x100;
	splitCtrl.setULongValue(acquire ? IN_FROM_ADC : IN_FROM_SMA0);
      }

      // Caller knows data type, we can add a debug-mode runtime check
      pfc.setULongValue(acquire ? 0 : 1);

      pfp.setULongValue(emit ? 0 : 2);
      

      if (loop)
        w1in.loopback(w3out);

      if (acquire)
        w4out.connect(psd ? w9in1 : w1sin);

      // connect sma0 either directory to bias/delay, or if psd, to the second input of the splitter
      w1out.connect(psd ? w9in0 : w2in);
      if (psd)
	w9out0.connect(w2in);

      w2out.connect(w3in);

      if (emit) {
	w3sout.connect(w5in);
      }
     CC::ExternalPort *myOut =
        acquire ? 0 : &w1in.connectExternal("w0out", p01, p10);

      if (two) {
        w3out.connect(w6in,p31,p60);
        w6out.connect(w7in);
        w7out.connect(w8in);
      }

      if (rccFile) {
	w3out.connect(w13in,p31,p90);
      }
      CC::ExternalPort &myIn =
	two ? w8out.connectExternal ( "w0in", p00, p81 ) :
	rccFile ? w13out.connectExternal ( "w13out", p00, p91) :
	w3out.connectExternal ( "w0in", p00, p31 );

      if (delay) {
	CC::Property dlyCtrl(*w[2], "dlyCtrl");
	CC::Property dlyHoldoffBytes(*w[2], "dlyHoldoffBytes");
	CC::Property dlyHoldoffCycles(*w[2], "dlyHoldoffCycles");
	dlyCtrl.setULongValue(7);
	dlyHoldoffBytes.setULongValue(delay);
	w[12]->start();
      }

      w[1]->start();
      w[2]->start();
      w[3]->start();
      if (two) {
        w[6]->start();
        w[7]->start();
        w[8]->start();
      }
      if (psd)
	w[9]->start();
      if (emit) {
	CC::Property dacCtrl(*w[5], "dacControl");
	dacCtrl.setULongValue(0x78);
	w[5]->start();
      }
      if (acquire)
        w[4]->start();
      if (rccFile)
	w[13]->start();
      unsigned outLeft = 0, inLeft = 0, inN = 0, outN = 0;
      int ifd = -1, cfd = -1, ofd = -1;
      off_t bytes;
      static int16_t cosineBuf[4096];
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
      } else {
	if (cosine)
	  doCosine(cosineBuf, sizeof(cosineBuf));
	if (!emit)
	  inLeft = ioCount * ioSize;
      }
      outLeft = acquire ? 0 : (emit ? ioCount * ioSize : inLeft);
      if (ofile && (ofd = open(ofile, O_WRONLY|O_CREAT|O_TRUNC)) < 0) {
        fprintf(stderr, "Can't open file \"%s\" for output\n", ofile);
        return 1;
      }

      struct timeval tv0, tv1, tv2;
      static tick_t ticks[NTICKS + 10] = {{{0}}};
      unsigned tick = 0;
      bool doTick = doTicks;
      gettimeofday(&tv0, 0);
      gettimeofday(&tv1, 0);
      // While anything to do
      if (doTick)
	get_tick_count(&ticks[tick++]);
      while (outLeft || inLeft) {
        uint32_t length;
        uint8_t *data;
        CC::ExternalBuffer *cBuffer;
        if (!acquire) {
          CC::ExternalBuffer *pBuffer;
          // While output to do, do all that can be done
          for (;outLeft && (pBuffer = myOut->getBuffer(data, length));
               outLeft -= ioSize, outN++) {
	    if (doTick)
	      get_tick_count(&ticks[tick++]);
            assert(length >= ioSize);
            if (file) {
              if (read(ifd, data, ioSize) != (int)ioSize) {
                fprintf(stderr, "Error reading input file\n");
                return 1;
              }
            } else if (cosine)
	      memcpy64((uint64_t*)data, (uint64_t*)cosineBuf, ioSize);
	    else if (!dummy)
              for (unsigned w = 0; w < ioSize/sizeof(uint32_t); w++)
                ((uint32_t *)(data))[w] = outN * (ioSize/sizeof(uint32_t)) + w;
	    if (doTick)
	      get_tick_count(&ticks[tick++]);
            pBuffer->put(outN, ioSize, false);
	    if (doTick) {
	      get_tick_count(&ticks[tick++]);
	      if (tick >= NTICKS)
		doTick = false;
	    }
          }
          myOut->tryFlush(); // keep buffered output moving
        }

        if (!doread)
          continue;
        uint8_t opCode;
        bool end;
        for (;inLeft && (cBuffer = myIn.getBuffer(opCode, data, length, end));
             inLeft -= ioSize, inN++) {
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
            if (writev(ofd, io, n) != (int)(sizeof(head) + ioSize)) {
              fprintf(stderr, "Error writing output file\n");
              return 1;
            }
          } else if (!dummy)
            for (unsigned w = 0; w < ioSize/sizeof(uint32_t); w++)
              if (d32[w] != inN * ioSize/sizeof(uint32_t) + w) {
                fprintf(stderr, "Bad data 0x%x, len %d w %d inN %d should be 0x%lx\n",
                        d32[w], length, w, inN, inN * ioSize/sizeof(uint32_t) + w);
                oops = "Data mismatch on input";
              }
          cBuffer->release();
          if (oops)
            throw CC::ApiError(oops, NULL);
        }
      }
      if (!doread && myOut->tryFlush()) {
	static const struct timespec t = { 0, 1};
	nanosleep(&t, NULL);
      }
      gettimeofday(&tv2, 0);
      uint64_t t0 = tv0.tv_sec * 1000000LL + tv0.tv_usec;
      uint64_t t1 = tv1.tv_sec * 1000000LL + tv1.tv_usec;
      uint64_t t2 = tv2.tv_sec * 1000000LL + tv2.tv_usec;
      uint64_t ioBytes = (uint64_t)ioCount * ioSize;
      fprintf(stderr, "Bytes %lld, Time delta = %lld, %f MBytes/seconds, Framesize %lu\n",
	      (long long)ioBytes,(long long)((t2 - t1) - (t1 - t0)),
	      (double)ioBytes/(double)((t2-t1)-(t1-t0)), ioSize);
      printf("Successfully %s%s%s %ld messages\n",
	     acquire ? "" : "sent",
	     acquire || emit ? "" : " and ",
	     emit ? "" : "received",
	     ioCount);
      if (file)
        printf("Contents of file \"%s\" successfully sent and received\n",
               file);
      if (ofile) {
        printf("File \"%s\" successfully written\n", ofile);
        close(ofd);
      }
      if (rccFile ) {
	rcc_dThread->m_run = false;
	rcc_dThread->join();
      }
      if (doTicks) {
	tick_t *tp = ticks + 1;
	for (unsigned n = 0; tp->ll && n <= NTICKS; n++, tp++) {
	  printf("%3d: %20lld\n", n, (long long)(tp->ll - (tp - 1)->ll));
	}
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
catch ( ... )
{
  printf ( "Got an exception\n" );
}
return 0;
}
