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

AC_DEFUN([OCPI_CHECK_LZMA],
[
	AC_CACHE_CHECK([for lzma dir], lzma_cv_lzma_dir, [
		AC_ARG_WITH(lzma,
                  AC_HELP_STRING([--with-lzma], [lzma directory (default '/opt/opencpi/prerequisites/xz')]),
                  [lzma_cv_lzma_dir=$withval],
                  [lzma_cv_lzma_dir=/opt/opencpi/prerequisites/xz]
		)
	])
AS_IF([test -e ${lzma_cv_lzma_dir}/include/lzma.h],[],AC_MSG_ERROR([cannot verify \"${lzma_cv_lzma_dir}/include/lzma.h\" exists.]))
AC_SUBST(lzma_dir, ${lzma_cv_lzma_dir})
dnl AC_SUBST([LZMA_CFLAGS],[-I${lzma_cv_lzma_dir}/include])
dnl AC_SUBST([LZMA_CXXFLAGS],[-I${lzma_cv_lzma_dir}/include])
dnl AC_SUBST([LZMA_LDFLAGS],["-L${lzma_cv_lzma_dir}/${ocpi_target_host}/lib -llzma"])
AC_SUBST([LZMA_STATIC_LIBS],["${lzma_cv_lzma_dir}/${ocpi_target_host}/lib/liblzma.a"])
dnl AM_SUBST_NOTMAKE(LZMA_CFLAGS)
dnl AM_SUBST_NOTMAKE(LZMA_CXXFLAGS)
dnl AM_SUBST_NOTMAKE(LZMA_LDFLAGS)
AM_SUBST_NOTMAKE(LZMA_STATIC_LIBS)
])
