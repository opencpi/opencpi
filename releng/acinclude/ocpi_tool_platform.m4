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

AC_DEFUN([OCPI_OCPI_TOOL_PLATFORM],
[
	AC_ARG_VAR(OCPI_TOOL_PLATFORM, [default OCPI tool host platform])
	AC_CACHE_CHECK([host platform], ocpi_cv_ocpi_tool_platform, [
		AC_ARG_WITH(tool-platform,
		AC_HELP_STRING([--with-tool-platform], [ocpi tool host platform (default from environment variable 'OCPI_TOOL_PLATFORM', default is current host)]),
		ocpi_cv_ocpi_tool_platform="$withval",
                AS_IF([test "x${OCPI_TOOL_PLATFORM}" != "x"],
                      [ocpi_cv_ocpi_tool_platform=${OCPI_TOOL_PLATFORM}],
                      [
                        ocpi_cv_ocpi_tool_platform=$(./bootstrap/scripts/getPlatform.sh | cut -f5 -d" ")
                        AC_MSG_WARN(OCPI_TOOL_PLATFORM was not specified. The value has been evaluated from the environment:)
                      ])
	        )
	])
	AC_SUBST(ocpi_tool_platform, $ocpi_cv_ocpi_tool_platform)
	AC_SUBST(OCPI_TOOL_PLATFORM, $ocpi_cv_ocpi_tool_platform)
])
