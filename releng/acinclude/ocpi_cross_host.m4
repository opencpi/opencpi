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

AC_DEFUN([OCPI_OCPI_CROSS_HOST],
[
  AC_ARG_VAR(OCPI_CROSS_HOST, [default OCPI cross-compiling executable prefix])
  AC_CACHE_CHECK([cross-compiling host prefix], ocpi_cv_ocpi_cross_host, [
    AC_ARG_WITH(cross-host,
    AC_HELP_STRING([--with-cross-host], [ocpi cross-compiling executable prefix (default from environment variable 'OCPI_CROSS_HOST', e.g. arm-xilinx-linux-gnueabi)]),
    ocpi_cv_ocpi_cross_host="$withval",
    AS_IF([test "x${OCPI_CROSS_HOST}" != "x"],
          [ocpi_cv_ocpi_cross_host=${OCPI_CROSS_HOST}],
          AC_MSG_ERROR([--with-cross-host was not specified and OCPI_CROSS_HOST was not found in the environment.]))
    )
  ])
  AC_SUBST(ocpi_cross_host, $ocpi_cv_ocpi_cross_host)
  AC_SUBST(OCPI_CROSS_HOST, $ocpi_cv_ocpi_cross_host)
])
