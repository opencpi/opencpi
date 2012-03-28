#define _GNU_SOURCE
#include <time.h>
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
#include "HdlOCCP.h"
#include "HdlOCDP.h"
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
  unsigned bufSize, nBufs, ramp;
  uint64_t rampValue;
  volatile uint8_t *buffers;
  volatile OcdpMetadata *metadata;
  volatile uint32_t *flags;
  volatile uint32_t *doorbell;
  volatile uint64_t *startTime, *doneTime;
  // Then the dynamic state of the stream
  unsigned bufIdx, opCode;
  char *buf; // local buffer
  char *cbuf; // local compare buffer
} Stream;

// Forward declarations
static void
  reset(volatile OccpWorkerRegisters *, unsigned),
  init(volatile OccpWorkerRegisters *),
  start(volatile OccpWorkerRegisters *),
  *doStream(void *args),
  setupStream(Stream *s, volatile OcdpProperties *p, bool isToCpu,
	      unsigned nCpuBufs, unsigned nFpgaBufs, unsigned bufSize,
	      uint8_t *cpuBase, unsigned long long dmaBase, uint32_t *offset, unsigned ramp);

typedef unsigned long long ull; 
static inline ull ticks2ns(uint64_t ticks) {
  return (ticks * 1000000000ull + (1ull << 31))/ (1ull << 32);
}
static inline ull ns2ticks(uint32_t sec, uint32_t nsec) {
  return ((uint64_t)sec << 32ull) + (nsec + 500000000ull) * (1ull<<32) /1000000000;
}

static inline uint64_t now() {
#ifdef __APPLE__
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ns2ticks(tv.tv_sec, tv.tv_usec * 1000);
#else
  struct timespec ts;
  clock_gettime (CLOCK_REALTIME, &ts);
  return ns2ticks(ts.tv_sec, ts.tv_nsec);
#endif
}


static inline uint64_t swap32(uint64_t x) {return (x <<32) | (x >> 32); }

static int compu32(const void *a, const void *b) { return *(int32_t*)a - *(int32_t*)b; }
void init_fpga_time(volatile OccpSpace *occp) {
  unsigned n;
  uint32_t delta[100];
  uint32_t sum = 0;
  
  for (n = 0; n < 100; n++) {
    occp->admin.timeDelta = occp->admin.time;
    delta[n] = occp->admin.timeDelta >> 32;
  }
  qsort(delta, 100, sizeof(uint32_t), compu32);
  
  for (n = 0; n < 90; n++)
    sum += delta[n];
  sum = ((sum + 45) / 90) / 2;
  // we have average delay
  fprintf(stderr, "delta ticks min %llu max %llu avg %llu\n",
	  ticks2ns(delta[0]), ticks2ns(delta[99]), ticks2ns(sum));
  occp->admin.time = swap32(now() + sum);
  occp->admin.timeDelta = swap32(now());
  fprintf(stderr, "now delta is: %lluns\n", ticks2ns(occp->admin.timeDelta >> 32));
  #ifndef __APPLE__
  {
    struct timespec ts;
    clock_getres(CLOCK_REALTIME, &ts);
    fprintf(stderr, "res: %ld\n", ts.tv_nsec);
  }
  #endif
}
static unsigned checkStream(Stream *s, uint64_t *tp, Stream *other);
bool noData = 0, noCheck = 0, verbose = 0, single = 0, measure = 0;
unsigned maxFrames = 0, bufSize = 4096;
uint64_t bytes;
 int
main(int argc, char *argv[])
{
  unsigned dmaMeg;
  unsigned long long dmaBase;
  int fd;
  volatile OccpSpace *occp;
  uint8_t *bar1, *cpuBase;
  const char *dmaEnv;
  volatile OcdpProperties *dp0Props, *dp1Props;
  volatile uint32_t *sma0Props, *sma1Props, *biasProps;
  volatile OccpWorkerRegisters *dp0, *dp1, *sma0, *sma1, *bias;
  unsigned nFpgaBufs = 2, nCpuBufs = 200, ramp = 0;
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
	    "  -r<size>  ramp the data.  <size> is size of ramp value (1/2/4/8).\n"
	    "  -t        when single, perform time measurements for each round trip.\n"
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
    case 't':
      measure = true;
      break;
    case 'r':
      ramp = atoi(&argv[1][2]);
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
  fprintf(stderr, "BufSize=%d, CpuBufs %d FpgaBufs %d Ramp %d\n", bufSize, nCpuBufs, nFpgaBufs, ramp);
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
  init_fpga_time(occp);
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
	      nCpuBufs, nFpgaBufs, bufSize, cpuBase, dmaBase, &dmaOffset, ramp);
  setupStream(&toCpu, dp1Props, true,
	      nCpuBufs, nFpgaBufs, bufSize, cpuBase, dmaBase, &dmaOffset, ramp);
  // start workers. Should be order independent
  start(dp0);
  start(dp1);
  start(sma0);
  start(bias);
  start(sma1);

  if (single) {
    unsigned nTo = 1;
    fprintf(stderr, "Running single threaded\n");
    gettimeofday(&tv0, 0);
    gettimeofday(&tv1, 0);
    if (measure) {
      fprintf(stderr, "Nanoseconds:   Size        Pull       Push      Total    Processing\n");
      do {
	uint32_t tcons, n, tprod;
	uint64_t t[4];
	
	for (n = 1000000000; n && !(tcons = fromCpu.flags[fromCpu.bufIdx]); n--) {
	}
	if (!n) {
	  fprintf(stderr, "Timed out waiting for buffer from cpu to fpga\n");
	  return 1;
	}
	checkStream(&fromCpu, NULL, NULL);
	for (n = 1000000000; n && !(tprod = toCpu.flags[toCpu.bufIdx]); n--) {
	}
	if (!n) {
	  fprintf(stderr, "Timed out waiting for buffer from fpga to cpu\n");
	  return 1;
	}
	nTo = checkStream(&toCpu, t, &fromCpu);
#if 0
 	t[0] = ((t[0] & 0xffffffff) << 32) | (t[0] >> 32);
	t[1] = ((t[1] & 0xffffffff) << 32) | (t[1] >> 32);
	t[2] = ((t[2] & 0xffffffff) << 32) | (t[2] >> 32);
	t[3] = ((t[3] & 0xffffffff) << 32) | (t[3] >> 32);
	fprintf(stderr, "Times: cons reg %llx prod reg %llx cons db %llx prod db %llx\n",
		(ull)t[0], (ull)t[1], (ull)t[2], (ull)t[3]);
#endif
	fprintf(stderr,
		"Measure: %10u  %10llu %10llu %10llu %10llu\n",
		bufSize,
		ticks2ns(t[2] - t[0]), ticks2ns(t[3] - t[1]), ticks2ns(t[3] - t[0]),
		ticks2ns(t[1] - t[2]));
      } while (nTo != 0 && (maxFrames == 0 || toCpu.opCode < maxFrames));
    } else
      do {
	if (fromCpu.flags[fromCpu.bufIdx])
	  checkStream(&fromCpu, NULL, NULL);
	if (toCpu.flags[toCpu.bufIdx])
	  nTo = checkStream(&toCpu, NULL, NULL);
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
    fprintf(stderr, "Bytes %lld, Time delta = %lld, %f MBytes/seconds, Message size %d\n",
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
checkStream(Stream *s, uint64_t *tp, Stream *other) {
  unsigned n;
  // mark the buffer not ready for CPU.  FPGA will set it to 1 when it is ready
  s->flags[s->bufIdx] = 0;
  // Wait for buffer to be read, so we can fill/empty it.
  if (s->isToCpu) {
    // We have received a buffer from the FPGA
    if (tp) {
      tp[0] = *other->startTime; // capture consumer's "issue pull of metadata" time
      tp[1] = *s->startTime; // capture producers "issue doorbell" time.
      tp[2] = *other->doneTime;
      tp[3] = *s->doneTime;
    }
    if (noData)
      n = s->bufSize;
    else {
      if ((n = s->metadata[s->bufIdx].length)) {
	memcpy(s->buf, (void*)&s->buffers[s->bufIdx * s->bufSize], n);
	assert(write(1, s->buf, n) == n);
	if (s->ramp) {
	  for (unsigned nn = 0; nn < (s->bufSize / s->ramp); nn++) {
	    uint64_t value = s->rampValue++;
	    switch (s->ramp) {
	    case 1: s->cbuf[nn] = value; break;
	    case 2: ((uint16_t *)s->cbuf)[nn] = value; break;
	    case 4: ((uint32_t *)s->cbuf)[nn] = value; break;
	    case 8: ((uint64_t *)s->cbuf)[nn] = value; break;
	    default:
	      ;
	    }
	  }
	  if (memcmp(s->buf, s->cbuf, n))
	    fprintf(stderr, "Ramp mismatch: message %d, starts at %d offset in file. %d\n",
		    s->opCode, s->opCode * s->bufSize, n);
	}
      }
    }
    if (!noCheck) {
      assert((s->metadata[s->bufIdx].opCode & 0xff) == (s->opCode & 0xff));
      bytes += s->bufSize;
    } else
      bytes += s->metadata[s->bufIdx].length;
  } else { // from cpu
    if (noData) {
      n = s->bufSize;
      s->metadata[s->bufIdx].length = n;
    } else {
      if (s->ramp) {
	for (unsigned nn = 0; nn < (s->bufSize / s->ramp); nn++) {
	  uint64_t value = s->rampValue++;
	  switch (s->ramp) {
	  case 1: s->buf[nn] = value; break;
	  case 2: ((uint16_t *)s->buf)[nn] = value; break;
	  case 4: ((uint32_t *)s->buf)[nn] = value; break;
	  case 8: ((uint64_t *)s->buf)[nn] = value; break;
	  default:
	    ;
	  }
	}
	n = s->bufSize;
      } else {
	n = read(0, s->buf, s->bufSize);
	assert(n >= 0);
      }
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
    n = checkStream(s, NULL, NULL);
  } while (n != 0 && (maxFrames == 0 || s->opCode < maxFrames));// On a big end);
  return 0;
}

 static void
setupStream(Stream *s, volatile OcdpProperties *p, bool isToCpu,
	    unsigned nCpuBufs, unsigned nFpgaBufs, unsigned bufSize,
	    uint8_t *cpuBase, unsigned long long dmaBase, uint32_t *offset,
	    unsigned ramp)
{
  uint64_t addr;
  s->rampValue = 0;
  s->ramp = ramp;
  s->bufIdx = 0;
  s->opCode = 0;
  s->buf = malloc(bufSize); // because linux can't read/write to/from dma memory
  s->cbuf = malloc(bufSize);
  s->isToCpu = isToCpu;
  s->nBufs = nCpuBufs;
  s->bufSize = bufSize;
  s->buffers = cpuBase + *offset;
  s->metadata = (OcdpMetadata *)(s->buffers + nCpuBufs * bufSize);
  s->flags = (uint32_t *)(s->metadata + nCpuBufs);
  s->doorbell = &p->nRemoteDone;
  s->startTime = &p->startTime;
  s->doneTime = &p->doneTime;
  *offset += (uint8_t *)(s->flags + nCpuBufs) - s->buffers;
  memset((void *)s->flags, isToCpu ? 0 : 1, nCpuBufs * sizeof(uint32_t));
  p->nLocalBuffers = nFpgaBufs;
  p->nRemoteBuffers = nCpuBufs;
  p->localBufferBase = 0;
  p->localMetadataBase = nFpgaBufs * bufSize; // above 4 x 2k buffers
  p->localBufferSize = bufSize;
  p->localMetadataSize = sizeof(OcdpMetadata);
  p->memoryBytes = 32*1024;
  addr = dmaBase + (s->buffers - cpuBase);
  p->remoteBufferBase = addr;
  p->remoteBufferHi = addr >> 32;
  addr = dmaBase + ((uint8_t*)s->metadata - cpuBase);
  p->remoteMetadataBase = addr;
  p->remoteMetadataHi = addr >> 32;
  addr = dmaBase + ((uint8_t*)s->flags - cpuBase);
  p->remoteFlagBase = addr;
  p->remoteFlagHi = addr >> 32;
  p->remoteBufferSize = bufSize;
  p->remoteMetadataSize = sizeof(OcdpMetadata);
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
