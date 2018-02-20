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

AC_DEFUN([OCPI_OCPI_CROSS_BUILD_BIN_DIR],
[
        AC_ARG_VAR(OCPI_CROSS_BUILD_BIN_DIR, [default OCPI cross-compiling executable directory])
	AC_CACHE_CHECK([cross-compiling binary dir], ocpi_cv_ocpi_cross_dir, [
		AC_ARG_WITH(cross-dir,
		AC_HELP_STRING([--with-cross-dir], [ocpi cross-compiling executable directory (default from environment variable 'OCPI_CROSS_BUILD_BIN_DIR')]),
		ocpi_cv_ocpi_cross_dir="$withval",
        if test "x${OCPI_CROSS_BUILD_BIN_DIR}" != "x"; then
		ocpi_cv_ocpi_cross_dir=${OCPI_CROSS_BUILD_BIN_DIR}
	else 
	AC_MSG_ERROR([--with-cross-dir was not specified and OCPI_CROSS_BUILD_BIN_DIR was not found in the environment.])
	fi)
	])
	AC_SUBST(ocpi_cross_build_bin_dir, $ocpi_cv_ocpi_cross_dir)
	AC_SUBST(OCPI_CROSS_BUILD_BIN_DIR, $ocpi_cv_ocpi_cross_dir)
])
