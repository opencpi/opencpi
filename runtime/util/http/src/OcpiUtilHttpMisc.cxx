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

#include <OcpiUtilHttpMisc.h>
#include <OcpiUtilMisc.h>
#include <string>
#include <ctime>
#include <cstdlib>

bool
OCPI::Util::Http::isToken (const std::string & str)
  throw ()
{
  std::string::size_type fni;

  for (fni=0; fni<str.length(); fni++) {
    if (str[fni] < 32 || str[fni] >= 127) {
      return false;
    }
    else {
      switch (str[fni]) {
      case '(': case ')': case '<': case '>': case '@': case ',':
      case ';': case ':': case '\\': case '\"': case '/': case '[':
      case ']': case '?': case '=': case '{': case '}':
        return false;
      }
    }
  }

  return true;
}

/*
 * HTTP-date    = rfc1123-date | rfc850-date | asctime-date
 * rfc1123-date = wkday "," SP date1 SP time SP "GMT"
 * rfc850-date  = weekday "," SP date2 SP time SP "GMT"
 * asctime-date = wkday SP date3 SP time SP 4DIGIT
 * date1        = 2DIGIT SP month SP 4DIGIT ; day month year (e.g., 02 Jun 1982)
 * date2        = 2DIGIT "-" month "-" 2DIGIT ; day-month-year (e.g., 02-Jun-82)
 * date3        = month SP ( 2DIGIT | ( SP 1DIGIT )) ; month day (e.g., Jun  2)
 * time         = 2DIGIT ":" 2DIGIT ":" 2DIGIT ; 00:00:00 - 23:59:59
 * wkday        = "Mon" | "Tue" | "Wed" | "Thu" | "Fri" | "Sat" | "Sun"
 * weekday      = "Monday" | "Tuesday" | "Wednesday"
 *                | "Thursday" | "Friday" | "Saturday" | "Sunday"
 * month        = "Jan" | "Feb" | "Mar" | "Apr"
 *                | "May" | "Jun" | "Jul" | "Aug"
 *                | "Sep" | "Oct" | "Nov" | "Dec"
 */

namespace {
  const char * shortWkdayNames[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0
  };

  const char * shortMonthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    0
  };
}

std::time_t
OCPI::Util::Http::parseHttpDate (const std::string & httpDate)
  throw (std::string)
{
  struct tm timeStr;
  std::string::size_type idx, idx2;
  std::string weekday, date, time, tail;
  int format;

  idx = httpDate.find (' ');
  if (idx==0 || idx == std::string::npos) {
    throw std::string ("no space in date string");
  }

  bool colon = (idx && httpDate[idx-1] == ',');

  if (colon && idx==4) {
    format = 1; // rfc1123-date
    idx2 = ++idx + 11;
  }
  else if (!colon && idx==3) {
    format = 3; // asctime-date
    idx2 = ++idx + 6;
  }
  else {
    format = 2; // rfc850-date
    idx2 = ++idx + 9;
  }

  if (idx2 > httpDate.length() || httpDate[idx2] != ' ') {
    throw std::string ("no space after date");
  }

  date = httpDate.substr (idx, idx2-idx);
  idx  = idx2+1;
  idx2 = httpDate.find (' ', idx);

  if (idx2 == std::string::npos) {
    throw std::string ("no space after time");
  }

  time = httpDate.substr (idx, idx2-idx);
  tail = httpDate.substr (idx2+1);

  /*
   * Parse Day, Month and Year
   */

  std::string day;
  std::string month;
  std::string year;

  switch (format) {
  case 1: // rfc1123-date
    {
      if (date.length() != 11) {
        throw std::string ("rfc1123 date string should be length 11");
      }
      day   = date.substr (0, 2);
      month = date.substr (3, 3);
      year  = date.substr (7);
    }
    break;

  case 2: // rfc850-date
    {
      if (date.length() != 9) {
        throw std::string ("rfc850 date string should be length 9");
      }

      day   = date.substr (0, 2);
      month = date.substr (3, 3);
      year  = date.substr (7);
    }
    break;

  case 3: // asctime-date
    {
      if (date.length() != 6) {
        throw std::string ("asctime date string should be length 6");
      }

      month = date.substr (0, 3);
      day = date.substr (4);
      year = tail;
    }
    break;
  }

  char * dptr = 0;
  timeStr.tm_mday = (int)strtoul (day.c_str(), &dptr, 10);

  if (!dptr || *dptr) {
    throw std::string ("invalid day of the month");
  }

  int monthsSinceJan;
  for (monthsSinceJan=0; shortMonthNames[monthsSinceJan]; monthsSinceJan++) {
    if (month == shortMonthNames[monthsSinceJan]) {
      break;
    }
  }
  
  if (!shortMonthNames[monthsSinceJan]) {
    throw std::string ("invalid month name");
  }
  
  timeStr.tm_mon = monthsSinceJan;

  dptr = 0;
  int yearno = (int)std::strtol (year.c_str(), &dptr, 10);

  if (!dptr || *dptr) {
    throw std::string ("invalid year");
  }
  
  if (format == 1 || format == 3) {
    timeStr.tm_year = yearno - 1900;
  }
  else {
    timeStr.tm_year = yearno;
  }

  /*
   * Parse Hour, Minute and Second
   */

  if (time.length() != 8) {
    throw std::string ("time string should be of length 8");
  }

  std::string hour = time.substr (0, 2);
  std::string minute = time.substr (3, 2);
  std::string second = time.substr (6, 2);

  char * tptr = 0;
  timeStr.tm_hour = (int)std::strtol (hour.c_str(), &tptr, 10);

  if (!tptr || *tptr) {
    throw std::string ("invalid hour");
  }

  tptr = 0;
  timeStr.tm_min = (int)std::strtol (minute.c_str(), &tptr, 10);

  if (!tptr || *tptr) {
    throw std::string ("invalid minute");
  }

  tptr = 0;
  timeStr.tm_sec = (int)std::strtol (second.c_str(), &tptr, 10);

  if (!tptr || *tptr) {
    throw std::string ("invalid second");
  }

  timeStr.tm_isdst = -1;

  std::time_t theTime = std::mktime (&timeStr);

  if (theTime == static_cast<time_t> (-1)) {
    throw std::string ("cannot convert time");
  }

  /*
   * Now theTime represents timeStr when interpreted as local time.
   * However, HTTP dates are always GMT, so we need to compensate
   * for that.
   */

  /*
   * warning: non-reentrant code, uses gmtime's static buffer
   */

  std::time_t curTime = ::std::time (0);
  std::time_t curTimeAsGMT = std::mktime (std::gmtime (&curTime));
  theTime -= (curTimeAsGMT - curTime);

  return theTime;
}

std::string
OCPI::Util::Http::makeHttpDate (std::time_t curTime)
  throw (std::string)
{
  std::string res;

  /*
   * warning: non-reentrant code, uses gmtime's static buffer
   */

  struct tm * tmStr = std::gmtime (&curTime);

  if (!tmStr) {
    throw std::string ("oops");
  }

  /*
   * Date
   */

  res = shortWkdayNames[tmStr->tm_wday];
  res += ", ";

  if (tmStr->tm_mday < 10) {
    res += "0";
  }

  res += OCPI::Util::integerToString (tmStr->tm_mday);
  res += " ";
  res += shortMonthNames[tmStr->tm_mon];
  res += " ";
  res += OCPI::Util::integerToString (tmStr->tm_year + 1900);

  /*
   * Time
   */

  res += " ";
  if (tmStr->tm_hour < 10) {
    res += "0";
  }
  res += OCPI::Util::integerToString (tmStr->tm_hour);
  res += ":";
  if (tmStr->tm_min < 10) {
    res += "0";
  }
  res += OCPI::Util::integerToString (tmStr->tm_min);
  res += ":";
  if (tmStr->tm_sec < 10) {
    res += "0";
  }
  res += OCPI::Util::integerToString (tmStr->tm_sec);
  res += " GMT";

  return res;
}

std::string
OCPI::Util::Http::getHttpTimestamp ()
  throw ()
{
  return makeHttpDate (time(0));
}
