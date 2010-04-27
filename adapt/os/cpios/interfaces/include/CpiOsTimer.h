// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

// -*- c++ -*-

#ifndef CPI_OS_TIMER_H__
#define CPI_OS_TIMER_H__

#include <CpiOsDataTypes.h>

/**
 * \file
 *
 * \brief Measure and accumulate elapsed wall clock time.
 * Revision History:
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

namespace CPI {
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

    class Timer {
    public:
      /**
       * \brief A data structure to hold time in seconds and nanoseconds.
       *
       * Supports simple comparisons and arithmetic.
       *
       */

      struct ElapsedTime
      {
        unsigned int seconds;
        unsigned int nanoseconds;
      };

    public:
      /**
       * Constructor.
       *
       * \param[in] start Whether the timer should be started right away.
       *                  If false, start() needs to be called separately.
       *
       * \post The accumulated elapsed time is initialized to zero.
       */

      Timer (bool start = false)
        throw ();

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
       */

      void stop ()
        throw ();

      /**
       * Reset the timer.
       *
       * \post The timer is stopped.
       * \post The accumulated elapsed time is zero.
       */

      void reset ()
        throw ();

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

      /**
       * Query the operating system/hardware granularity for measuring time.
       *
       * \param[out] prec Data structure to hold the result in seconds and
       *                  nanoseconds.
       */

      static void getPrecision (ElapsedTime & prec)
        throw ();

    private:
      uint64_t m_osOpaque[4];
    };

    namespace
    {
      const static unsigned int nanoseconds_per_second = 1000000000;

    } // End: namespace <unamed>

    /**
     * @brief
     *   Greater than comparison of two ElapsedTime instances.
     *
     * @returns true if the ElapsedTime instance on the right hand
     *          side represents an elapsed time that is greater than
     *          the left hand side. Returns false otherwise.
     */
    inline
    bool operator> ( const CPI::OS::Timer::ElapsedTime& lhs,
                     const CPI::OS::Timer::ElapsedTime& rhs )
      throw ( )
    {
      if ( lhs.seconds > rhs.seconds )
      {
        return true;
      }

      if ( lhs.seconds == rhs.seconds &&
	   lhs.nanoseconds > rhs.nanoseconds )
      {
        return true;
      }

      return false;
    }

    /**
     * @brief
     *   Less than comparison of two ElapsedTime instances.
     *
     * @returns true if the ElapsedTime instance on the right hand
     *          side represents an elapsed time that is less than
     *          the left hand side. Returns false otherwise.
     */
    inline
    bool operator< ( const CPI::OS::Timer::ElapsedTime& lhs,
                     const CPI::OS::Timer::ElapsedTime& rhs )
      throw ( )
    {
      if ( lhs.seconds > rhs.seconds )
      {
        return false;
      }

      if ( lhs.seconds == rhs.seconds &&
	   lhs.nanoseconds > rhs.nanoseconds )
      {
        return false;
      }

      return true;
    }

    /**
     * @brief
     *   Equality test between two ElapsedTime.
     *
     * @returns true if the two ElapsedTime instances represent
     *          the same amount of elapsed time. Returns false
     *          otherwise.
     */
    inline
    bool operator== ( const CPI::OS::Timer::ElapsedTime& lhs,
                      const CPI::OS::Timer::ElapsedTime& rhs )
      throw ( )
    {
      return ( lhs.seconds == rhs.seconds ) &&
             ( lhs.nanoseconds == rhs.nanoseconds );
    }

    /**
     * @brief
     *   Inequality test between two ElapsedTime.
     *
     * @returns true if the two ElapsedTime instances represent a
     *          different amount of elapsed time. Returns false
     *          otherwise.
     */
    inline
    bool operator!= ( const CPI::OS::Timer::ElapsedTime& lhs,
                      const CPI::OS::Timer::ElapsedTime& rhs )
      throw ( )
    {
      return ( lhs.seconds != rhs.seconds ) ||
             ( lhs.nanoseconds != rhs.nanoseconds );
    }

    /**
     * @brief
     *   Adds two ElapsedTime instances.
     *
     * @returns Sum of the ElapsedTime instances.
     */
    inline
    CPI::OS::Timer::ElapsedTime operator+
                                   ( const CPI::OS::Timer::ElapsedTime& lhs,
                                     const CPI::OS::Timer::ElapsedTime& rhs )
      throw ( )
    {
      CPI::OS::Timer::ElapsedTime result;

      result.seconds = lhs.seconds + rhs.seconds;

      result.nanoseconds = lhs.nanoseconds + rhs.nanoseconds;

      if ( result.nanoseconds >= nanoseconds_per_second )
      {
        result.nanoseconds -= nanoseconds_per_second;
        result.seconds += 1;
      }

      return result;
    }

    /**
     * @brief
     *   Subtracts two ElapsedTime instances.
     *
     * @returns Difference between the ElapsedTime instances.
     */
    inline
    CPI::OS::Timer::ElapsedTime operator-
                                   ( const CPI::OS::Timer::ElapsedTime& lhs,
                                     const CPI::OS::Timer::ElapsedTime& rhs )
      throw ( )
    {
      CPI::OS::Timer::ElapsedTime result;

      if ( lhs.nanoseconds > rhs.nanoseconds )
      {
        result.seconds  = lhs.seconds - rhs.seconds;
        result.nanoseconds = lhs.nanoseconds - rhs.nanoseconds;
      }
      else
      {
        result.seconds = lhs.seconds - rhs.seconds - 1;

        result.nanoseconds = lhs.nanoseconds +
                             nanoseconds_per_second -
                             rhs.nanoseconds;
      }

      return result;
    }

  } // End: namespace OS

} // End: namespace CPI

#endif
