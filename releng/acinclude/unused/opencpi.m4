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

dnl let the user specify the ocpi_base_dir. the priority is
dnl 1. --with-base-dir argument
dnl 2. the OCPI_BASE_DIR environment variable
dnl 3. the --prefix argument
AC_DEFUN([OCPI_OCPI_BASE_DIR],
[
	AC_ARG_VAR(OCPI_BASE_DIR, [default OCPI base directory])
	AC_CACHE_CHECK([for opencpi base], ocpi_cv_ocpi_base_dir, [
		AC_ARG_WITH(base-dir,
		AC_HELP_STRING([--with-base-dir], [ocpi base directory (default from environment variable 'OCPI_BASE_DIR', else use 'prefix')]),
		ocpi_cv_ocpi_base_dir=$withval,
        if test "x${OCPI_BASE_DIR}" != "x"; then
			ocpi_cv_ocpi_base_dir=${OCPI_BASE_DIR}
		elif test "x${prefix}" != "xNONE"; then
			ocpi_cv_ocpi_base_dir=${prefix}
		else
			ocpi_cv_ocpi_base_dir=$ac_default_prefix
		fi
		)
    AC_MSG_CHECKING($ocpi_cv_ocpi_base_dir)
	dnl check if this is a cross compile session, prepend the sysroot
	AS_IF([test "x$cross_compiling" = "xyes"],[ 
	CROSS_SYSROOT=`$CC --print-sysroot`
	ocpi_cv_ocpi_base_dir=${CROSS_SYSROOT}${ocpi_cv_ocpi_base_dir}
	])
	])
AC_SUBST(ocpi_base_dir, $ocpi_cv_ocpi_base_dir)
])

dnl Check that OCPI is installed so we can compile against it
AC_DEFUN([OCPI_CHECK_OCPI],
	[
	AC_REQUIRE([OCPI_OCPI_BASE_DIR])
	AC_REQUIRE([GR_LIB64])
	AC_MSG_CHECKING([to see ocpi is installed])
	if test -e $ocpi_cv_ocpi_base_dir/lib${gr_libdir_suffix}/pkgconfig/opencpi.pc; then
		PKG_CONFIG_PATH="${ocpi_cv_ocpi_base_dir}/lib${gr_libdir_suffix}/pkgconfig:${PKG_CONFIG_PATH}"
		export PKG_CONFIG_PATH
		AC_MSG_RESULT($ocpi_cv_ocpi_base_dir)
	elif test -e $ocpi_cv_ocpi_base_dir/lib64${gr_libdir_suffix}/pkgconfig/opencpi.pc; then
		PKG_CONFIG_PATH="${ocpi_cv_ocpi_base_dir}/lib64${gr_libdir_suffix}/pkgconfig:${PKG_CONFIG_PATH}"
		export PKG_CONFIG_PATH
		AC_MSG_RESULT($ocpi_cv_ocpi_base_dir)
	else
		AC_MSG_ERROR(You must specify a valid opencpi root directory. Try using --with-base-dir)
	fi
])

dnl use OCPI_BASE_DIR as the default prefix unless --prefix is provided
AC_DEFUN([OCPI_OCPI_BASE_DIR_AS_PREFIX],
	[
	AS_IF([test "x${prefix}" = "xNONE"], [
	dnl Prefix wasn't provided, we need to use ocpi base dir
	AC_REQUIRE([OCPI_OCPI_BASE_DIR])
	AS_IF([test "x${ocpi_cv_ocpi_base_dir}" = "xNONE"], [
		AC_MSG_ERROR([opencpi base directory is not set; this is not expected])
	])
	dnl Use base dir value for prefix
	ac_default_prefix=${ocpi_cv_ocpi_base_dir}
	prefix=${ocpi_cv_ocpi_base_dir}
	AC_MSG_NOTICE(using ${ocpi_cv_ocpi_base_dir} as installation prefix)
	])
])


dnl use OCPI_BASE_DIR as the default prefix unless --prefix is provided
AC_DEFUN([OCPI_OCPI_CDK_DIR_AS_PREFIX],
	[
	AS_IF([test "x${prefix}" = "xNONE"], [
	dnl Prefix wasn't provided, we need to use ocpi base dir
	AC_REQUIRE([OCPI_OCPI_CDK_DIR])
	AS_IF([test "x${ocpi_cv_ocpi_cdk_dir}" = "xNONE"], [
		AC_MSG_ERROR([the cdk directory is not set; this is not expected])
	])
	dnl Use base dir value for prefix
	ac_default_prefix=${ocpi_cv_ocpi_cdk_dir}
	prefix=${ocpi_cv_ocpi_cdk_dir}
	AC_MSG_NOTICE(using ${ocpi_cv_ocpi_cdk_dir} as ref prefix)
	])
])





