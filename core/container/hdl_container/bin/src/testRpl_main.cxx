
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
#include <string.h>
#include <stdio.h>
#include <memory>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/time.h>
#include "OcpiThread.h"
#include "OcpiContainerApi.h"

#if defined ( __x86_64__ ) && !defined ( _CPU_IA64 )
#define _CPU_IA64
#elif defined ( __i686__ ) || defined ( __i386__ ) && !defined ( _CPU_IA32 )
#define _CPU_IA32
#elif defined ( __PPC__ ) && !defined ( _CPU_POWERPC )
#define _CPU_POWERPC
#endif

#include "fasttime_private.h"

#define W(s) write(2, s, sizeof(s) - 1)

namespace OA = OCPI::API;

#if 0
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
	(void)exclude;
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
#endif

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

  class  DThread : public OCPI::Util::Thread
  {
    bool m_run;
    OA::Container & m_container;
  public:
    DThread(  OA::Container & c )
      :m_run(true),m_container(c){};
    void stop() {
      m_run = false;
    }
    void run() {
      m_container.thread();
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
    test = false, doTicks = false, metadata = false;
  char *firstarg = 0, *secondarg = 0, *file = 0, *ofile = 0, *xfile = (char*)"file", *rccFile = 0,
    *rccName = 0, *setProp = 0;
  uint32_t setValue;
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
	    "-M\t\tStore  metadata as well as data when writing an output file\n"
            "-NW[io] <nbufs>\tSpecify number of buffers for worker W in or out\n"
            "\t\tOptions are: -n0o -n1i -n3o  -n4i -n6o -n0i\n"
            "\t\tWorker 0 is this test program's input and output\n"
            "-P <pdname>\tSpecify file to dump port data\n"
            "-R\t\tSuppress the read-back and test of what is written\n"
	    "-S\t\tInsert software worker (rcc) on input side\n"
	    "-T\t\tPrint timing information between I/Os\n"
	    "-X prop=value\n"
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
      case 'M':
        metadata = true;
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
      case 'X':
	setProp = *++ap;
	{
	  char *p = strchr(setProp, '=');
	  *p++ = 0;
	  setValue = strtoul(p, NULL, 0);
	}
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
          snprintf(buf, sizeof(buf), "OCPI_OCFRP_DUMMY=%s", *++ap);
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
          snprintf(buf, sizeof(buf), "OCPI_DUMP_PORTS=%s", *++ap);
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
  OA::Container
    *rccContainer = 0,
    *rplContainer = OA::ContainerManager::find("hdl", firstarg),
    *rplContainer2 = two ? OA::ContainerManager::find("hdl", secondarg) : 0;
  
  if (rplContainer && (!two || rplContainer2)) {
    try {
      DThread * rcc_dThread=0;
      if (rccFile) {
	rccContainer = OA::ContainerManager::find("rcc");
	rcc_dThread = new DThread( *rccContainer );
	rcc_dThread->start();
      }

      // Create an application on this container, no parameters at this time
      std::auto_ptr<OA::ContainerApplication>
        ap(rplContainer->createApplication( /* "testRPL" */ )),
        ap2(two ? rplContainer2->createApplication( /* "testRpl2" */ ) : 0),
	apRcc(rccFile ? rccContainer->createApplication() : 0);


      OA::ContainerApplication
        &a = *ap,
        &a2 = two ? *ap2 : a,
	&rcca = rccFile ? *apRcc : a;

      OA::Worker *w[14] = {
	((OA::Worker *)0),
	// SMA0 in first FPGA
	&a.createWorker(xfile, 0, "w2c1", "FC", "FCi"),                                 // w2# 1
	// MIddle worker in first FPGA
	&a.createWorker(xfile, 0, "w3c2", "Bias", "BIASi"),                             // w3# 2
	// SMA1 in first FPGA
	&a.createWorker(xfile, 0, "w4c3", "FP", "FPi"),                                 // w4# 3
	// ADC in first FPGA
	acquire ? &a.createWorker(xfile, 0, "w10c1", "ADC", "ADCi") : 0,                 //w10# 4
	// DAC in first FPGA
	emit ? &a.createWorker(xfile, 0, "w11c1", "DAC", "DACi") : 0,                    //w11# 5
	// SMA0 in second FPGA
	two ? &a2.createWorker(xfile, 0, "w2c2", "FC", "FCi") : 0,                      // w2# 6
	// Middle worker in second FPGA
	two ? &a2.createWorker(xfile, 0, "w3c2", "Bias", "BIASi") : 0,                  // w3# 7
	// SMA1 in second FPGA
	two ? &a2.createWorker(xfile, 0, "w4c2", "FP", "FPi") : 0,                      // w4# 8
	// Splitter from (SMA0|ADC) to (Middle|Framgate) in first FPGA
	psd ? &a.createWorker(xfile, 0, "w?c1", "WsiSplitter2x2", "WsiSplitter2x2i") :
	// Splitter from (SMA0|nothing) to (split1|Middle) in first FPGA
	(test ? &a.createWorker(xfile, 0, "w5c1", "splitter2x2", "split0") : 0 ),       // w5# 9
	// If PSD, Framegate, else if test, Middle/WUT
	psd ? &a.createWorker(xfile, 0, "w?c1", "FrameGate", "FrameGatei") :          
	(test ? &a.createWorker(xfile, 0, "w6c1", "psd", "psd") : 0),                   // w6#10
	// If PSD, PSD, else if test, Middle/WUT, else (split0|middle) to SMA1
	psd ? &a.createWorker(xfile, 0, "w?c1", "PSD", "PSDi") :
	(test ? &a.createWorker(xfile, 0, "w7c1", "splitter2x2", "split1") : 0 ),       // w7#11
	// Dram worker
	psd ? &a.createWorker(xfile, 0, "w12c1", "DramServer", "DramServeri") : 0,       //w12#12
	// RCC worker
	rccFile ? &rcca.createWorker(rccFile, 0, "wNcRcc", rccName, 0) : 0                //   #13
      };

      OA::Port &w1in = w[1]->getPort("WMIin");

      OA::Port &w1out = w[1]->getPort("WSIout");

      OA::Port &w1sin = w[1]->getPort("WSIin");

      OA::Port &w2in = w[2]->getPort("WSIin");

      OA::Port &w2out = w[2]->getPort("WSIout");

      OA::Port &w3in = w[3]->getPort("WSIin");

      OA::Port &w3out = w[3]->getPort("WMIout");
      OA::Port &w3sout = w[3]->getPort("WSIout");

      OA::Port &w4out = acquire ? w[4]->getPort("ADCout") : *(OA::Port *)0;

      OA::Port &w5in = emit ? w[5]->getPort("DACin") : *(OA::Port *)0;

      OA::Port &w6in = two ? w[6]->getPort("WMIin") : *(OA::Port *)0;
      OA::Port &w6out = two ? w[6]->getPort("WSIout") : *(OA::Port *)0;
      OA::Port &w7in = two ? w[7]->getPort("WSIin") : *(OA::Port *)0;
      OA::Port &w7out = two ? w[7]->getPort("WSIout") : *(OA::Port *)0;
      OA::Port &w8in = two ? w[8]->getPort("WSIin") : *(OA::Port *)0;
      OA::Port &w8out = two ? w[8]->getPort("WMIout") : *(OA::Port *)0;
      OA::Port &w9in0 = psd ? w[9]->getPort("WSIinB") : *(OA::Port *)0;
      OA::Port &w9in1 = psd ? w[9]->getPort("WSIinA") : *(OA::Port *)0;
      OA::Port &w9out0 = psd ? w[9]->getPort("WSIoutC") : *(OA::Port *)0;
      // The issue here is that input ports get their protocol bound at
      // construction time rather than connection time.
      if (rccFile)
	putenv((char *)"OCPI_DEFAULT_PROTOCOL=ocpi-pci-pio");
      OA::Port &w13in = rccFile ? w[13]->getPort("in") : *(OA::Port*)0;
      OA::Port &w13out = rccFile ? w[13]->getPort("out") : *(OA::Port*)0;

      OA::PValue
        p00[] = {OA::PVULong("bufferCount", bufferCount[0][0]),
                 OA::PVString("xferRole", active[0][0]),
                 OA::PVULong("bufferSize", bufferSize), OA::PVEnd},
        p01[] = {OA::PVULong("bufferCount", bufferCount[0][1]),
                 OA::PVString("xferRole", active[0][1]),
                 OA::PVULong("bufferSize", bufferSize), OA::PVEnd},
        p10[] = {OA::PVULong("bufferCount", bufferCount[1][0]),
                 OA::PVString("xferRole", active[1][0]),
                 OA::PVULong("bufferSize", bufferSize), OA::PVEnd},
        p31[] = {OA::PVULong("bufferCount", bufferCount[3][1]),
                 OA::PVString("xferRole", active[3][1]),
                 OA::PVULong("bufferSize", bufferSize), OA::PVEnd},
        p60[] = {OA::PVULong("bufferCount", bufferCount[6][0]),
                 OA::PVString("xferRole", active[6][0]),
                 OA::PVULong("bufferSize", bufferSize), OA::PVEnd},
        p81[] = {OA::PVULong("bufferCount", bufferCount[8][1]),
                 OA::PVString("xferRole", active[8][1]),
                 OA::PVULong("bufferSize", bufferSize), OA::PVEnd},
        p90[] = {OA::PVULong("bufferCount", bufferCount[9][0]),
                 OA::PVULong("bufferSize", bufferSize), OA::PVEnd},
        p91[] = {OA::PVULong("bufferCount", bufferCount[9][1]),
                 OA::PVULong("bufferSize", bufferSize), OA::PVEnd};


      OA::Property pfc(*w[1], "control");
      OA::Property pfp(*w[3], "control");
      if (psd) {
	OA::Property splitCtrl(*w[9], "splitCtrl");
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
     OA::ExternalPort *myOut =
        acquire ? 0 : &w1in.connectExternal("w0out", p01, p10);

      if (two) {
        w3out.connect(w6in,p31,p60);
        w6out.connect(w7in);
        w7out.connect(w8in);
      }

      if (rccFile) {
	w3out.connect(w13in,p31,p90);
      }
      OA::ExternalPort &myIn =
	two ? w8out.connectExternal ( "w0in", p00, p81 ) :
	rccFile ? w13out.connectExternal ( "w13out", p00, p91) :
	w3out.connectExternal ( "w0in", p00, p31 );

      if (setProp) {
	OA::Property p(*w[2], setProp);
	p.setULongValue(setValue);
      }
      if (delay) {
	OA::Property dlyCtrl(*w[2], "dlyCtrl");
	OA::Property dlyHoldoffBytes(*w[2], "dlyHoldoffBytes");
	OA::Property dlyHoldoffCycles(*w[2], "dlyHoldoffCycles");
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
	OA::Property dacCtrl(*w[5], "dacControl");
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
            (bytes = lseek(ifd, 0, SEEK_END)) < 0 ||
            lseek(ifd, 0, SEEK_SET) < 0) {
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
      if (ofile && (ofd = open(ofile, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0) {
        fprintf(stderr, "Can't open file \"%s\" for output\n", ofile);
        return 1;
      }

      struct timeval tv0, tv1, tv2;
      static tick_t ticks[NTICKS + 10] = {{{0,0}}};
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
        OA::ExternalBuffer *cBuffer;
        if (!acquire) {
          OA::ExternalBuffer *pBuffer;
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
              for (unsigned i = 0; i < ioSize/sizeof(uint32_t); i++)
                ((uint32_t *)(data))[i] = outN * (ioSize/sizeof(uint32_t)) + i;
	    if (doTick)
	      get_tick_count(&ticks[tick++]);
            pBuffer->put(ioSize, outN, false);
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
        for (;inLeft && (cBuffer = myIn.getBuffer(data, length, opCode, end));
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
          if (file && !ofile) {
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
	    unsigned nVecs, vecOffset;
	    ssize_t nBytes;
	    if (metadata) {
	      nVecs = ioSize ? 2 : 1;
	      nBytes = sizeof(head) + ioSize;
	      vecOffset = 0;
	    } else {
	      nVecs = ioSize ? 1 : 0;
	      nBytes = ioSize;
	      vecOffset = 1;
	    }
	    if (nBytes &&
		writev(ofd, io + vecOffset, nVecs) != nBytes) {
              fprintf(stderr, "Error writing output file\n");
              return 1;
            }
          } else if (!dummy)
            for (unsigned i = 0; i < ioSize/sizeof(uint32_t); i++)
              if (d32[i] != inN * ioSize/sizeof(uint32_t) + i) {
                fprintf(stderr, "Bad data 0x%x, len %d w %d inN %d should be 0x%lx\n",
                        d32[i], length, i, inN, inN * ioSize/sizeof(uint32_t) + i);
                oops = "Data mismatch on input";
              }
          cBuffer->release();
          if (oops)
            throw "oops";
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
	rcc_dThread->stop();
	rcc_dThread->join();
      }
      if (doTicks) {
	tick_t *tp = ticks + 1;
	for (unsigned n = 0; tp->ll && n <= NTICKS; n++, tp++) {
	  printf("%3d: %20lld\n", n, (long long)(tp->ll - (tp - 1)->ll));
	}
      }
      return 0;
    } catch (OA::Error &e) {
      fprintf(stderr, "API Error: %s\n", e.m_error.c_str());
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
