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

// -*- c++ -*-
#ifndef OCPI_OS_TIMER_H__
#define OCPI_OS_TIMER_H__

// TODO: Move all to C++11 chrono and remove custom classes, etc.

#include <utility> // std::pair
#include "ocpi-config.h"
/**
 * \file
 *
 * \brief Measure and accumulate elapsed wall clock time.
 * Revision History:
 *      7/26/2016 - Moved high precision on x86 to C++11/Boost chrono
 *
 *     12/09/2015 - Added support for locking core execution of timer
 *                  to a single core when using highpercision option
 *
 *     06/24/2009 - Frank Pilhofer
 *                  Bugfix in ElapsedTime operator> and operator<.
 *
 *     11/18/2008 - Michael Pepe
 *                  Add arithmetic operators for ElapsedTime.
 *
 *     10/15/2008 - Michael Pepe
 *                  Enlarge m_osOpaque for Linux/PPC support.
 *
 *     04/01/2008 - Frank Pilhofer
 *                  Initial version.
 */

#if defined (__x86_64__) || defined (__i386__)
// AV-1564 - just use C++11
// If the resolution offered by high_resolution_clock is not sufficient, we might need to bring
// back the RDTSCP code that is currently in git history (and a .hold file here). Why?
// http://stackoverflow.com/a/13096917/836748 shows some reasons
//
// I had proof that sched_affinity was being ignored, so other options/ideas:
// ===
// Linux offers "smp_call_function_single" which you can say "go run this
// one function on THAT specific CPU." That COULD cause all kinds of ugly
// context switching, etc. But it would be accurate - put the RDTSCP into a
// callback function and fire it off that way. Problem is that it is very
// difficult to find information on it.
// http://lxr.free-electrons.com/source/include/linux/smp.h#L27
// ===
// Another option would be switching to CLOCK_MONOTONIC_RAW explicitly
// ===
// Option three is we kick off a service thread that we use pthreads library
// to lock to a single core but I'm not sure how the calling architecture
// could work offhand.
#ifdef OCPI_HAVE_STD_CHRONO
#include <chrono>
#define OCPI_USE_CHRONO
#else
#warning Could not use C++11 library for timers - using OCPI_GETTIME_CLOCK_TYPE
#endif
#endif

namespace OCPI {
  namespace OS {

#ifdef OCPI_USE_CHRONO
    // Utility function to convert elapsed time into seconds:nanoseconds as pair of uint32_t
    // Had weird template issues when it was within the Time class
    template <class TimePoint>
    std::pair<uint32_t, uint32_t> __chrono_elapsed(const TimePoint &tp) {
      using namespace std::chrono;
      auto elapsed = TimePoint::clock::now() - tp;
      auto sec = duration_cast<seconds>(elapsed);
      elapsed -= sec;
      auto nsec = duration_cast<nanoseconds>(elapsed);
      // After 2038 rollover, "should" be safe except for rollover event itself.
      return std::make_pair(static_cast<uint32_t>(sec.count()), static_cast<uint32_t>(nsec.count()));
    }
#endif

    /**
     * \brief Measure and accumulate elapsed wall clock time.
     *
     * The Timer class functions like a stopwatch that can be started,
     * stopped, and re-started multiple times.  While running, the Timer
     * instance accumulates the elapsed wall-clock time.
     *
     * When stopped, the elapsed running time can be retrieved in seconds
     * and nanoseconds.
     */

    class Time {
    public:
      static const uint32_t nsPerSecond = 1000000000;
      static const uint64_t ticksPerSecond = 1ull << 32;
      typedef uint64_t TimeVal; // our base type for time where 1 == 1/2^32 of a second
#ifdef OCPI_USE_CHRONO
      // Match C++11 std::chrono conventions:
      typedef TimeVal rep;
      typedef std::ratio<1, ticksPerSecond> period;
      typedef std::chrono::duration<rep, period> duration;
#endif
    protected:
      TimeVal m_time;
    public:
      inline uint32_t seconds() const { return (uint32_t)(m_time >> 32); }
      // This is rounded to the nearest nanosecond
      inline uint32_t nanoseconds() const {
        return (uint32_t)
          (((m_time & 0xffffffffll) * nsPerSecond + ticksPerSecond/2) / ticksPerSecond);
      }
      inline Time(uint32_t seconds_in, uint32_t nanoseconds_in) {
        set(seconds_in, nanoseconds_in);
      }
      inline Time() {}
      inline Time(TimeVal t) { m_time = t;}
      inline void set(uint32_t seconds_in, uint32_t nanoseconds_in) {
        m_time = (((uint64_t)nanoseconds_in << 32) + nsPerSecond / 2) / nsPerSecond +
          ((uint64_t)seconds_in << 32);
      }
      inline void set(TimeVal t) { m_time = t;}
      inline void set(const std::pair<uint32_t, uint32_t> &in) { set(in.first, in.second); }
      inline bool operator> (const Time &r) const { return m_time > r.m_time; }
      inline bool operator< (const Time &r) const { return m_time < r.m_time; }
      inline bool operator>= (const Time &r) const { return m_time >= r.m_time; }
      inline bool operator<= (const Time &r) const { return m_time <= r.m_time; }
      inline bool operator== (const Time &r) const { return m_time == r.m_time; }
      inline bool operator!= (const Time &r) const { return m_time != r.m_time; }
      inline Time operator+ (const Time &r) const { return Time(m_time + r.m_time); }
      inline Time operator- (const Time &r) const { return Time(m_time - r.m_time); }
      inline Time operator+= (const Time &r) { return Time(m_time += r.m_time); }
      inline uint64_t bits() const { return m_time; }
      static Time now();
#ifdef OCPI_USE_CHRONO
      // Convert to C++11 native duration:
      inline duration get_duration() { return duration(m_time); }
      // Convert from C++11 native timepoint, advance to now, with guard against POD, e.g. set_elapsed(0):
      template <class TimePoint>
        inline typename std::enable_if< std::is_class<TimePoint>::value, void>::type
        set_elapsed (const TimePoint &tp) {
          auto val = __chrono_elapsed(tp);
          set(val.first, val.second);
        }
      // TODO: set(timepoint) without advancing (from C++11 to our m_time)
#endif
    };
    typedef Time ElapsedTime;

    class Timer {
    public:
      /**
       * \brief A data structure to hold time in seconds and nanoseconds.
       *
       * Supports simple comparisons and arithmetic.
       *
       */

    private:
      struct TimerClockInfo {
        Time startTime;
        ElapsedTime accumulatedTime;
      } tci;
      uint64_t m_opaque[16];
      Time expiration;
      bool running;
      void init(bool start);
    public:
      /**
       * Constructor.
       *
       * \param[in] start_now Whether the timer should be started right away.
       *                  If false, start() needs to be called separately.
       *
       * \post The accumulated elapsed time is initialized to zero.
       */

      Timer (bool start_now = false)
        throw ();
      Timer (uint32_t seconds_in, uint32_t nanoseconds_in)
	throw ();
      Timer (Time)
	throw ();
      bool expired();

      /**
       * Destructor.
       */

      ~Timer ()
        throw ();

      /**
       * Start the timer.
       *
       * \pre The timer shall be stopped.
       * \post The timer is running.
       */

      void start ()
        throw ();

      /**
       * Stop the timer.
       *
       * \pre The timer shall be running.
       * \post The timer is stopped.
       * Returns elapsed time since start
       */

      ElapsedTime stop ()
        throw ();

      /**
       * Reset the timer.
       *
       * \post The timer is stopped.
       * \post The accumulated elapsed time is zero.
       */

      void reset ()
        throw ();
      inline void reset (uint32_t seconds, uint32_t nanoseconds)
        throw () {
	reset(Time(seconds, nanoseconds));
      }
      inline void reset (Time time)
        throw () {
	reset();
	expiration = time;
      }
      // reset and start
      void restart ()
        throw ();
      inline void restart (uint32_t seconds, uint32_t nanoseconds)
        throw () {
	reset(seconds, nanoseconds);
	start();
      }
      inline void restart (Time time)
        throw () {
	reset(time);
	start();
      }

      /**
       * Query the accumulated elapsed time.
       *
       * \param[out] timer Data structure to hold the result in seconds and
       *                   nanoseconds.
       *
       * \pre The timer shall be stopped.
       */

      void getValue (ElapsedTime & timer)
        throw ();
      // Does not stop the timer, allowing it to be sampled
      ElapsedTime getElapsed() throw();
      ElapsedTime getRemaining() throw();
      inline Time getStart() const throw() { return tci.startTime;}


      /**
       * Query the operating system/hardware granularity for measuring time.
       *
       * \param[out] prec Data structure to hold the result in seconds and
       *                  nanoseconds.
       */

      static void getPrecision (ElapsedTime & prec)
        throw ();

    };
  } // End: namespace OS
} // End: namespace OCPI

#endif
