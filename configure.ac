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

dnl If changing version number here, also get minor/patchlevel below:
AC_INIT([opencpi],[1.4],[discuss@lists.opencpi.org])
AC_DEFINE([OCPI_VERSION_MAJOR],1,[Major version])
AC_DEFINE([OCPI_VERSION_MINOR],4,[Minor version])
AC_DEFINE([OCPI_VERSION_PATCHLEVEL],0,[Patchlevel version])

dnl Bring in tool paths (if applicable)
dnl next line is a bug that you SHOULD be able to do that with only "--with-cross-dir" AV-1511
AS_IF([test -n "$OCPI_CROSS_BUILD_BIN_DIR"], [export PATH=${OCPI_CROSS_BUILD_BIN_DIR}:${PATH}])
AS_IF([test -z "$OCPI_CONFIGURE_IS_RUNNING" -a -z "$CONFIG_SHELL" ], [AC_MSG_ERROR([Please use ocpi-configure or cross-configure and do not call configure directly.])])
AC_CONFIG_AUX_DIR([build-aux])
AC_PREREQ([2.63])

dnl Export OCPI_TARGET_CFLAGS/OCPI_TARGET_CXXFLAGS flags early (to ensure sysroot and other flags make it for cross-compilers)
AS_IF([test -n "$OCPI_TARGET_CFLAGS"],[CFLAGS="$OCPI_TARGET_CFLAGS"])
AS_IF([test -n "$OCPI_TARGET_CXXFLAGS"],[CXXFLAGS="$OCPI_TARGET_CXXFLAGS"])

AC_USE_SYSTEM_EXTENSIONS
dnl https://lists.gnu.org/archive/html/automake/2012-05/msg00014.html
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CONFIG_SRCDIR([runtime/util/driver/src/OcpiDriverManager.cxx])
AC_CONFIG_HEADERS([av_config.h])
AC_CONFIG_MACRO_DIR([releng/acinclude])
dnl https://www.gnu.org/software/automake/manual/html_node/List-of-Automake-options.html
AM_INIT_AUTOMAKE([-Wall -Wno-portability foreign subdir-objects no-dist no-installinfo no-installman])
dnl make V=1 to show all commands, but default to silent
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifndef([AS_VAR_APPEND],[m4_define([AS_VAR_APPEND],[$1="$$1$2"])])

AC_PROG_CXX
AS_IF([test -n "$OCPI_CROSS_HOST" -a -z "$CROSS_CONFIGURE_RUNNING" -a -z "$CONFIG_SHELL"],
      AC_MSG_ERROR([OCPI_CROSS_HOST set ($OCPI_CROSS_HOST).
      Please use cross-configure with OCPI_TARGET_PLATFORM and do not call ocpi-configure or configure directly.]))
AS_IF([test -n "$OCPI_CROSS_BUILD_BIN_DIR" && test "$CXX" == "g++"], AC_MSG_ERROR([Cross-compiler not found in path!]))
AC_PROG_CC
dnl This next macro is obsolete in automake 1.14
m4_ifdef([AM_PROG_CC_C_O], [AM_PROG_CC_C_O])
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PROG_GREP
AC_PROG_MKDIR_P
AC_PROG_SED

LT_INIT([pic-only])
AC_SUBST([LIBTOOL_DEPS])
AX_C___ATTRIBUTE__        dnl function described in releng/acinclude/ax_c__attribute.m4
AS_VAR_IF([ax_cv___attribute__],[yes],,AC_MSG_ERROR([Compiler does not support attributes?]))

dnl the priority is: specify full name at configure, --with-x, environmental variable, figure it out from the environment
dnl honestly, this hasn't been all that tested and will likely have subtle bugs
OCPI_OCPI_CDK_DIR         dnl function described in releng/acinclude/ocpi_cdk.m4
OCPI_IMPORT_PLATFORMS     dnl function described in releng/acinclude/ocpi_import_platforms.m4
export OCPI_PROJECT_PATH=$ocpi_cv_import_platforms
OCPI_OCPI_TOOL_HOST       dnl function described in releng/acinclude/ocpi_tool_host.m4
OCPI_OCPI_TARGET_HOST     dnl function described in releng/acinclude/ocpi_target_host.m4
OCPI_OCPI_TOOL_PLATFORM   dnl function described in releng/acinclude/ocpi_tool_platform.m4
OCPI_OCPI_TARGET_PLATFORM dnl function described in releng/acinclude/ocpi_target_platform.m4
OCPI_CLOCK_TYPES          dnl function described in releng/acinclude/ocpi_clock_gettime.m4
ocpi_build_dir=target-$ocpi_cv_ocpi_target_host
AC_SUBST(ocpi_build_dir, $ocpi_build_dir)

OCPI_OCPI_BUILD_SHARED_LIBRARIES dnl function described in releng/acinclude/ocpi_build_shared_libraries.m4
OCPI_OCPI_DEBUG           dnl function described in releng/acinclude/ocpi_debug.m4

dnl These flags are used by every compiler:
COMMON_CFLAGS="  -Wall -Wfloat-equal -Wextra -fno-strict-aliasing -Wformat -Wuninitialized -Winit-self -Wshadow -frecord-gcc-switches"
COMMON_CFLAGS+=" -fstack-protector -Wno-conversion -Wno-sign-conversion"

dnl These flags are added to the core but not third-party imports (e.g. MD5):
STRICT_FLAGS="  -Wconversion -Werror=parentheses -Werror=unused-parameter -Werror=empty-body"
STRICT_FLAGS+=" -Werror=write-strings -Werror=reorder -Werror=extra -Werror=comment -Werror=format -Werror=init-self"
STRICT_FLAGS+=" -Wsign-conversion"
dnl Some day:
dnl -Werror=shadow

dnl Not in all GCC versions (and checks guarded from CentOS6's autoconf 2.63):
m4_version_prereq([2.64],[
  AX_CHECK_COMPILE_FLAG([-Wmaybe-uninitialized],[COMMON_CFLAGS+=" -Wmaybe-uninitialized";STRICT_FLAGS+=" -Werror=maybe-uninitialized"])
  AX_CHECK_COMPILE_FLAG([-Wmisleading-indentation],[COMMON_CFLAGS+=" -Wmisleading-indentation";STRICT_FLAGS+=" -Werror=misleading-indentation"])
  AX_CHECK_COMPILE_FLAG([-Wnull-dereference],[COMMON_CFLAGS+=" -Wnull-dereference";STRICT_FLAGS+=" -Werror=null-dereference"])
  AX_CHECK_COMPILE_FLAG([-Wunused-result],[COMMON_CFLAGS+=" -Wunused-result";STRICT_FLAGS+=" -Werror=unused-result"])
  AX_CHECK_COMPILE_FLAG([-Wunused-but-set-variable],[COMMON_CFLAGS+=" -Wunused-but-set-variable";STRICT_FLAGS+=" -Werror=unused-but-set-variable"])
  AX_CHECK_COMPILE_FLAG([-fsanitize=undefined],[STRICT_FLAGS+=" -fsanitize=undefined"])
],[true])

dnl AX_CHECK_COMPILE_FLAG([-fsanitize=thread],[COMMON_CFLAGS+=" -pie -fsanitize=thread"])

AS_VAR_IF([OCPI_TARGET_CFLAGS],,[OCPI_TARGET_CFLAGS="$COMMON_CFLAGS -std=c99"])
AS_VAR_IF([OCPI_TARGET_CXXFLAGS],,[OCPI_TARGET_CXXFLAGS="$COMMON_CFLAGS -std=c++0x"])

AS_CASE([$host_os],
dnl	[*darwin*],[],
	[*linux*],[],
	[AC_MSG_ERROR([Platform not supported])])

dnl Check if cross-compiling. If so, check for cross-dir set.
dnl This catches GNU-style --host=
AS_CASE([$host_cpu],
        [$build_cpu],[ocpi_cross_compiling=0],
        [ocpi_cross_compiling=1; OCPI_OCPI_CROSS_BUILD_BIN_DIR OCPI_OCPI_CROSS_HOST])

dnl This catches legacy non-empty OCPI_CROSS_HOST definitions
AS_VAR_IF([OCPI_CROSS_HOST],,,[ocpi_cross_compiling=1; OCPI_OCPI_CROSS_BUILD_BIN_DIR OCPI_OCPI_CROSS_HOST])

INIT_CXXFLAGS="${OCPI_TARGET_CXXFLAGS} -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS $lt_prog_compiler_pic"
INIT_CFLAGS="${OCPI_TARGET_CFLAGS} -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS $lt_prog_compiler_pic"

dnl Save flags for some checks
SAVED_CXXFLAGS="${CXXFLAGS}"
CXXFLAGS="${INIT_CXXFLAGS}"

dnl Check for nullptr now that C++ standard is set in INIT_CXXFLAGS
AX_CHECK_NULLPTR dnl function described in releng/acinclude/ax_check_nullptr.m4

dnl Check for working std::chrono (AV-1567)
OCPI_CHECK_CHRONO dnl function described in releng/acinclude/ocpi_check_chrono.m4

dnl Check for python-2.7
PKG_CHECK_MODULES(PYTHON, python-2.7, [pythonfound=yes], [pythonfound=no])

dnl Restore
CXXFLAGS="${SAVED_CXXFLAGS}"

dnl These are from Makefile.ocpi.common
OCPI_CONFIG_DEFINES+="\
  -DOCPI_OS_${ocpi_target_os} \
  -DOCPI_OS=OCPI${ocpi_target_os} \
  -DOCPI_OS_VERSION_${ocpi_target_os_version} \
  -DOCPI_OS_VERSION=${ocpi_target_os_version} \
  -DOCPI_PLATFORM_${ocpi_target_platform} \
  -DOCPI_PLATFORM=${ocpi_target_platform} \
  -DOCPI_ARCH_${ocpi_target_arch} \
  -DOCPI_ARCH=${ocpi_target_arch} \
  -DOCPI_DEBUG=${ocpi_debug} \
  -DOCPI_DYNAMIC=${ocpi_build_shared_libraries} \
"
AM_CONDITIONAL([OCPI_RPMBUILD], [test x${RPM_BUILD_ROOT} != x])
AM_CONDITIONAL([ON_JENKINS], [test x${JENKINS_URL} != x])
AM_CONDITIONAL([OCPI_CROSSCOMPILING], [test x${ocpi_cross_compiling} = x1])
dnl This is because "test" and "expr" don't do quite what was needed:
AS_CASE([x${ocpi_target_platform}],[xxilinx*],[zynq_yes=1],[xpico_t6a],[zynq_yes=1])
AM_CONDITIONAL([OCPI_CROSSCOMPILING_ZYNQ], [test x${zynq_yes} = x1])
dnl This is unused right now...? AM_COND_IF([OCPI_CROSSCOMPILING],[OCPI_CONFIG_DEFINES+=" -DOCPI_CROSSCOMPILING"])

INIT_CFLAGS+=" ${OCPI_CONFIG_DEFINES}"
INIT_CXXFLAGS+=" ${OCPI_CONFIG_DEFINES}"

AC_SUBST(INIT_LDFLAGS)
AC_SUBST(INIT_CXXFLAGS)
AC_SUBST(INIT_CFLAGS)
AC_SUBST(STRICT_FLAGS)
AC_SUBST(ocpi_target_host)
AC_SUBST(ocpi_cdk_dir)
dnl This is to guard against automake seeing "invalid" GNU-make "ifeq" pairs, etc:
AC_SUBST(AUTOGUARD,[])
AM_SUBST_NOTMAKE(AUTOGUARD)

dnl Checks for libraries.
OCPI_CHECK_LZMA           dnl function described in releng/acinclude/lzma.m4
OCPI_CHECK_GTEST          dnl function described in releng/acinclude/gtest.m4
AC_SEARCH_LIBS([socket],[socket network],,AC_MSG_ERROR([Could not find library containing socket call]))
AC_SEARCH_LIBS([pthread_create],[pthread],,AC_MSG_ERROR([Could not find library containing pthread_create call]))
AC_SEARCH_LIBS([gettimeofday],[rt],,AC_MSG_ERROR([Could not find library containing gettimeofday call]))
AC_SEARCH_LIBS([shm_open],[rt],,AC_MSG_ERROR([Could not find library containing shm_open call])) dnl needed by fasttime on systems where gettimeofday is in stdlib
AC_SEARCH_LIBS([exp],[m],,AC_MSG_ERROR([Could not find library containing exp call]))
AC_SEARCH_LIBS([clock_gettime],[rt],,AC_MSG_ERROR([Could not find library containing clock_gettime call]))

dnl Checks for header files.
AC_FUNC_ALLOCA
AS_VAR_IF([ac_cv_working_alloca_h],[yes],,AC_MSG_ERROR([Could not find required header: alloca.h]))
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h locale.h malloc.h memory.h netdb.h netinet/in.h time.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h sys/time.h unistd.h utime.h],,AC_MSG_ERROR([Could not find required header: $ac_header]))
AC_CHECK_HEADERS([valgrind/valgrind.h],AC_DEFINE([OCPI_HAVE_VALGRIND_H],1,[Define to 1 if valgrind/valgrind.h is available]))

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AS_VAR_IF([ac_cv_header_stdbool_h],[yes],,AC_MSG_ERROR([Could not find required header: stdbool.h]))
AC_C_INLINE
AS_VAR_IF([ac_cv_c_inline],[no],AC_MSG_ERROR([Compiler does not support inline?]))
AC_C_RESTRICT
AC_STRUCT_TIMEZONE
dnl ensure bool is one byte (AV-810)
AC_LANG_PUSH(C++)
AC_CHECK_SIZEOF([bool])
AS_VAR_IF([ac_cv_sizeof_bool],[1],,AC_MSG_ERROR([sizeof(bool) is $ac_cv_sizeof_bool and not 1]))
AC_LANG_POP(C++)

dnl explicitly check it's not in compatible mode
AC_CHECK_MEMBERS([struct tm.tm_zone],,AC_MSG_ERROR([Could not find required timezone structure.]))
AC_CHECK_TYPES([ptrdiff_t,off_t,pid_t,size_t,ssize_t,int8_t,int16_t,int32_t,int64_t,uint8_t,uint16_t,uint32_t,uint64_t],[],AC_MSG_ERROR([Could not find required type.]))

dnl Checks for library functions.
AC_FUNC_FORK
dnl AC_CHECK_FUNCS runs ACTIONS if found / not found (compare to AC_CHECK_PROG)
AC_CHECK_FUNCS([atexit bzero clock_gettime dup2 floor fork fseek fseeko ftruncate getcwd gethostbyaddr gethostbyname gethostname getpagesize gettimeofday if_nameindex inet_ntoa localeconv localtime_r malloc memchr memmove memset mkdir mkfifo mktime mmap munmap pow putenv realloc regcomp rmdir select setenv setlocale socket sqrt strcasecmp strchr strcspn strdup strerror strncasecmp strndup strnlen strpbrk strrchr strspn strstr strtod strtol strtoul strtoull uname utime],,AC_MSG_ERROR([Could not resolve required function]))

dnl Checks for executables
dnl AC_CHECK_PROG sets VALUES if found / not found (compare to AC_CHECK_FUNCS)
dnl This requires a two-step process of set and then check a flag
dnl Unlike other functions like AC_CHECK_FUNCS, the plural AC_CHECK_PROGS searches for ALTERNATES
AC_CHECK_PROG(LOCATE_CHECK,[locate],[yes])
AS_VAR_IF([LOCATE_CHECK],[],AC_MSG_ERROR([cannot find locate executable]))
AC_CHECK_PROG(PATCHELF_CHECK,[patchelf],[yes],[],[$PATH$PATH_SEPARATOR/opt/opencpi/prerequisites/patchelf/bin/])
AS_VAR_IF([PATCHELF_CHECK],[],AC_MSG_ERROR([cannot find patchelf executable]))
AC_CHECK_PROG(READELF_CHECK,[readelf],[yes])
AS_VAR_IF([READELF_CHECK],[],AC_MSG_ERROR([cannot find readelf executable]))
AC_CHECK_PROG(XARGS_CHECK,[xargs],[yes])
AS_VAR_IF([XARGS_CHECK],[],AC_MSG_ERROR([cannot find xargs executable]))
AC_CHECK_PROG([SWIG_CHECK],[swig],[yes])
AM_CONDITIONAL([SWIG_CHECK], [test "x$SWIG_CHECK" = xyes -a "x$pythonfound" = xyes])

dnl Generate output
AC_CONFIG_FILES([Makefile])
dnl https://lists.gnu.org/archive/html/bug-autoconf/2008-08/msg00028.html for pushing variable into filename
AM_COND_IF([OCPI_CROSSCOMPILING],
      AC_CONFIG_FILES([tools/cdk/include/autoconfig_import-${ocpi_target_platform}.mk:tools/cdk/include/autoconfig_import.mk.in],[],[ocpi_cross_host='${ocpi_cross_host}']),
      AC_CONFIG_FILES([tools/cdk/include/autoconfig_import.mk]))

AC_OUTPUT
AM_COND_IF([OCPI_CROSSCOMPILING],AS_BOX([Configured to cross-compile for ${ocpi_target_arch}/${ocpi_target_os_version}]))
