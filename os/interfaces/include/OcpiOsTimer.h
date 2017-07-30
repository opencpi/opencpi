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

#include <OcpiOsDataTypes.h>
/**
 * \file
 *
 * \brief Measure and accumulate elapsed wall clock time.
 * Revision History:
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

namespace OCPI {
  namespace OS {

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
      struct TimerTickInfo {
	uint32_t lower, upper;
	uint64_t accumulatedCounter;
      } tti;
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
