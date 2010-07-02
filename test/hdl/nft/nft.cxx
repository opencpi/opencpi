#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "OCCP.h"
#include "OCDP.h"

#define WORKER_DP0 (13)
#define WORKER_DP1 (14)
#define WORKER_SMA0 (2)
#define WORKER_BIAS (3)
#define WORKER_SMA1 (4)
#define OCDP_OFFSET_DP0 (32*1024)
#define OCDP_OFFSET_DP1 0

// Control operations on workers
static void
  reset(volatile CPI::RPL::OccpWorkerRegisters *, unsigned),
  init(volatile CPI::RPL::OccpWorkerRegisters *),
  start(volatile CPI::RPL::OccpWorkerRegisters *);

// Packet of arguments required to process a stream endpoint on the cpu side
struct Stream {
  unsigned bufSize, nBufs;
  volatile uint8_t *buffers;
  volatile CPI::RPL::OcdpMetadata *metadata;
  volatile uint32_t *flags;
  volatile uint32_t *doorbell;
};

static void
  *doRead(void *args),
  *doWrite(void *args),
  setupStream(Stream *s, volatile CPI::RPL::OcdpProperties *p, bool isToCpu,
	      unsigned nCpuBufs, unsigned nFpgaBufs, unsigned bufSize,
	      uint8_t *cpuBase, unsigned long long dmaBase, uint32_t *offset);

int main(int argc, char *argv[])
{
  errno = 0;
  unsigned dmaMeg;
  unsigned long long
    dmaBase, dmaSize,
    bar0Base = strtoull(argv[1], NULL, 0),
    bar1Base = strtoull(argv[2], NULL, 0),
    bar1Size = strtoull(argv[3], NULL, 0);
  assert(errno == 0);
  int fd = open("/dev/mem", O_RDWR|O_SYNC);
  assert(fd >= 0);

  CPI::RPL::OccpSpace *occp =
    (CPI::RPL::OccpSpace *)mmap(NULL, sizeof(CPI::RPL::OccpSpace),
				PROT_READ|PROT_WRITE, MAP_SHARED, fd, bar0Base);
  assert(occp != (CPI::RPL::OccpSpace*)-1);
  uint8_t *bar1 = (uint8_t*)mmap(NULL, bar1Size,
				 PROT_READ|PROT_WRITE, MAP_SHARED, fd, bar1Base);
  assert(bar1 != (uint8_t*)-1);
  const char *dmaEnv = getenv("CPI_DMA_MEMORY");
  assert(dmaEnv);
  unsigned count = sscanf(dmaEnv, "%uM$0x%llx", &dmaMeg,
			  (unsigned long long *) &dmaBase);
  assert(count == 2);
  dmaSize = (unsigned long long)dmaMeg * 1024 * 1024;
  uint8_t *cpuBase =
    (uint8_t*)mmap(NULL, dmaSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, dmaBase);
  assert(cpuBase != (uint8_t*)-1);
  // These have this structure of properties
  volatile CPI::RPL::OcdpProperties
    *dp0Props = (CPI::RPL::OcdpProperties*)occp->config[WORKER_DP0],
    *dp1Props = (CPI::RPL::OcdpProperties*)occp->config[WORKER_DP1];

  // These just have a single 32 bit scalar property
  volatile uint32_t
    *sma0Props = (uint32_t*)occp->config[WORKER_SMA0],
    *sma1Props = (uint32_t*)occp->config[WORKER_SMA1],
    *biasProps = (uint32_t*)occp->config[WORKER_BIAS];
  volatile CPI::RPL::OccpWorkerRegisters
    *dp0 = &occp->worker[WORKER_DP0].control,
    *dp1 = &occp->worker[WORKER_DP1].control,
    *sma0 = &occp->worker[WORKER_SMA0].control,
    *sma1 = &occp->worker[WORKER_SMA1].control,
    *bias = &occp->worker[WORKER_BIAS].control;

  // So far we have not done anything other than establish mappings and pointers

  // Reset workers
  reset(sma0,  0);
  reset(sma1,  0);
  reset(bias,  0);
  reset(dp0,  0);
  reset(dp1,  0);

  // initialize workers
  init(sma0);
  init(sma1);
  init(bias);
  init(dp0);
  init(dp1);

  // configure workers as appropriate
  *sma0Props = 1; // WMI input to WSI output
  *biasProps = 0;
  *sma1Props = 2; // WSI input to WMI output

  // Configure streams, SW side and HW side

  const unsigned bufSize = 2048, nFpgaBufs = 4, nCpuBufs = 8;
  // These structures define the cpu-side stream endpoints
  Stream fromCpu, toCpu;
  uint32_t dmaOffset = 0; // this is our "dma buffer allocation" pointer...
  setupStream(&fromCpu, dp0Props, false,
	      nCpuBufs, nFpgaBufs, bufSize, cpuBase, dmaBase, &dmaOffset);
  setupStream(&toCpu, dp1Props, true,
	      nCpuBufs, nFpgaBufs, bufSize, cpuBase, dmaBase, &dmaOffset);

  // start workers
  start(dp0);
  start(dp1);
  start(sma0);
  start(bias);
  start(sma1);

  // Now everything is running, and waiting to be fed some data
  // First we'll start a thread that reads data
  pthread_t readThread;
  int r = pthread_create(&readThread, NULL, doRead, &toCpu);
  assert(r == 0);

  // Now well copy from stdin to the FPGA in the main thread
  doWrite(&fromCpu);

  // Rendezvous with the background thread when it finishes reading from FPGA to stdout
  r = pthread_join(readThread, NULL);
  assert(r == 0);

  return 0;
}

// reset a worker
 static void
reset(volatile CPI::RPL::OccpWorkerRegisters *w, unsigned timeout) {
   // compute log-2 timeout value
   if (!timeout)
     timeout = 16;
   unsigned logTimeout = 31;
   for (uint32_t u = 1 << logTimeout; !(u & timeout);
	u >>= 1, logTimeout--)
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
init(volatile CPI::RPL::OccpWorkerRegisters *w) {
  check(w->initialize);
}

// start a worker
 static void
start(volatile CPI::RPL::OccpWorkerRegisters *w) {
  check(w->start);
}

// function to run in a thread to read data from the FPGA and write it to stdout
 static void *
doRead(void *args) {
  Stream *s = (Stream *)args;
  unsigned bufIdx = 0, opCode = 0, nwrite;
  void *buf = malloc(s->bufSize); // just here because linux can't read/write to/from dma memory
  do {
    // Wait for buffer to be full, so we can empty it.
    while (s->flags[bufIdx] == 0)
      ;
    nwrite = s->metadata[bufIdx].length;
    if (nwrite != 0) {
      memcpy(buf, (void*)&s->buffers[bufIdx * s->bufSize], nwrite);
      int n = write(1, buf, nwrite);
      assert(n > 0 && (unsigned)n == nwrite);
    }
    assert(s->metadata[bufIdx].opCode == (opCode & 0xff));
    opCode++;
    // mark the buffer empty.  FPGA will set it to 1 when it fills it (ready to use)
    s->flags[bufIdx] = 0;
    // Tell hardware we have emptied it.
    *s->doorbell = 1;
    if (++bufIdx == s->nBufs)
      bufIdx = 0;
  } while (nwrite != 0);
  return 0;
}

// function to run in a thread to write data from stdin to the FPGA
 static void *
doWrite(void *args) {
  Stream *s = (Stream *)args;
  unsigned bufIdx = 0, opCode = 0;
  int nread;
  void *buf = malloc(s->bufSize);
  do {
    // Wait for buffer to be empty, so we can fill it.
    while (s->flags[bufIdx] == 0)
      ;
    nread = read(0, buf, s->bufSize);
    assert(nread >= 0);
    if (nread) {
      nread &= ~3; // for now bitstream doesn't preserve bytes in words.
      memcpy((void *)&s->buffers[bufIdx * s->bufSize], buf, nread);
    }
    s->metadata[bufIdx].length = nread;
    s->metadata[bufIdx].opCode = opCode++;
    // Set it full. FPGA will set it to 1 (empty/ready to use)
    s->flags[bufIdx] = 0;
    // Tell hardware we have filled it.
    *s->doorbell = 1;
    if (++bufIdx == s->nBufs)
      bufIdx = 0;
  } while (nread != 0);
  return 0;
 }

 static void
setupStream(Stream *s, volatile CPI::RPL::OcdpProperties *p, bool isToCpu,
	    unsigned nCpuBufs, unsigned nFpgaBufs, unsigned bufSize,
	    uint8_t *cpuBase, unsigned long long dmaBase, uint32_t *offset)
{
  s->nBufs = nCpuBufs;
  s->bufSize = bufSize;
  s->buffers = cpuBase + *offset;
  s->metadata = (CPI::RPL::OcdpMetadata *)(s->buffers + nCpuBufs * bufSize);
  s->flags = (uint32_t *)(s->metadata + nCpuBufs);
  s->doorbell = &p->nRemoteDone;
  *offset += (uint8_t *)(s->flags + nCpuBufs) - s->buffers;
  memset((void *)s->flags, isToCpu ? 0 : 1, bufSize * sizeof(uint32_t));
  p->nLocalBuffers = nFpgaBufs;
  p->nRemoteBuffers = nCpuBufs;
  p->localBufferBase = 0;
  p->localMetadataBase = nFpgaBufs * bufSize; // above 4 x 2k buffers
  p->localBufferSize = bufSize;
  p->localMetadataSize = sizeof(CPI::RPL::OcdpMetadata);
  p->memoryBytes = 32*1024;
  p->remoteBufferBase = dmaBase + (s->buffers - cpuBase);
  p->remoteMetadataBase = dmaBase + ((uint8_t*)s->metadata - cpuBase);
  p->remoteBufferSize = bufSize;
  p->remoteMetadataSize = sizeof(CPI::RPL::OcdpMetadata);
  p->remoteFlagBase = dmaBase + ((uint8_t*)s->flags - cpuBase);
  p->remoteFlagPitch = sizeof(uint32_t);
  p->control = OCDP_CONTROL(isToCpu ? OCDP_CONTROL_PRODUCER : OCDP_CONTROL_CONSUMER,
			    CPI::RPL::OCDP_ACTIVE_MESSAGE);
 }
