dnl CHECK_GTEST: look for gtest libraries and headers (OpenCPI prereq specific)

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

AC_DEFUN([OCPI_CHECK_GTEST],
[
	AC_CACHE_CHECK([for gtest], gtest_cv_gtest_dir, [
		AC_ARG_WITH(gtest,
                  AC_HELP_STRING([--with-gtest], [gtest directory (default '/opt/opencpi/prerequisites/gtest')]),
                  [gtest_cv_gtest_dir=$withval],
                  [gtest_cv_gtest_dir=/opt/opencpi/prerequisites/gtest]
		)
	])
AS_IF([test -e ${gtest_cv_gtest_dir}/include/gtest/gtest.h],[],AC_MSG_ERROR([cannot verify directory \"${gtest_cv_gtest_dir}/include/gtest/gtest.h\" exists.]))
AC_SUBST(gtest_dir, $gtest_cv_gtest_dir)
AC_SUBST([GTEST_CFLAGS],[-I${gtest_cv_gtest_dir}/include])
AC_SUBST([GTEST_CXXFLAGS],[-I${gtest_cv_gtest_dir}/include])
dnl AC_SUBST([GTEST_LDFLAGS],["-L${gtest_cv_gtest_dir}/${ocpi_target_host}/lib -lgtest"])
AC_SUBST([GTEST_STATIC_LIBS], [${gtest_cv_gtest_dir}/${ocpi_target_host}/lib/libgtest.a])
AM_SUBST_NOTMAKE(GTEST_CFLAGS)
AM_SUBST_NOTMAKE(GTEST_CXXFLAGS)
dnl AM_SUBST_NOTMAKE(GTEST_LDFLAGS)
AM_SUBST_NOTMAKE(GTEST_STATIC_LIBS)
])
