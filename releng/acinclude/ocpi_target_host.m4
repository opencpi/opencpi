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

AC_DEFUN([OCPI_OCPI_TARGET_HOST],
[
	AC_ARG_VAR(OCPI_TARGET_HOST, [default OCPI target hosting processor])
	AC_CACHE_CHECK([target host], ocpi_cv_ocpi_target_host, [
		AC_ARG_WITH(target,
		AC_HELP_STRING([--with-target], [target hosting processor (default from environment variable 'OCPI_TARGET_HOST')]),
		ocpi_cv_ocpi_target_host=$withval,
		if test "x${OCPI_TARGET_HOST}" != "x"; then
			ocpi_cv_ocpi_target_host=${OCPI_TARGET_HOST}
		else
			AC_MSG_WARN([OCPI_TARGET_HOST was not specified and set to OCPI_TOOL_HOST:])
			ocpi_cv_ocpi_target_host=$ocpi_cv_ocpi_tool_host
		fi)
	])
AC_SUBST(ocpi_target_host, $ocpi_cv_ocpi_target_host)
AC_SUBST(OCPI_TARGET_HOST, $ocpi_cv_ocpi_target_host)
AC_SUBST(ocpi_target_os, [$(echo $ocpi_cv_ocpi_target_host | cut -d- -f1)])
AC_SUBST(OCPI_TARGET_OS, [$(echo $ocpi_cv_ocpi_target_host | cut -d- -f1)])
AC_SUBST(ocpi_target_os_version, [$(echo $ocpi_cv_ocpi_target_host | cut -d- -f2)])
AC_SUBST(OCPI_TARGET_OS_VERSION, [$(echo $ocpi_cv_ocpi_target_host | cut -d- -f2)])
AC_SUBST(ocpi_target_arch, [$(echo $ocpi_cv_ocpi_target_host | cut -d- -f3)])
AC_SUBST(OCPI_TARGET_ARCH, [$(echo $ocpi_cv_ocpi_target_host | cut -d- -f3)])
])
