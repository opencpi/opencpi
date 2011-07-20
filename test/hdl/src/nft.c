#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include "OCCP.h"
#include "OCDP.h"
#include "getPci.h"

#define WORKER_DP0 (13)
#define WORKER_DP1 (14)
#define WORKER_SMA0 (2)
#define WORKER_BIAS (3)
#define WORKER_SMA1 (4)
#define OCDP_OFFSET_DP0 (32*1024)
#define OCDP_OFFSET_DP1 0

#ifdef NDEBUG
#undef assert
#define assert(foo) (foo)
#endif

// Packet of arguments required to process a stream endpoint on the cpu side
typedef struct {
  bool isToCpu;
  unsigned bufSize, nBufs;
  volatile uint8_t *buffers;
  volatile OcdpMetadata *metadata;
  volatile uint32_t *flags;
  volatile uint32_t *doorbell;
  // Then the dynamic state of the stream
  unsigned bufIdx, opCode;
  char *buf; // local buffer
} Stream;

// Forward declarations
static void
  reset(volatile OccpWorkerRegisters *, unsigned),
  init(volatile OccpWorkerRegisters *),
  start(volatile OccpWorkerRegisters *),
  *doStream(void *args),
  setupStream(Stream *s, volatile OcdpProperties *p, bool isToCpu,
	      unsigned nCpuBufs, unsigned nFpgaBufs, unsigned bufSize,
	      uint8_t *cpuBase, unsigned long long dmaBase, uint32_t *offset);

static unsigned checkStream(Stream *s);
bool noData = 0, noCheck = 0, verbose = 0, single = 0;
unsigned maxFrames = 0, bufSize = 4096;
uint64_t bytes;
 int
main(int argc, char *argv[])
{
  unsigned dmaMeg;
  unsigned long long dmaBase;
  int fd;
  OccpSpace *occp;
  uint8_t *bar1, *cpuBase;
  const char *dmaEnv;
  volatile OcdpProperties *dp0Props, *dp1Props;
  volatile uint32_t *sma0Props, *sma1Props, *biasProps;
  volatile OccpWorkerRegisters *dp0, *dp1, *sma0, *sma1, *bias;
  unsigned nFpgaBufs = 2, nCpuBufs = 200;
  // These structures define the cpu-side stream endpoints
  Stream fromCpu, toCpu;
  uint32_t dmaOffset = 0; // this is our "dma buffer allocation" pointer...
  pthread_t readThread;
  struct timeval tv0, tv1, tv2;
  Bar bars[MAXBARS];
  unsigned nbars;
  const char *err;
  const char *name;
  char *cp = strrchr(argv[0], '/');
  name = cp ? cp + 1 : argv[0];

  if (argc == 1) {
    fprintf(stderr,
	    "Usage is: %s PCI-Device\n"
	    "  %s reads standard input and writes standard output\n"
	    "  routing the data through the specified FPGA board\n"
	    "Options are:\n"
	    "  -d        don't actually move, read or write any data\n"
	    "  -C<nbufs> set number of cpu-side buffers - default is %u\n"
	    "  -F<nbufs> set number of fpga-side buffers - default is %u\n"
	    "  -c        don't check opcodes for proper sequencing\n"
	    "  -m<nmsgs> number of messages to move (default is all of std input)\n"
	    "  -b<bsize> size of buffers (rounded up to 16 byte boundary)\n"
	    "  -v        verbose\n"
	    "  -s        single thread\n"
	    , name, name, nCpuBufs, nFpgaBufs);
    return 1;
  }
  while (argv[1][0] == '-') {
    switch (argv[1][1]) {
    case 'd':
      noData = 1;
      break;
    case 'C':
      nCpuBufs = atoi(&argv[1][2]);
      break;
    case 'F':
      nFpgaBufs = atoi(&argv[1][2]);
      break;
    case 'c':
      noCheck = 1;
      break;
    case 'm':
      maxFrames=atoi(&argv[1][2]);
      break;
    case 'b':
      bufSize=atoi(&argv[1][2]);
      bufSize+=15;
      bufSize &= ~15;
      break;
    case 'v':
      verbose = true;
      break;
    case 's':
      single = true;
      break;
    default:
      fprintf(stderr, "Base option flag: %s\n", argv[1]);
      return 1;
    }
    argv++;
  }
  if (geteuid()) {
    fprintf(stderr, "You must run this program with \"sudo -E\", as in \"sudo -E %s pci-dev\"\n",
	    name);
    return 1;
  }
  if (!(dmaEnv = getenv("OCPI_DMA_MEMORY"))) {
    fprintf(stderr, "You must set the OCPI_DMA_MEMORY variable before running this program\n");
    return 1;
  }
  if (sscanf(dmaEnv, "%uM$0x%llx", &dmaMeg, (unsigned long long *) &dmaBase) != 2) {
    fprintf(stderr, "The OCPI_DMA_MEMORY environment variable is not formatted correctly\n");
    return 1;
  }

  if ((err = getOpenCPI(argv[1], bars, &nbars, verbose)) || nbars != 2) {
    fprintf(stderr, "Couldn't get PCI information about PCI device %s.  Try ocfrp_check.\n", argv[1]);
    return 1;
  }
  fprintf(stderr, "BufSize=%d, CpuBufs %d FpgaBufs %d\n", bufSize, nCpuBufs, nFpgaBufs);
  errno = 0;
  assert ((fd = open("/dev/mem", O_RDWR|O_SYNC)) >= 0);
  assert((occp = (OccpSpace *)mmap(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE,
				   MAP_SHARED, fd, bars[0].address)) != MAP_FAILED);
  assert((bar1 = (uint8_t*)mmap(NULL, bars[1].size, PROT_READ|PROT_WRITE,
				MAP_SHARED, fd, bars[1].address)) != MAP_FAILED);
  assert(sscanf(dmaEnv, "%uM$0x%llx", &dmaMeg, (unsigned long long *) &dmaBase) == 2);
  assert((cpuBase = (uint8_t*)mmap(NULL, (unsigned long long)dmaMeg * 1024 * 1024,
				   PROT_READ|PROT_WRITE, MAP_SHARED, fd, dmaBase)) !=
	 MAP_FAILED);
  // Global setup is done, mappings established.  Now do individual pointers.
  // Pointers to the WCI property spaces for each worker
  dp0Props = (OcdpProperties*)occp->config[WORKER_DP0];
  dp1Props = (OcdpProperties*)occp->config[WORKER_DP1];
  sma0Props = (uint32_t*)occp->config[WORKER_SMA0];
  sma1Props = (uint32_t*)occp->config[WORKER_SMA1];
  biasProps = (uint32_t*)occp->config[WORKER_BIAS];
  // Pointers to the CP control space for each worker
  dp0 = &occp->worker[WORKER_DP0].control,
  dp1 = &occp->worker[WORKER_DP1].control,
  sma0 = &occp->worker[WORKER_SMA0].control,
  sma1 = &occp->worker[WORKER_SMA1].control,
  bias = &occp->worker[WORKER_BIAS].control;
  // Reset workers.  Should be order independent.
  reset(sma0,  0);
  reset(sma1,  0);
  reset(bias,  0);
  reset(dp0,  0);
  reset(dp1,  0);
  // initialize workers. Should be order independent.
  init(sma0);
  init(sma1);
  init(bias);
  init(dp0);
  init(dp1);
  // configure workers as appropriate
  *sma0Props = 1; // WMI input to WSI output
  *biasProps = 0; // leave data unchanged as it passes through
  *sma1Props = 2; // WSI input to WMI output
  // Configure streams, SW side and HW side
  setupStream(&fromCpu, dp0Props, false,
	      nCpuBufs, nFpgaBufs, bufSize, cpuBase, dmaBase, &dmaOffset);
  setupStream(&toCpu, dp1Props, true,
	      nCpuBufs, nFpgaBufs, bufSize, cpuBase, dmaBase, &dmaOffset);
  // start workers. Should be order independent
  start(dp0);
  start(dp1);
  start(sma0);
  start(bias);
  start(sma1);

  if (single) {
    unsigned nFrom = 1, nTo = 1;
    fprintf(stderr, "Single\n");
    gettimeofday(&tv0, 0);
    gettimeofday(&tv1, 0);
    do {
      if (fromCpu.flags[fromCpu.bufIdx])
	nFrom = checkStream(&fromCpu);
      if (toCpu.flags[toCpu.bufIdx])
	nTo = checkStream(&toCpu);
    } while (nTo != 0 && (maxFrames == 0 || toCpu.opCode < maxFrames));
  } else {
    // Now everything is running, and waiting to be fed some data
    // First we'll start a thread that reads data from FPGA and writes to stdout
    assert(pthread_create(&readThread, NULL, doStream, &toCpu) == 0);

  
    gettimeofday(&tv0, 0);
    gettimeofday(&tv1, 0);
    // Now well copy from stdin to the FPGA in the main thread
    doStream(&fromCpu);

    // Rendezvous with the background thread when it finishes reading from FPGA to stdout
    assert(pthread_join(readThread, NULL) == 0);
  }
  gettimeofday(&tv2, 0);
  {
    uint64_t t0 = tv0.tv_sec * 1000000LL + tv0.tv_usec;
    uint64_t t1 = tv1.tv_sec * 1000000LL + tv1.tv_usec;
    uint64_t t2 = tv2.tv_sec * 1000000LL + tv2.tv_usec;
    fprintf(stderr, "Bytes %lld, Time delta = %lld, %f MBytes/seconds, Framesize %d\n",
	    (long long)bytes,(long long)((t2 - t1) - (t1 - t0)),
	    (double)bytes/(double)((t2-t1)-(t1-t0)), bufSize);
  }
  return 0;
}

// reset a worker
 static void
reset(volatile OccpWorkerRegisters *w, unsigned timeout) {
   // compute log-2 timeout value
   uint32_t logTimeout = 31, u;
   if (!timeout)
     timeout = 16;
   for (u = 1 << logTimeout; !(u & timeout); u >>= 1, logTimeout--)
     ;
   // Assert Reset
   w->control =  logTimeout;
   // Take out of reset
   w->control = OCCP_CONTROL_ENABLE | logTimeout ;
}

// check a control operation return code
 static void
check(uint32_t val) {
  assert(val == OCCP_SUCCESS_RESULT);
}

// initialize a worker
 static void
init(volatile OccpWorkerRegisters *w) {
  check(w->initialize);
}

// start a worker
 static void
start(volatile OccpWorkerRegisters *w) {
  check(w->start);
}

static unsigned
checkStream(Stream *s) {
  unsigned n;
  // mark the buffer not ready for CPU.  FPGA will set it to 1 when it is ready
  s->flags[s->bufIdx] = 0;
  // Wait for buffer to be read, so we can fill/empty it.
  if (s->isToCpu) {
    if (noData)
      n = s->bufSize;
    else {
      if ((n = s->metadata[s->bufIdx].length)) {
	memcpy(s->buf, (void*)&s->buffers[s->bufIdx * s->bufSize], n);
	assert(write(1, s->buf, n) == n);
      }
    }
    if (!noCheck) {
      assert((s->metadata[s->bufIdx].opCode & 0xff) == (s->opCode & 0xff));
      bytes += s->bufSize;
    } else
      bytes += s->metadata[s->bufIdx].length;
  } else {
    if (noData) {
      n = s->bufSize;
      s->metadata[s->bufIdx].length = n;
    } else {
      n = read(0, s->buf, s->bufSize);
      assert(n >= 0);
      if (n)
	memcpy((void *)&s->buffers[s->bufIdx * s->bufSize], s->buf, n);
      s->metadata[s->bufIdx].length = n;// & ~3; // bitstream doesn't preserve bytes in words.
    }
    if (!noCheck)
      s->metadata[s->bufIdx].opCode = s->opCode;
  }
  s->opCode++;

  if (verbose)
    fprintf(stderr, "%s cpu done %d %d\n", s->isToCpu ? "to" : "from",
	    s->opCode, s->bufIdx);

  // Tell hardware we have processed the buffer (filled it or emptied it).
  *s->doorbell = 1;
  if (++s->bufIdx == s->nBufs)
    s->bufIdx = 0;
  return n;
}


// function to run in a thread to read data from the FPGA and write it to stdout
 static void *
doStream(void *args) {
  Stream *s = (Stream *)args;
  int n;
  do {
    if (verbose)
      fprintf(stderr, "%s cpu %d\n", s->isToCpu ? "to" : "from", s->opCode);
    while (s->flags[s->bufIdx] == 0)
      sched_yield();
    n = checkStream(s);
  } while (n != 0 && (maxFrames == 0 || s->opCode < maxFrames));// On a big end);
  return 0;
}

 static void
setupStream(Stream *s, volatile OcdpProperties *p, bool isToCpu,
	    unsigned nCpuBufs, unsigned nFpgaBufs, unsigned bufSize,
	    uint8_t *cpuBase, unsigned long long dmaBase, uint32_t *offset)
{
  s->bufIdx = 0;
  s->opCode = 0;
  s->buf = malloc(bufSize); // because linux can't read/write to/from dma memory
  s->isToCpu = isToCpu;
  s->nBufs = nCpuBufs;
  s->bufSize = bufSize;
  s->buffers = cpuBase + *offset;
  s->metadata = (OcdpMetadata *)(s->buffers + nCpuBufs * bufSize);
  s->flags = (uint32_t *)(s->metadata + nCpuBufs);
  s->doorbell = &p->nRemoteDone;
  *offset += (uint8_t *)(s->flags + nCpuBufs) - s->buffers;
  memset((void *)s->flags, isToCpu ? 0 : 1, nCpuBufs * sizeof(uint32_t));
  p->nLocalBuffers = nFpgaBufs;
  p->nRemoteBuffers = nCpuBufs;
  p->localBufferBase = 0;
  p->localMetadataBase = nFpgaBufs * bufSize; // above 4 x 2k buffers
  p->localBufferSize = bufSize;
  p->localMetadataSize = sizeof(OcdpMetadata);
  p->memoryBytes = 32*1024;
  p->remoteBufferBase = dmaBase + (s->buffers - cpuBase);
  p->remoteMetadataBase = dmaBase + ((uint8_t*)s->metadata - cpuBase);
  p->remoteBufferSize = bufSize;
  p->remoteMetadataSize = sizeof(OcdpMetadata);
  p->remoteFlagBase = dmaBase + ((uint8_t*)s->flags - cpuBase);
  p->remoteFlagPitch = sizeof(uint32_t);
  p->control = OCDP_CONTROL(isToCpu ? OCDP_CONTROL_PRODUCER : OCDP_CONTROL_CONSUMER,
			    OCDP_ACTIVE_MESSAGE);
  if (verbose) {
    fprintf(stderr, "%s cpu stream: d %llx m %llx f %llx\n",
	    s->isToCpu ? "to" : "from",
	    (unsigned long long)p->remoteBufferBase,
	    (unsigned long long)p->remoteMetadataBase,
	    (unsigned long long)p->remoteFlagBase);
  }
 }
