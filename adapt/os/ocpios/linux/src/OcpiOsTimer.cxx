
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


#include <OcpiOsTimer.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsAssert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <cstring>
#include <cstdlib>

#ifdef __APPLE__
#include <sys/time.h>
inline static void now(struct timespec *t) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  TIMEVAL_TO_TIMESPEC(&tv, t);
}
#endif
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

namespace {
  class CounterFreq {
  public:
    CounterFreq ();
    bool useHighResTimer () const;
    operator unsigned long long () const;

  private:
    bool m_useHighResTimer;
    unsigned long long m_counterFreq;
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
  : m_useHighResTimer (false)
{
  int fd = open ("/proc/cpuinfo", O_RDONLY);

  if (fd < 0) {
    // Looks like /proc/cpuinfo doesn't exist
    m_counterFreq = 0;
    return;
  }

  // This ought to be enough
  char tmp[2048];
  ssize_t nread = read (fd, tmp, 2047);
  close (fd);

  if (nread < 0) {
    m_counterFreq = 0;
    return;
  }

  tmp[nread] = 0;

  /*
   * We always trust a value called "timebase period".
   */

  char * ptr;

  if ((ptr = std::strstr (tmp, "timebase period"))) {
    unsigned int period;
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
    unsigned int freq;
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

  if ((ptr = std::strstr (tmp, "cpu cores"))) {
    ptr += 9;
    while (*ptr == ' ' || *ptr == '\t' || *ptr == ':') {
      ptr++;
    }
    unsigned long ncores = strtoul (ptr, NULL, 10);
    if (ncores != 1)
      return;
  }
  if ((ptr = std::strstr (tmp, "cpu MHz"))) {
    unsigned int mhz, khz = 0;
    char * endPtr;

    // Position ptr at the beginning of the value
    ptr += 7;
    while (*ptr == ' ' || *ptr == '\t' || *ptr == ':') {
      ptr++;
    }

    mhz = strtoul (ptr, &endPtr, 10);

    if (*endPtr++ == '.') {
      if (*endPtr >= '0' && *endPtr <= '9') {
        khz = 100 * static_cast<int> (*endPtr++ - '0');

        if (*endPtr >= '0' && *endPtr <= '9') {
          khz += 10 * static_cast<int> (*endPtr++ - '0');

          if (*endPtr >= '0' && *endPtr <= '9') {
            khz += static_cast<int> (*endPtr - '0');
          }
        }
      }
    }

    if (mhz) {
      m_useHighResTimer = true;
      m_counterFreq = 1000000ull * mhz + 1000ull * khz;
      return;
    }
  }

#endif

  /*
   * Did not find anything useful in /proc/cpuinfo.  Tell the Timer
   * implementation to fall back to clock_gettime().
   */

  m_useHighResTimer = false;
  m_counterFreq = 0;
}

inline
bool
CounterFreq::useHighResTimer () const
{
  return m_useHighResTimer;
}

inline
CounterFreq::operator unsigned long long () const
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

  #define TBU( t ) tb_upper_tmp = tb_upper;
  #define TBL( t ) __asm__ __volatile__ ( "rdtsc" : "=a" ( tb_lower ), \
                                                    "=d" ( tb_upper ) );

#else // vxWorks Diab and Green Hills inline assembler format

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

#endif

/*
 * ----------------------------------------------------------------------
 * OCPI::OS::Timer implementation.
 * ----------------------------------------------------------------------
 */

namespace {
  struct TimerClockInfo {
    struct timespec startTime;
    OCPI::OS::Timer::ElapsedTime accumulatedTime;
  };

  struct TimerTickInfo {
    unsigned int lower, upper;
    unsigned long long accumulatedCounter;
  };

  struct TimerData {
    bool running;

    union {
      TimerClockInfo tci;
      TimerTickInfo tti;
    } t;
  };
}

inline
TimerData &
o2td (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<TimerData *> (ptr);
}

OCPI::OS::Timer::
Timer (bool start)
  throw ()
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (TimerData)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (TimerData));

  TimerData & td = o2td (m_osOpaque);

  if (useHighResTimer()) {
    TimerTickInfo & tti = td.t.tti;
    tti.accumulatedCounter = 0;

    if ((td.running = start)) {
      register unsigned int tb_lower, tb_upper, tb_upper_tmp;

      do {
        TBU( tb_upper );
        TBL( tb_lower );
        TBU( tb_upper_tmp );
      }
      while ( tb_upper != tb_upper_tmp );

      tti.lower = tb_lower;
      tti.upper = tb_upper;
    }
  }
  else {
    TimerClockInfo & tci = td.t.tci;
    tci.accumulatedTime.seconds = 0;
    tci.accumulatedTime.nanoseconds = 0;

    if ((td.running = start)) {
#ifdef __APPLE__
      now(&tci.startTime);
#else
      if (clock_gettime (CLOCK_MONOTONIC, &tci.startTime) != 0) {
        ocpiAssert (0);
      }
#endif
    }
  }
}

OCPI::OS::Timer::
~Timer ()
  throw ()
{
}

void
OCPI::OS::Timer::
start ()
  throw ()
{
  TimerData & td = o2td (m_osOpaque);
  ocpiAssert (!td.running);
  td.running = true;

  if (useHighResTimer()) {
    TimerTickInfo & tti = td.t.tti;
    register unsigned int tb_lower, tb_upper, tb_upper_tmp;

    do {
      TBU( tb_upper );
      TBL( tb_lower );
      TBU( tb_upper_tmp );
    }
    while ( tb_upper != tb_upper_tmp );

    tti.lower = tb_lower;
    tti.upper = tb_upper;
  }
  else {
    TimerClockInfo & tci = td.t.tci;
#ifdef __APPLE__
      now(&tci.startTime);
#else
    if (clock_gettime (CLOCK_MONOTONIC, &tci.startTime) != 0) {
      ocpiAssert (0);
    }
#endif
  }
}

void
OCPI::OS::Timer::
stop ()
  throw ()
{
  if (useHighResTimer()) {
    register unsigned int tb_lower, tb_upper, tb_upper_tmp;
    TimerData & td = o2td (m_osOpaque);
    TimerTickInfo & tti = td.t.tti;

    do {
      TBU( tb_upper );
      TBL( tb_lower );
      TBU( tb_upper_tmp );
    }
    while ( tb_upper != tb_upper_tmp );

    ocpiAssert (td.running);

    unsigned long long t2 = (((unsigned long long) tb_upper)  << 32) | tb_lower;
    unsigned long long t1 = (((unsigned long long) tti.upper) << 32) | tti.lower;
    ocpiAssert (t2 > t1);

    td.running = false;
    tti.accumulatedCounter += t2 - t1;
  }
  else {
    struct timespec stopTime;

#ifdef __APPLE__
    now(&stopTime);
#else
    if (clock_gettime (CLOCK_MONOTONIC, &stopTime) != 0) {
      ocpiAssert (0);
    }
#endif

    TimerData & td = o2td (m_osOpaque);
    TimerClockInfo & tci = td.t.tci;
    ocpiAssert (td.running);

    ocpiAssert ((stopTime.tv_sec > tci.startTime.tv_sec) ||
               (stopTime.tv_sec == tci.startTime.tv_sec &&
                stopTime.tv_nsec >= tci.startTime.tv_nsec));

    td.running = false;
    tci.accumulatedTime.seconds += stopTime.tv_sec - tci.startTime.tv_sec;

    if (tci.startTime.tv_nsec > stopTime.tv_nsec) {
      tci.accumulatedTime.seconds--;
      tci.accumulatedTime.nanoseconds += 1000000000 + stopTime.tv_nsec - tci.startTime.tv_nsec;
    }
    else {
      tci.accumulatedTime.nanoseconds += stopTime.tv_nsec - tci.startTime.tv_nsec;
    }

    if (tci.accumulatedTime.nanoseconds >= 1000000000) {
      tci.accumulatedTime.seconds++;
      tci.accumulatedTime.nanoseconds -= 1000000000;
    }
  }
}

void
OCPI::OS::Timer::
reset ()
  throw ()
{
  TimerData & td = o2td (m_osOpaque);

  td.running = false;

  if (useHighResTimer()) {
    TimerTickInfo & tti = td.t.tti;
    tti.accumulatedCounter = 0;
  }
  else {
    TimerClockInfo & tci = td.t.tci;
    tci.accumulatedTime.seconds = 0;
    tci.accumulatedTime.nanoseconds = 0;
  }
}

void
OCPI::OS::Timer::
getValue (ElapsedTime & timer)
  throw ()
{
  TimerData & td = o2td (m_osOpaque);
  ocpiAssert (!td.running);

  if (useHighResTimer()) {
    TimerTickInfo & tti = td.t.tti;
    ocpiAssert (g_counterFreq != 0);
    timer.seconds = static_cast<unsigned int> (tti.accumulatedCounter / g_counterFreq);
    timer.nanoseconds = static_cast<unsigned int> (((tti.accumulatedCounter % g_counterFreq) * 1000000000ull) / g_counterFreq);
  }
  else {
    TimerClockInfo & tci = td.t.tci;
    timer = tci.accumulatedTime;
  }
}

void
OCPI::OS::Timer::
getPrecision (ElapsedTime & prec)
  throw ()
{
  if (useHighResTimer()) {
    ocpiAssert (g_counterFreq != 0);

    prec.seconds = 0;
    prec.nanoseconds = static_cast<unsigned int> (1000000000ull / g_counterFreq);

    if (!prec.nanoseconds) {
      // round up better-than-nanosecond resolution to 1.
      prec.nanoseconds = 1;
    }
  }
  else {
    struct timespec res;

#ifdef __APPLE__
      now(&res);
#else
    if (clock_getres (CLOCK_MONOTONIC, &res) != 0) {
      ocpiAssert (0);
    }
#endif

    prec.seconds = res.tv_sec;
    prec.nanoseconds = res.tv_nsec;
  }
}
