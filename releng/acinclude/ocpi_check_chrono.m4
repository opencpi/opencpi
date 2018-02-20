dnl This file is protected by Copyright. Please refer to the COPYRIGHT file
dnl distributed with this source distribution.
dnl
dnl This file is part of OpenCPI <http://www.opencpi.org>
dnl
dnl OpenCPI is free software: you can redistribute it and/or modify it under the
dnl terms of the GNU Lesser General Public License as published by the Free
dnl Software Foundation, either version 3 of the License, or (at your option)
dnl any later version.
dnl
dnl OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
dnl WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
dnl FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
dnl details.
dnl
dnl You should have received a copy of the GNU Lesser General Public License
dnl along with this program. If not, see <http://www.gnu.org/licenses/>.

dnl This function finds if a compiler's std::chrono is acceptable (AV-1567)
dnl If it is, defines OCPI_HAVE_STD_CHRONO
dnl If not defined, the target program will have to use boost::chrono instead
dnl Note: This does NOT try to find boost::chrono like
dnl https://www.gnu.org/software/autoconf-archive/ax_boost_chrono.html

AC_DEFUN([OCPI_CHECK_CHRONO], [
  AC_LANG_PUSH([C++])
  AC_CACHE_CHECK([if ${CXX-c++} has acceptable std::chrono], [ax_cv_stdchrono],
    [AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [[
#include <chrono>
class Time {
  public:
    static const uint64_t ticksPerSecond = 1ull << 32;
    typedef uint64_t TimeVal; // our base type for time where 1 == 1/2^32 of a second
    // Match C++11 std::chrono conventions:
    typedef TimeVal rep;
    typedef std::ratio<1, ticksPerSecond> period;
    typedef std::chrono::duration<rep, period> duration;
    inline duration get_duration() { return duration(m_time); }
  // protected:
    TimeVal m_time;
    Time() : m_time(0x04030201) {}; // Silly default constructor
}; // Time
        ]], [[
  using namespace std::chrono;
  // In constructor 'std::chrono::duration<_Rep, _Period>::duration(const std::chrono::duration<_Rep, _Period>&) [with _Rep2 = long int, _Period2 = std::ratio<1l, 1000000000l>, _Rep = long int, _Period = std::ratio<1l, 1000000l>]':

  Time time1;

  auto time_now = high_resolution_clock::now();
  time_now -= duration_cast<nanoseconds>( time1.get_duration() );

        ]])],
      [ax_cv_stdchrono=yes],
      [ax_cv_stdchrono=no]
    )
  ])
  AS_VAR_IF([ax_cv_stdchrono],[yes],[
    AC_DEFINE([OCPI_HAVE_STD_CHRONO], 1, [Define to 1 if your C++11 std::chrono is acceptable])
  ])
  AC_LANG_POP([C++])
])
