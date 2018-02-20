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

#include <time.h>
#include "OcpiTime.h"

// gps and utc conversions.
// the type time_t is the "seconds" UTC from <time.h>
// the type Time::GpsTime is the 64 bit GPS time we use everywhere.
// some funkiness is due to the fact that time_t is SIGNED (due to history)

static struct LeapSecond {
  uint16_t month, day, year, extra;
  time_t 
    utc,   // The UTC second that was added
    gps;   // the GPS second afterwhich the leap second was added
} s_leapSeconds[] = {
  {  6, 30, 1972, 0, 0, 0},
  { 12, 31, 1972, 0, 0, 0},
  { 12, 31, 1973, 0, 0, 0},
  { 12, 31, 1974, 0, 0, 0},
  { 12, 31, 1975, 0, 0, 0},
  { 12, 31, 1976, 0, 0, 0},
  { 12, 31, 1977, 0, 0, 0},
  { 12, 31, 1978, 0, 0, 0},
  { 12, 31, 1979, 0, 0, 0},
  {  6, 30, 1981, 0, 0, 0},
  {  6, 30, 1982, 0, 0, 0},
  {  6, 30, 1983, 0, 0, 0},
  {  6, 30, 1985, 0, 0, 0},
  { 12, 31, 1987, 0, 0, 0},
  { 12, 31, 1989, 0, 0, 0},
  { 12, 31, 1990, 0, 0, 0},
  {  6, 30, 1992, 0, 0, 0},
  {  6, 30, 1993, 0, 0, 0},
  {  6, 30, 1994, 0, 0, 0},
  { 12, 31, 1995, 0, 0, 0},
  {  6, 30, 1997, 0, 0, 0},
  { 12, 31, 1998, 0, 0, 0},
  { 12, 31, 2005, 0, 0, 0},
  { 12, 31, 2008, 0, 0, 0},
  {  6, 30, 2012, 0, 0, 0},
  {  6, 30, 2015, 0, 0, 0},
  { 12, 31, 2016, 0, 0, 0},
  {  0,  0,    0, 0, 0, 0}
};
static time_t s_gpsZeroInUtc;
static LeapSecond *s_gpsStart, *s_gpsLast;

 // Initialize what we need, utc/gps conversion, etc.

static void init() {
  static struct tm t; // initialized to zero
  // GPS was zero on 0h 6-Jan-1980, and has no leap seconds
  t.tm_mon = 1 - 1;        // January
  t.tm_year = 1980 - 1900; // 1980
  t.tm_mday = 6;           // 6th 
  t.tm_hour = 0;
  t.tm_min = 0;
  t.tm_sec = 0;
  s_gpsZeroInUtc = timegm(&t);
  uint16_t leaps = 0;
  LeapSecond *lp;
  for (lp = s_leapSeconds; lp->month; lp++) {
    t.tm_mon = lp->month - 1;
    t.tm_year = lp->year - 1900;
    t.tm_mday = lp->day;
    t.tm_hour = 23;
    t.tm_min = 59;
    t.tm_sec = 59;
    lp->utc = timegm(&t) + 1;      // the UTC time before which a second was "skipped" in UTC, but kept in GPS
    if (lp->utc > s_gpsZeroInUtc)
      lp->gps = lp->utc + ++leaps; // the GPS time of that same second
    lp->extra = leaps;
  }
  s_gpsLast = lp;
}

Time::UtcTime Time::
gps2utc(GpsTime argGps) {
  if (!s_gpsLast)
    init();
  time_t gps = (time_t)argGps;
  for (LeapSecond *lp = s_gpsLast; lp >= s_gpsStart; lp--)
    if (gps <= lp->gps)
      return (time_t)(gps + s_gpsZeroInUtc - lp->extra);
  return 0;
}

Time::GpsTime Time::
utc2gps(time_t utc) {
  if (!s_gpsLast)
    init();
  for (LeapSecond *lp = s_gpsLast; lp >= s_gpsStart; lp--)
    if (utc <= lp->gps)
      return (GpsTime)utc - s_gpsZeroInUtc + lp->extra;
  return 0;
}
