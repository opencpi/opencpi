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

# SYNOPSIS
#
#   AX_CHECK_NULLPTR
#
# DESCRIPTION
#
#   Provides a test for the compiler support of C++11 nullptr.
#   Defines HAVE_NULLPTR if it is found.
#

AC_DEFUN([AX_CHECK_NULLPTR], [
  AC_LANG_PUSH([C++])
  AC_CACHE_CHECK([if ${CXX-c++} supports nullptr], [ax_cv_nullptr],
    [AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [[
        #include <cstddef>
        ]], [[
        char *char_null = nullptr;
        ]])],
      [ax_cv_nullptr=yes],
      [ax_cv_nullptr=no]
    )
  ])
  AS_VAR_IF([ax_cv_nullptr],[yes],[AC_DEFINE([HAVE_NULLPTR], 1, [Define to 1 if your compiler supports C++11 nullptr])])
  AC_LANG_POP([C++])
])
