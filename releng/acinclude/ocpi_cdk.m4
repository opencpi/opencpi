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

AC_DEFUN([OCPI_OCPI_CDK_DIR],
[
        AC_ARG_VAR(OCPI_CDK_DIR, [default OCPI target CDK location])
	AC_CACHE_CHECK([for opencpi cdk], ocpi_cv_ocpi_cdk_dir, [
		AC_ARG_WITH(cdk-dir,
		AC_HELP_STRING([--with-cdk-dir], [target ocpi cdk location directory (default from environment variable 'OCPI_CDK_DIR', else use 'prefix')]),
		ocpi_cv_ocpi_cdk_dir=$withval,
		if test "x${OCPI_CDK_DIR}" != "x"; then
			ocpi_cv_ocpi_cdk_dir=${OCPI_CDK_DIR}
		elif test "x${prefix}" != "xNONE"; then	
			AC_MSG_WARN(OCPI_CDK_DIR was not set. The value is now set to the default ${prefix})
			ocpi_cv_ocpi_cdk_dir=${prefix}
		else
			AC_MSG_WARN(OCPI_CDK_DIR was not set. The value is now set to the default /opt/opencpi/cdk)
			ocpi_cv_ocpi_cdk_dir=/opt/opencpi/cdk
		fi
		)
	])
AC_SUBST(ocpi_cdk_dir, $ocpi_cv_ocpi_cdk_dir)
AM_SUBST_NOTMAKE(OCPI_CDK_DIR)
])
