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

AC_DEFUN([OCPI_OCPI_TOOL_HOST],
[
        AC_REQUIRE([AC_CANONICAL_BUILD])
	AC_ARG_VAR(OCPI_TOOL_HOST, [default OCPI target host version])
	AC_CACHE_CHECK([tool host], ocpi_cv_ocpi_tool_host, [
		AC_ARG_WITH(tool-host,
		AC_HELP_STRING([--with-tool-host], [ocpi target host version (default from environment variable 'OCPI_TOOL_HOST', default is current host)]),
		ocpi_cv_ocpi_tool_host="$withval",
                AS_IF([test "x${OCPI_TOOL_HOST}" != "x"],
                      [ocpi_cv_ocpi_tool_host=${OCPI_TOOL_HOST}],
                      [
                        ocpi_cv_ocpi_tool_host=$(./bootstrap/scripts/getPlatform.sh | cut -f4 -d" ")
		        AC_MSG_WARN(OCPI_TOOL_HOST was not specified. The value has been evaluated from the environment:)
                      ])
	        )
	])
	AC_SUBST(ocpi_tool_host, $ocpi_cv_ocpi_tool_host)
	AC_SUBST(OCPI_TOOL_HOST, $ocpi_cv_ocpi_tool_host)
	AC_SUBST(ocpi_tool_os, [$(echo $ocpi_cv_ocpi_tool_host | cut -d- -f1)])
	AC_SUBST(OCPI_TOOL_OS, [$(echo $ocpi_cv_ocpi_tool_host | cut -d- -f1)])
	AC_SUBST(ocpi_tool_os_version, [$(echo $ocpi_cv_ocpi_tool_host | cut -d- -f2)])
	AC_SUBST(OCPI_TOOL_OS_VERSION, [$(echo $ocpi_cv_ocpi_tool_host | cut -d- -f2)])
	AC_SUBST(ocpi_tool_arch, [$(echo $ocpi_cv_ocpi_tool_host | cut -d- -f3)])
	AC_SUBST(OCPI_TOOL_ARCH, [$(echo $ocpi_cv_ocpi_tool_host | cut -d- -f3)])
])
