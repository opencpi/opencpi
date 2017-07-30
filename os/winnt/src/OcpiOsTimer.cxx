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

#include <OcpiOsTimer.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsAssert.h>
#include <windows.h>
#include <cstdlib>

namespace {
  struct TimerData {
    LARGE_INTEGER startedCounter;
    unsigned long long accumulatedCounter;
    bool running;
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
  td.accumulatedCounter = 0;

  if ((td.running = start)) {
    BOOL rc = QueryPerformanceCounter (&td.startedCounter);
    ocpiAssert (rc);
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
  BOOL rc = QueryPerformanceCounter (&td.startedCounter);
  ocpiAssert (rc);
}

void
OCPI::OS::Timer::
stop ()
  throw ()
{
  LARGE_INTEGER counterNow;
  BOOL rc = QueryPerformanceCounter (&counterNow);

  TimerData & td = o2td (m_osOpaque);
  ocpiAssert (td.running);
  ocpiAssert (rc);
  // the performance counter should not wrap around for a few years
  ocpiAssert (counterNow.QuadPart >= td.startedCounter.QuadPart);
  td.running = false;
  td.accumulatedCounter += counterNow.QuadPart - td.startedCounter.QuadPart;
}

void
OCPI::OS::Timer::
reset ()
  throw ()
{
  TimerData & td = o2td (m_osOpaque);
  td.running = false;
  td.accumulatedCounter = 0;
}

void
OCPI::OS::Timer::
getValue (ElapsedTime & timer)
  throw ()
{
  TimerData & td = o2td (m_osOpaque);
  ocpiAssert (!td.running);

  LARGE_INTEGER counterFreq;
  BOOL rc = QueryPerformanceFrequency (&counterFreq);
  ocpiAssert (rc);

  timer.seconds = static_cast<unsigned int> (td.accumulatedCounter / counterFreq.QuadPart);
  timer.nanoseconds = static_cast<unsigned int> (((td.accumulatedCounter % counterFreq.QuadPart) * 1000000000ull) / counterFreq.QuadPart);
}

void
OCPI::OS::Timer::
getPrecision (ElapsedTime & prec)
  throw ()
{
  LARGE_INTEGER counterFreq;
  BOOL rc = QueryPerformanceFrequency (&counterFreq);
  ocpiAssert (rc);
  ocpiAssert (counterFreq.QuadPart);

  prec.seconds = 0;
  prec.nanoseconds = static_cast<unsigned int> (1000000000ull / counterFreq.QuadPart);
}
