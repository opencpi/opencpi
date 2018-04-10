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

AC_DEFUN([OCPI_OCPI_TARGET_PLATFORM],
[
  AC_ARG_VAR(OCPI_TARGET_PLATFORM, [default OCPI target platform])
  AC_CACHE_CHECK([target platform], ocpi_cv_ocpi_target_platform, [
    AC_ARG_WITH(target-platform,
      AC_HELP_STRING([--with-target-platform], [target hosting platform (default from environment variable 'OCPI_TARGET_PLATFORM')]),
      ocpi_cv_ocpi_target_platform=$withval,
      if test "x${OCPI_TARGET_PLATFORM}" != "x"; then
        ocpi_cv_ocpi_target_platform=${OCPI_TARGET_PLATFORM}
      else
        ocpi_cv_ocpi_target_platform=$(./bootstrap/scripts/getPlatform.sh | cut -f5 -d" ")
        AC_MSG_WARN(OCPI_TARGET_PLATFORM was not specified and set to OCPI_TOOL_PLATFORM:)
      fi
    )
  ])
AC_SUBST(ocpi_target_platform, $ocpi_cv_ocpi_target_platform)
AC_SUBST(OCPI_TARGET_PLATFORM, $ocpi_cv_ocpi_target_platform)
])
