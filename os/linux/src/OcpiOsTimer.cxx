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

#include "av_team.h"
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

#ifdef OCPI_USE_CHRONO
using chono_clock_type = std::chrono::steady_clock; // high_resolution_clock;
typedef struct  {
  chono_clock_type::time_point tp;
} TimerTickInfo;
#else
typedef struct  {
  uint32_t lower, upper;
  uint64_t accumulatedCounter; // move to chono_clock_type::duration if everything moves to C++11
} TimerTickInfo;
#endif

/*
 * ----------------------------------------------------------------------
 * Frequency of the high-resolution timestamp counter, in Hz.
 *
 * On Mercury Linux/PPC, this is initialized from the "timebase" value
 * in /proc/cpuinfo (e.g., "timebase period : 24000 psecs", indicating
 * a frequency of 41.6 MHz).
 *
 * On Linux/x86, we use C++11's chrono library so don't use this.
 * ----------------------------------------------------------------------
 */

#ifndef OCPI_USE_CHRONO
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

#endif

/*
 * ----------------------------------------------------------------------
 * Read timer register.
 * ----------------------------------------------------------------------
 */

#if defined ( __GNUC__ ) && defined ( _ARCH_PPC )
  #define TBU( t ) __asm volatile ( "mfspr %0,269" : "=r" ( t ) )
  #define TBL( t ) __asm volatile ( "mfspr %0,268" : "=r" ( t ) )
#elif defined(OCPI_USE_CHRONO)
  // There are no macros needed. But we don't want a "no arch" warning either.
#elif defined ( __x86_64__ ) || defined ( __i386__ )
  #define TBU( t ) tb_upper_tmp = tb_upper;
  #define TBL( t ) __asm__ __volatile__ ( "rdtsc" : "=a" ( tb_lower ), \
                                                    "=d" ( tb_upper ) );
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
Timer::
Timer (bool start_now)
  throw ()
  : running(false)
{
  init(start_now);
}
Timer::
Timer(uint32_t seconds_in, uint32_t nanoseconds_in)  throw()
  : expiration(seconds_in, nanoseconds_in), running(false) {
  init(true);
}
Timer::
Timer(Time time) throw()
  : expiration(time), running(false) {
  init(time != 0);
}
void Timer::init(bool start_now) {
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_opaque), sizeof (TimerTickInfo)> ()));
  ocpiAssert (sizeof (m_opaque) >= sizeof (TimerTickInfo));
  TimerTickInfo &tti = *(TimerTickInfo *)m_opaque;
  tci.accumulatedTime.set(0);
  if (start_now) {
    start();
  } else { // not start
#ifdef OCPI_USE_CHRONO
    // Initialize using a placement new
    new (&tti.tp) chono_clock_type::time_point();
#else
    tti.accumulatedCounter = tti.upper = tti.lower = 0;
#endif
  }
}

Timer::
~Timer ()
  throw ()
{
  // chono_clock_type::time_point has no destructor to manually call (used placement new)
}

void
Timer::
start ()
  throw ()
{
  ocpiAssert (!running);

  running = true;
  TimerTickInfo &tti = *(TimerTickInfo *)m_opaque;
#ifdef OCPI_USE_CHRONO
  using namespace std::chrono;
  tti.tp = chono_clock_type::now();
  // To allow restarts, we subtract tci.accumulatedTime from now() to be the new effective start time
  // Need [base-10 value] of [ticksPerSecond count] so duration_cast
  if (0 != tci.accumulatedTime.bits())
    tti.tp -= duration_cast<nanoseconds>( tci.accumulatedTime.get_duration() );
#else // not Chrono
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
  } else {
    tci.startTime = Time::now();
  }
#endif
}

ElapsedTime
Timer::
stop ()
  throw ()
{
  ElapsedTime et = getElapsed();
  running = false;
  return et;
}

void
Timer::
reset ()
  throw ()
{
  running = false;
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
  // updates and returns tci.accumulatedTime
  if (running) {
    TimerTickInfo &tti = *(TimerTickInfo *)m_opaque;
#ifdef OCPI_USE_CHRONO
    tci.accumulatedTime.set_elapsed(tti.tp);
#else
    if (useHighResTimer()) {
      register volatile unsigned int tb_lower, tb_upper = 0, tb_upper_tmp;
      do {
        TBU( tb_upper );
        TBL( tb_lower );
        TBU( tb_upper_tmp );
        }
      while ( tb_upper != tb_upper_tmp );

      const unsigned long long t2 = (((unsigned long long) tb_upper)  << 32) | tb_lower;
      const unsigned long long t1 = (((unsigned long long) tti.upper) << 32) | tti.lower;

      ocpiAssert (t2 > t1);
      tti.accumulatedCounter += t2 - t1;
      tti.lower = tb_lower;
      tti.upper = tb_upper;
      ocpiAssert (g_counterFreq != 0);
      // FIXME: perhaps better accuracy dealing with ocpi ticks directly
      tci.accumulatedTime.set((uint32_t)(tti.accumulatedCounter / g_counterFreq),
                              (uint32_t)(((tti.accumulatedCounter % g_counterFreq) *
                                          1000000000ull) / g_counterFreq));
    } else {
      Time stopTime = Time::now();

      ocpiAssert (stopTime >= tci.startTime);

      tci.accumulatedTime += stopTime - tci.startTime;
      tci.startTime = stopTime;
    }
#endif // chrono
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
  TimerTickInfo &tti = *(TimerTickInfo *)m_opaque;

#ifdef OCPI_USE_CHRONO
  timer.set_elapsed(tti.tp);
#else
  if (useHighResTimer()) {
    ocpiAssert (g_counterFreq != 0);
    // FIXME: perhaps better accuracy dealing with ocpi ticks directly
    timer.set((uint32_t)(tti.accumulatedCounter / g_counterFreq),
              (uint32_t)(((tti.accumulatedCounter % g_counterFreq) * 1000000000ull) /
                         g_counterFreq));
  } else
    timer = tci.accumulatedTime;
#endif
}

void
Timer::
getPrecision (ElapsedTime & prec)
  throw ()
{
#ifdef OCPI_USE_CHRONO
  using namespace std::chrono;
  // Put 1 count into the clock and then ask how long it was
  chono_clock_type::duration dur(1);
  auto sec = duration_cast<seconds>(dur);
  dur -= sec;
  auto nsec = duration_cast<nanoseconds>(dur);
  prec.set(static_cast<uint32_t>(sec.count()), static_cast<uint32_t>(nsec.count()));
  static_assert(chono_clock_type::is_steady, "chono_clock_type::is_steady is false!");
#else
  if (useHighResTimer()) {
    ocpiAssert (g_counterFreq != 0);
    Time::TimeVal r = (Time::ticksPerSecond + g_counterFreq/2)/g_counterFreq;
    prec.set(r ? r : 1);
  } else {
#ifdef __APPLE__
    prec.set(0, 10000000); // 10ms - hah!
#else
    struct timespec res;
    ocpiCheck(clock_getres (OCPI_CLOCK_TYPE, &res) == 0);
    prec.set((uint32_t)res.tv_sec, (uint32_t)res.tv_nsec);
#endif
  }
#endif
}

  } // OCPI::OS
} // OCPI
