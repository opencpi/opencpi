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

dnl This function finds where clock_gettime is defined (might require
dnl librt linked in) and then checks if it supports CLOCK_MONOTONIC_RAW.
dnl If it does not, it will fall back to CLOCK_MONOTONIC. (AV-1494)
dnl Result is only exported as env variable "ocpi_clock_type"

AC_DEFUN([OCPI_CLOCK_TYPES],[
  AC_SEARCH_LIBS([clock_gettime],[rt])
  AC_LANG_PUSH([C++])
  AC_MSG_CHECKING([clock_gettime clock type])
  AC_RUN_IFELSE([
    AC_LANG_SOURCE([
    #include <ctime>
    int main() {
      struct timespec ts;
      return
        !clock_gettime(CLOCK_MONOTONIC_RAW, &ts) &&
	!clock_getres(CLOCK_MONOTONIC_RAW, &ts) &&
	ts.tv_sec == 0 ? 0 : 1;
    }
    ])],
    [export ocpi_clock_type=CLOCK_MONOTONIC_RAW],
    [export ocpi_clock_type=CLOCK_MONOTONIC],
    AC_COMPILE_IFELSE([
      AC_LANG_SOURCE([
      #include <ctime>
      struct timespec ts;
      int main() {
        return clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
      }
      ])],
      [export ocpi_clock_type=CLOCK_MONOTONIC_RAW],
      [export ocpi_clock_type=CLOCK_MONOTONIC]))
  AC_MSG_RESULT([${ocpi_clock_type}])
  AC_LANG_POP()
  AC_DEFINE_UNQUOTED([GETTIME_CLOCK_TYPE], [${ocpi_clock_type}], [CLOCK_MONOTONIC or CLOCK_MONOTONIC_RAW if system supports it])
  dnl AC_SUBST(OCPI_CLOCK_TYPE, $ocpi_clock_type)
])
