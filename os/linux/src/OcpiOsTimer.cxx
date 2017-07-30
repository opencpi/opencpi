/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef OCPI_OS_linux
  #include <sched.h>
  #ifdef OCPI_OS_VERSION_r5
    #ifndef __x86_64__
      #error No support for RHEL5 on non-64-bit machines
    #endif
    #include <asm/vsyscall.h>
  #endif
#else
  #include <sys/time.h> // for gettimeofday
  typedef uint64_t cpu_set_t;
#endif

#include "OcpiOsTimer.h"
#include "OcpiOsSizeCheck.h"
#include "OcpiOsAssert.h"

#ifndef OCPI_CLOCK_TYPE
  #ifdef CLOCK_MONOTONIC_RAW
    #define OCPI_CLOCK_TYPE CLOCK_MONOTONIC_RAW
  #else
    #define OCPI_CLOCK_TYPE CLOCK_MONOTONIC
  #endif
#endif
namespace OCPI {
  namespace OS {

Time Time::now() {
#ifdef __APPLE__
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return Time((uint32_t)tv.tv_sec, tv.tv_usec * 1000);
#else
  struct timespec ts;
  ocpiCheck(clock_gettime (OCPI_CLOCK_TYPE, &ts) == 0);
  return Time((uint32_t)ts.tv_sec, (uint32_t)ts.tv_nsec);
#endif
}
/*
 * ----------------------------------------------------------------------
 * Frequency of the high-resolution timestamp counter, in Hz.
 *
 * On Mercury Linux/PPC, this is initialized from the "timebase" value
 * in /proc/cpuinfo (e.g., "timebase period : 24000 psecs", indicating
 * a frequency of 41.6 MHz).
 *
 * On Linux/x86, this is initialized from the "CPU MHz" value in
 * /proc/cpuinfo (e.g., cpu MHz : 2593.753).  Documentation indicates
 * that the tick count becomes skewed if the CPU frequency changes.
 * Wikipedia adds, "recent Intel processors include a constant rate TSC
 * (identified by the constant_tsc flag in Linux's /proc/cpuinfo).  With
 * these processors the TSC reads at the processors maximum rate
 * regardless of the actual CPU running rate."
 *
 * If neither value is found, this value is set to 0, and the Timer
 * implementation falls back to POSIX clock_gettime().
 *
 * We only look for the first occurence of either string.  This assumes
 * that, if there is more than one processor, the timer frequency is
 * identical for all.
 * ----------------------------------------------------------------------
 */
#define cpuid(func,ax,bx,cx,dx)\
        __asm__ __volatile__ ("cpuid":\
        "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));

namespace {
  class CounterFreq {
  public:
    CounterFreq ();
    bool useHighResTimer () const;
    operator uint64_t () const;

  private:
    bool m_useHighResTimer;
    uint64_t m_counterFreq;
  };

  static CounterFreq g_counterFreq;

  inline
  bool
  useHighResTimer ()
  {
    return g_counterFreq.useHighResTimer ();
  }
}

CounterFreq::CounterFreq ()
  : m_useHighResTimer(false), m_counterFreq(0)
{
  int fd = open ("/proc/cpuinfo", O_RDONLY);

  if (fd < 0) {
    // Looks like /proc/cpuinfo doesn't exist
    return;
  }
  char tmp[2048]; // Enough to read first core's info
  memset(tmp, 0, 2048);
  ssize_t nread = read (fd, tmp, 2047);
  close (fd);

  std::string valueread(tmp);

  //std::cout << valueread << std::endl;


  if (nread < 0) {
    return;
  }
  tmp[nread] = 0;

  /*
   * We always trust a value called "timebase period".
   */

  char * ptr;

  if ((ptr = std::strstr (tmp, "timebase period"))) {
    unsigned long period;
    char * endPtr;

    // Position ptr at the beginning of the value
    ptr += 15;
    while (*ptr == ' ' || *ptr == '\t' || *ptr == ':') {
      ptr++;
    }

    period = strtoul (ptr, &endPtr, 10);

    if (*endPtr == ' ') {
      endPtr++;
    }

    if (period != 0 && strncmp (endPtr, "psecs", 5) == 0) {
      m_useHighResTimer = true;
      m_counterFreq = 1000000000000ull / static_cast<unsigned long long> (period);
      return;
    }
    else if (period != 0 && strncmp (endPtr, "nsecs", 5) == 0) {
      m_useHighResTimer = true;
      m_counterFreq = 1000000000ull / static_cast<unsigned long long> (period);
      return;
    }

    /*
     * All other (unexpected) values for timebase period: fall through.
     */
  }

  /*
   * Sometimes (MPC-102) there is a "timebase", which is the inverse.
   */

  if ((ptr = std::strstr (tmp, "timebase"))) {
    unsigned long freq;
    char * endPtr;

    // Position ptr at the beginning of the value
    ptr += 8;
    while (*ptr == ' ' || *ptr == '\t' || *ptr == ':') {
      ptr++;
    }

    freq = strtoul (ptr, &endPtr, 10);

    if (*endPtr == ' ') {
      endPtr++;
    }

    if (freq != 0 && *endPtr == '\n') {
      m_useHighResTimer = true;
      m_counterFreq = freq;
      return;
    }

    /*
     * All other (unexpected) values for timebase: fall through.
     */
  }

  /*
   * We only use the "cpu MHz" value on x86.
   */

#if defined (__x86_64__) || defined (__i386__)

  //parse cpuid for tsc_invariant
  int ax,bx,cx,dx;
  cpuid(0x80000007,ax,bx,cx,dx)
  //if tsc_invariant not present return - present in edx 0x80000007 bit 8
  if (!(dx&0x8)) {
    //    ocpiBad("OCPI::OS::Time subsystem cannot establish clock frequency");
    return;
  }
  const char *cp = strcasestr(tmp, "cpu MHz");
  if (cp) {
    cp += 7;
    while (isspace(*cp) || *cp == ':')
      cp++;
    m_useHighResTimer = true;
    m_counterFreq = (uint64_t)(atof(cp)*1e6);
    return;
  }

  size_t pointer;
  // TODO: changed to find model name and TSC invariant clock speed extracted
  if ((pointer = valueread.find("model name",0))) {

    // Position ptr at the beginning of the value
    size_t start = valueread.find("@",pointer);

    //go past the @ and the space
    start+=2;
    size_t end   = valueread.find("GHz",pointer);
    float multi = 1e9;
    if (end == std::string::npos){
      end = valueread.find("MHz",pointer);
      multi = 1e6;
      if (end == std::string::npos)
        return;
    }

    //find the substring that holds the speed bound by the @ sign and either
    //GHz or MHz
    std::string substring(valueread,start,end-(start));

    //convert the string to a float value
    float speed=(float) std::atof(substring.c_str());

    //this is enables the high resolution timer
    m_useHighResTimer = true;
    //this sets the tsc_invariant clock speed
    m_counterFreq = (uint64_t)(speed*multi);
    return;
  }

#endif


  /* this is only valid for the x86 cpu
   * Did not find anything useful in /proc/cpuinfo.  Tell the Timer
   * implementation to fall back to clock_gettime().
   */

  m_useHighResTimer = false;
}

inline
bool
CounterFreq::useHighResTimer () const
{
  return m_useHighResTimer;
}

inline
CounterFreq::operator uint64_t () const
{
  return m_counterFreq;
}

/*
 * ----------------------------------------------------------------------
 * Read timer register.
 * ----------------------------------------------------------------------
 */

#if defined ( __GNUC__ ) && defined ( _ARCH_PPC )
  #define TBU( t ) __asm volatile ( "mfspr %0,269" : "=r" ( t ) )
  #define TBL( t ) __asm volatile ( "mfspr %0,268" : "=r" ( t ) )

#elif defined ( __x86_64__ ) || defined ( __i386__ )

  // unused in x86, but needed for loop constructs
  #define TBU( t ) tb_upper_tmp = t;
  // See Intel's "ia-32-ia-64-benchmark-code-execution-paper.pdf"
  // How to Benchmark Code Execution Timers on Intel IA-32 and IA-64 Instruction Set
  // Architectures White Paper
  // or http://stackoverflow.com/a/14214220
  #define TBL( t ) __asm__ __volatile__ ("CPUID\n\t" \
                                         "RDTSC\n\t" \
                                         "mov %%edx, %0\n\t" \
                                         "mov %%eax, %1\n\t" \
                                         : "=r"(tb_upper), "=r"(tb_lower) \
                                         : \
                                         : "%rax","%rbx","%rcx","%rdx");

  #define eTBL( lo, hi, cpu ) __asm__ __volatile__ ("RDTSCP\n\t" \
                                                    "mov %%edx, %0\n\t" \
                                                    "mov %%eax, %1\n\t" \
                                                    "mov %%ecx, %2\n\t" \
                                                    "CPUID\n\t" \
                                                    : "=r"(hi), "=r"(lo), "=r"(cpu) \
                                                    : \
                                                    : "%rax","%rbx","%rcx","%rdx");

#elif defined (__vxWorks__) // vxWorks Diab and Green Hills inline assembler format
  #define TBU( t ) tm_move_from_tbr_upper ( t )
  #define TBL( t ) tm_move_from_tbr_lower ( t )

void tm_move_from_tbr_upper ( unsigned int time_val );
void tm_move_from_tbr_lower ( unsigned int time_val );

asm void tm_move_from_tbr_upper ( unsigned int time_val )
{
%reg time_val // Must be in the first column
  mfspr time_val,269;
}

asm void tm_move_from_tbr_lower ( unsigned int time_val )
{
%reg time_val // Must be in the first column
  mfspr time_val,268;
}
#else

#warning Unknown platform for build. Cannot insert assembly code for timer. Various OS::Timer functions will abort if called.
#define TBU( t ) abort()
#define TBL( t ) abort()

#endif

/*
 * ----------------------------------------------------------------------
 * Timer implementation.
 * ----------------------------------------------------------------------
 */

namespace {
  struct TimerClockInfo {
    Time startTime;
    ElapsedTime accumulatedTime;
  };

  struct TimerTickInfo {
    uint32_t lower, upper;
    uint16_t accumulatedCounter;
  };
/* Unused...?
  struct TimerData {
    bool running;

    TimerClockInfo tci;
    TimerTickInfo tti;
  }; */
}

Timer::
Timer (bool start_now)
  throw ()
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_opaque), sizeof (cpu_set_t)> ()));
  ocpiAssert (sizeof (m_opaque) >= sizeof (cpu_set_t));
  init(start_now);
}
Timer::
Timer(uint32_t seconds_in, uint32_t nanoseconds_in)  throw()
  : expiration(seconds_in, nanoseconds_in){
  init(true);
}
Timer::
Timer(Time time)  throw()
  : expiration(time) {
  init(time != 0);
}
void Timer::init(bool start_now) {
  tti.accumulatedCounter = tti.upper = tti.lower = 0;
  tci.accumulatedTime.set(0);
  if ((running = start_now)) { // Intentionally setting "running"
    if (useHighResTimer()) {
      register volatile unsigned int tb_lower, tb_upper = 0, tb_upper_tmp;

      do {
        TBU( tb_upper );
        TBL( tb_lower );
        TBU( tb_upper_tmp );
      }
      while ( tb_upper != tb_upper_tmp );

      tti.lower = tb_lower;
      tti.upper = tb_upper;
    }
    else
    {
      tci.startTime = Time::now();
    }
  }
}

Timer::
~Timer ()
  throw ()
{
}

void
Timer::
start ()
  throw ()
{ 
  ocpiAssert (!running);
  
  running = true;
  if (useHighResTimer()) {
#ifdef OCPI_OS_linux
    cpu_set_t &org_mask = *(cpu_set_t *)m_opaque;
    cpu_set_t mask;
    const size_t len = sizeof(mask);

    // Store off original affinity
    pid_t pid_id = getpid();
    if (sched_getaffinity(pid_id, len, &org_mask) < 0) {
      perror("sched_getaffinity");
      ocpiAssert (-1);
    }

    // Pin this process to the CPU we are currently on (AV-436)
    unsigned cpu_id;
#ifdef OCPI_OS_VERSION_r5
    typedef long (*vgetcpu_t)(unsigned int *cpu, unsigned int *node, unsigned long *tcache);
    vgetcpu_t vgetcpu = (vgetcpu_t)VSYSCALL_ADDR(__NR_vgetcpu);
    ocpiCheck(vgetcpu(&cpu_id, NULL, NULL) == 0);
#else
    cpu_id = sched_getcpu();
#endif
    mask=org_mask;
    CPU_ZERO(&mask);                     // clears the cpuset
    CPU_SET(cpu_id, &mask);              // set only this CPU
    sched_setaffinity(pid_id, len, &mask);
#endif
    register volatile unsigned int tb_lower, tb_upper = 0, tb_upper_tmp;

    do {
      TBU( tb_upper );
      TBL( tb_lower );
      TBU( tb_upper_tmp );
    }
    while ( tb_upper != tb_upper_tmp );

    tti.lower = tb_lower;
    tti.upper = tb_upper;
  }
  else
  {
    tci.startTime = Time::now();
  }
}

ElapsedTime
Timer::
stop ()
  throw ()
{
  ElapsedTime et = getElapsed();
#ifdef OCPI_OS_linux
  if (useHighResTimer()) {
    // Reset CPU affinity
    cpu_set_t &org_mask = *(cpu_set_t *)m_opaque;
    sched_setaffinity(getpid(), sizeof(org_mask), &org_mask);
  }
#endif
  running = false;
  return et;
}

void
Timer::
reset ()
  throw ()
{
  init(false);
}
void
Timer::
restart ()
  throw ()
{
  reset();
  start();
}

bool
Timer::expired() {
  if (expiration != 0 && getElapsed() > expiration) {
    running = false;
    return true;
  }
  return false;
}
ElapsedTime
Timer::
getElapsed ()
  throw ()
{
  if (running) {
    if (useHighResTimer()) {
      register volatile unsigned int tb_lower, tb_upper = 0, tb_upper_tmp, cpu;

#if defined ( __x86_64__ ) || defined ( __i386__ )
      (void) tb_upper_tmp; // Stop unused warnings
      eTBL( tb_lower, tb_upper, cpu ); // Macro modifies tb_upper and tb_lower atomically
#else
      do {
        TBU( tb_upper );
        TBL( tb_lower );
        TBU( tb_upper_tmp );
        }
      while ( tb_upper != tb_upper_tmp );
#endif

      unsigned long long t2 = (((unsigned long long) tb_upper)  << 32) | tb_lower;
      unsigned long long t1 = (((unsigned long long) tti.upper) << 32) | tti.lower;
      ocpiAssert (t2 > t1);

      tti.accumulatedCounter += t2 - t1;
      tti.lower = tb_lower;
      tti.upper = tb_upper;
      ocpiAssert (g_counterFreq != 0);
      // FIXME: perhaps better accuracy dealing with ocpi ticks directly
      tci.accumulatedTime.set((uint32_t)(tti.accumulatedCounter / g_counterFreq),
                              (uint32_t)(((tti.accumulatedCounter % g_counterFreq) *
                                          1000000000ull) / g_counterFreq));
    }
    else {
      Time stopTime = Time::now();

      ocpiAssert (stopTime >= tci.startTime);

      tci.accumulatedTime += stopTime - tci.startTime;
      tci.startTime = stopTime;
    }
  }
  return tci.accumulatedTime;
}

ElapsedTime Timer::
getRemaining() throw() {
  return expiration - getElapsed();
}


void
Timer::
getValue (ElapsedTime & timer)
  throw ()
{
  ocpiAssert(!running);

  if (useHighResTimer()) {
    ocpiAssert (g_counterFreq != 0);
    // FIXME: perhaps better accuracy dealing with ocpi ticks directly
    timer.set((uint32_t)(tti.accumulatedCounter / g_counterFreq),
              (uint32_t)(((tti.accumulatedCounter % g_counterFreq) * 1000000000ull) /
                         g_counterFreq));
  } else
    timer = tci.accumulatedTime;
}

void
Timer::
getPrecision (ElapsedTime & prec)
  throw ()
{
  if (useHighResTimer()) {
    ocpiAssert (g_counterFreq != 0);


    Time::TimeVal r = (Time::ticksPerSecond + g_counterFreq/2)/g_counterFreq;
    prec.set(r ? r : 1);
  } else {
#ifdef __APPLE__
    prec.set(0, 10000000); // 10ms - hah!
#else
    struct timespec res;
    clockid_t x = OCPI_CLOCK_TYPE;
// This is due to clock_getres returning bad values for this clock
// FIXME: check whether this is specific to centos6
#if defined(CLOCK_MONOTONIC_RAW) && defined(CLOCK_MONOTONIC)
    if (OCPI_CLOCK_TYPE == CLOCK_MONOTONIC_RAW)
      x = CLOCK_MONOTONIC;
#endif
    ocpiCheck(clock_getres (x, &res) == 0);
    prec.set((uint32_t)res.tv_sec, (uint32_t)res.tv_nsec);
#endif
  }
}
  }
}
