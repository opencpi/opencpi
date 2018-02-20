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

AC_DEFUN([OCPI_OCPI_DEBUG],
[
   dnl Default to on, or env variable
   AS_IF([test -n "${OCPI_DEBUG+1}"],
     [ocpi_debug=${OCPI_DEBUG}],
     [ocpi_debug=1]
   )
   AC_ARG_VAR(OCPI_DEBUG, [default decision to build with debug enabled])
   AC_ARG_WITH([debug],
     AS_HELP_STRING([--without-debug],[decision to build with debug enabled (defaults to on or 'OCPI_DEBUG')]),
     dnl Command line, if present, overrides
     AS_IF([test "x$with_debug" != "xno"],
       AS_IF([test -n "${OCPI_DEBUG+1}" && test ${ocpi_debug} -eq 0],
         AC_MSG_WARN([Overriding OCPI_DEBUG=0 with command --with-debug]))
       [ocpi_debug=1]
       ,
       AS_IF([test -n "${OCPI_DEBUG+1}" && test ${ocpi_debug} -eq 1],
         AC_MSG_WARN([Overriding OCPI_DEBUG=1 with command --without-debug]))
       [ocpi_debug=0]
     )
   )
   AC_SUBST(ocpi_debug, ${ocpi_debug})
   AC_SUBST(OCPI_DEBUG, ${ocpi_debug})
])
