#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <memory>
#include <sys/uio.h>
#include <CpiDriver.h>
#include "CpiApi.h"

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


int main(int argc, char *argv[])
{
  try
  {
  unsigned long bufferCount[10][2] =
    {{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2}};
  const char *active[10][2] = {{0}};
  unsigned long bufferSize = 2048, ioCount = 1, ioSize = 16, memorySize = 0;
  uint64_t memoryBase = 0;
  bool probe = false, loop = false, doread = true, two = false, same = true,
    acquire = false, emit = false;
  char *firstarg = 0, *secondarg = 0, *file = 0, *ofile = 0;
  if (argc == 1) {
    fprintf(stderr, "Usage is: testRpl <options> [<container-name>]\n");
    fprintf(stderr, "  Options are:\n"
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
          snprintf(buf, sizeof(buf), "CPI_OCFRP_DUMMY=%s", *++ap);
          putenv(buf);
        }
        break;
      case 'P':
        {
          static char buf[100];
          snprintf(buf, sizeof(buf), "CPI_DUMP_PORTS=%s", *++ap);
          putenv(buf);
        }
        break;
      case 'i':
        ioCount = atoi(*++ap);
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
  CC::Interface *rplContainer, *rplContainer2;



  CPI::RPL::Driver driver();

  CPI::Util::DriverManager dm("OCFRP");



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

      w1out.connect(w2in);

      w2out.connect(w3in);

      CC::ExternalPort *myOut =
        acquire ? 0 : &w1in.connectExternal("w0out", p01, p10);

      if (two) {
        w3out.connect(w6in,p31,p60);
        w6out.connect(w7in);
        w7out.connect(w8in);
      }

      CC::ExternalPort &myIn =
        two ? w8out.connectExternal("w0in", p00, p81) : w3out.connectExternal("w0in", p00, p31);

      w[1]->start();
      w[2]->start();
      w[3]->start();
      if (two) {
        w[6]->start();
        w[7]->start();
        w[8]->start();
      }

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

      // While anything to do
      while (outLeft || inLeft) {
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
            if (writev(ofd, io, n) != sizeof(head) + ioSize) {
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
          cBuffer->release();
          if (oops)
            throw CC::ApiError(oops, NULL);
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
