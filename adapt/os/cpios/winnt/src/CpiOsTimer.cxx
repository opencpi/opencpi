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

#include <CpiOsTimer.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsAssert.h>
#include <windows.h>

namespace {
  struct TimerData {
    LARGE_INTEGER startedCounter;
    unsigned long long accumulatedCounter;
    bool running;
  };
}

inline
TimerData &
o2td (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<TimerData *> (ptr);
}

CPI::OS::Timer::
Timer (bool start)
  throw ()
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (TimerData)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (TimerData));

  TimerData & td = o2td (m_osOpaque);
  td.accumulatedCounter = 0;

  if ((td.running = start)) {
    BOOL rc = QueryPerformanceCounter (&td.startedCounter);
    cpiAssert (rc);
  }
}

CPI::OS::Timer::
~Timer ()
  throw ()
{
}

void
CPI::OS::Timer::
start ()
  throw ()
{
  TimerData & td = o2td (m_osOpaque);
  cpiAssert (!td.running);
  td.running = true;
  BOOL rc = QueryPerformanceCounter (&td.startedCounter);
  cpiAssert (rc);
}

void
CPI::OS::Timer::
stop ()
  throw ()
{
  LARGE_INTEGER counterNow;
  BOOL rc = QueryPerformanceCounter (&counterNow);

  TimerData & td = o2td (m_osOpaque);
  cpiAssert (td.running);
  cpiAssert (rc);
  // the performance counter should not wrap around for a few years
  cpiAssert (counterNow.QuadPart >= td.startedCounter.QuadPart);
  td.running = false;
  td.accumulatedCounter += counterNow.QuadPart - td.startedCounter.QuadPart;
}

void
CPI::OS::Timer::
reset ()
  throw ()
{
  TimerData & td = o2td (m_osOpaque);
  td.running = false;
  td.accumulatedCounter = 0;
}

void
CPI::OS::Timer::
getValue (ElapsedTime & timer)
  throw ()
{
  TimerData & td = o2td (m_osOpaque);
  cpiAssert (!td.running);

  LARGE_INTEGER counterFreq;
  BOOL rc = QueryPerformanceFrequency (&counterFreq);
  cpiAssert (rc);

  timer.seconds = static_cast<unsigned int> (td.accumulatedCounter / counterFreq.QuadPart);
  timer.nanoseconds = static_cast<unsigned int> (((td.accumulatedCounter % counterFreq.QuadPart) * 1000000000ull) / counterFreq.QuadPart);
}

void
CPI::OS::Timer::
getPrecision (ElapsedTime & prec)
  throw ()
{
  LARGE_INTEGER counterFreq;
  BOOL rc = QueryPerformanceFrequency (&counterFreq);
  cpiAssert (rc);
  cpiAssert (counterFreq.QuadPart);

  prec.seconds = 0;
  prec.nanoseconds = static_cast<unsigned int> (1000000000ull / counterFreq.QuadPart);
}
