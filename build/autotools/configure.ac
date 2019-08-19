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
dnl
dnl ================================================================================
dnl Naming conventions:
dnl  AC_DEFINE variables are CPP macro names and are thus in uppoer case
dnl  Other variables here in upper case are environment variables which we generally avoid
dnl  Variables in camel case come from OpenCPI (platform variables)
dnl  Variables in lower case with underscores are variables we define and use here or Makefile.am

dnl ================================================================================
dnl 0. Set constants for the software, and autoconf (not auto-discovered values)
dnl ================================================================================
dnl If changing version number here, also get minor/patchlevel below:
AC_INIT([opencpi],[1.6.0],[discuss@lists.opencpi.org])
dnl This version is what comes with centos6, which is as far back as we want to go.
AC_PREREQ([2.63])
AC_DEFINE([VERSION_MAJOR],1,[Major version])
AC_DEFINE([VERSION_MINOR],6,[Minor version])
AC_DEFINE([VERSION_PATCHLEVEL],0,[Patchlevel version])
dnl Note that this is renamed by AX_PREFIX_CONFIG below
AC_CONFIG_HEADERS([config.h])
dnl Define the fixed prefix for the macros and the file name
AX_PREFIX_CONFIG_H(ocpi-config.h,OCPI,config.h)
AC_CONFIG_MACRO_DIR([acinclude])
AC_CONFIG_AUX_DIR([build-aux])
dnl This is a pathname that must be present so you know you are executing in the right place.
AC_CONFIG_SRCDIR([runtime/rcc/src/RccContainer.cxx])
dnl ============================================================================================
dnl 1. Import the OpenCPI platform variable values and set libtool variables
dnl    all for substitutions in Makefile.am, ocpi-config.h, or ocpi-platform-config.h
dnl    CamelCase Variables starting with "Ocpi" passed to us via platform-variables.sh
dnl    Put together compilation flags
dnl ============================================================================================
# Import OpenCPI platform variables.  The "./" is necessary to avoid path problems on some OSes.
# These variables are not used when processing configure.ac->configure.in->configure
# This "source" command is issued when the generated "configure" script is executed, which is
# is from the "target-<platform>" directory.
source ./platform-variables.sh
# Import OpenCPI optional warnings variables USED WHEN autoreconf is processing configure.ac
m4_include(optional-warnings.m4)
# Set libtool flags - assuming they are platform-agnostic
libtool_static_program_flags="-static -export-dynamic -static-libtool-libs"
libtool_dynamic_program_flags="-shared -avoid-version"
libtool_static_library_flags="-all-static -static"
libtool_dynamic_library_flags="-shared -avoid-version"
# -module alone is not enough to surpress creating the static library WTF?
libtool_plugin_library_flags="-shared -avoid-version"
# Get the two options that determine actual compiler flags etc.
# Arg 3 to AC_ARG_ENABLE being blank means it does more for you - defines and sets the variable
AC_ARG_ENABLE([debug],
	      [AS_HELP_STRING([--enable-debug], [Enable for best debugging (default is yes)])],
	      [],[enable_debug=yes])
AC_ARG_ENABLE([dynamic],
              [AS_HELP_STRING([--enable-dynamic],
		              [Use dynamic libraries with executables (default is no)])],
	      [],[enable_dynamic=no])
# Set default so environment will be ignored
AC_ARG_VAR(prerequisite_dir, [where prerequisites are installed and used])
#AC_ARG_VAR(platform_dir, [where compilations are installed])
#we should push this stuff into Makefile.am since we have the conditional there
if test $enable_dynamic = yes ; then
   libtool_library_flags="$libtool_dynamic_library_flags"
   libtool_program_flags="$libtool_dynamic_program_flags"
   ocpi_library_suffix="$OcpiDynamicLibrarySuffix"
   ocpi_library_flags="$OcpiDynamicLibraryFlags"
   ocpi_program_flags="$OcpiDynamicProgramFlags"
   ocpi_swig_flags="$OcpiDynamicSwigFlags"
   ocpi_dynamic=1
else
   libtool_library_flags="$libtool_static_library_flags"
   libtool_program_flags="$libtool_static_program_flags"
   ocpi_library_suffix="$OcpiStaticLibrarySuffix"
   ocpi_library_flags="$OcpiStaticLibraryFlags"
   ocpi_program_flags="$OcpiStaticProgramFlags"
   ocpi_swig_flags="$OcpiStaticSwigFlags"
   ocpi_dynamic=0
fi
libtool_library_flags+=" -no-undefined"
libtool_program_flags+=" -no-undefined"
if test $enable_debug = yes; then
   ocpi_debug_flags="$OcpiDebugOnFlags"
   ocpi_debug=1
else
   ocpi_debug_flags="$OcpiDebugOffFlags"
   ocpi_debug=0
fi
# set the cflags and cxxflags from the platform variables
common_cppflags="$OcpiRequiredCPPFlags"
common_cflags="$OcpiRequiredCFlags $OcpiCFlags $ocpi_debug_flags"
strict_cflags="$OcpiStrictCFlags"
common_cxxflags="$OcpiRequiredCXXFlags $OcpiCXXFlags $ocpi_debug_flags"
strict_cxxflags="$OcpiStrictCXXFlags"
# add to the c/cxx flags based on testing support for optional warnings
m4_version_prereq([2.64],[
  CFLAGS="$OcpiUnknownWarningsError"
  CXXFLAGS="$OcpiUnknownWarningsError"
  AC_LANG_PUSH(C)
  m4_foreach_w([ocpi_w],OcpiOptionalCWarnings,
               [AX_CHECK_COMPILE_FLAG([-W]ocpi_w,
                                      [common_cflags+=" [-W]ocpi_w";
				       strict_cflags+=" [-Werror=]ocpi_w"])])
  AC_LANG_POP(C)
  AC_LANG_PUSH(C++)
  m4_foreach_w([ocpi_w],OcpiOptionalCXXWarnings,
               [AX_CHECK_COMPILE_FLAG([[-W]ocpi_w],
                                      [common_cxxflags+=" [-W]ocpi_w";
				       strict_cxxflags+=" [-Werror=]ocpi_w"])])
  AC_LANG_POP(C++)
],[
  CFLAGS=
  CXXFLAGS=
])
# These are needed in the Makefile.am
m4_foreach_w([ocpi_var],[
  OcpiDynamicLibrarySuffix OcpiStaticLibrarySuffix OcpiDynamicLibraryFlags OcpiStaticLibraryFlags
  OcpiPluginFlags OcpiExtraLibs
  libtool_static_library_flags libtool_dynamic_library_flags libtool_plugin_library_flags
  libtool_library_flags libtool_program_flags
  common_cppflags common_cflags common_cxxflags strict_cppflags strict_cflags strict_cxxflags
  ocpi_dynamic ocpi_library_suffix ocpi_library_flags ocpi_program_flags ocpi_swig_flags OcpiSWIG
  PYTHON_VERSION OcpiPythonInclude
], [AC_SUBST(ocpi_var)])

# These are needed in ocpi-config.h and ocpi-platform-config.h
# These come from the platform definition itself
AC_SUBST([OcpiPlatform])
AC_SUBST([OcpiPlatformOs])
AC_SUBST([OcpiPlatformOsVersion])
AC_SUBST([OcpiPlatformArch])
dnl ============================================================================================
dnl 2. Perform some platform checks that we need
dnl    Find some programs and check whether they are good enough
dnl ============================================================================================
AC_PROG_SED
AC_PROG_GREP
AC_PROG_LN_S
AC_PROG_CXX
dnl If Swig was not specified by the platform variables, find it.
AC_PATH_PROG([OcpiSWIG],[swig],AC_MSG_ERROR([Required program 'swig' not found.]))
dnl AC_USE_SYSTEM_EXTENSIONS # this was used in the past but is obsolete/unneeded now

dnl ============================================================================================
dnl 3. Enable automake to do Makefile.am->Makefile.in and autoconf to do Makefile.in->Makefile
dnl ============================================================================================
AM_INIT_AUTOMAKE([-Wall -Wno-portability foreign subdir-objects no-dist no-installinfo no-installman silent-rules])
AM_CONDITIONAL([ocpi_is_dynamic],[test $enable_dynamic = yes])
AM_CONDITIONAL([ocpi_is_cross],[test x"$OcpiCrossCompile" != x])
AM_CONDITIONAL([ocpi_can_remove_needed],[test x"$OcpiCanRemoveNeeded" = x1])
# The following checks the version and sets PYTHON and PYTHON_VERSION which we use.
# Unfortunately it does not provide the sys.prefix() from the running python.
# It sets PYTHON_PREFIX to the install location, not the current location.
# So we find PYTHON_PREFIX when the makefiles are executed $(shell....)
# It is python 2.6 since that is what centos6 has.
AM_PATH_PYTHON([2.6])
dnl
dnl ================================================================================
dnl 4. Enable generation of ocpi-config.h
dnl ================================================================================
dnl let autoheader do the right thing for defines with constant names
AC_DEFINE_UNQUOTED([DEBUG], $ocpi_debug, [OpenCPI debug compilation mode])
AC_DEFINE_UNQUOTED([DYNAMIC], $ocpi_dynamic, [OpenCPI dynamic libraries])
AC_DEFINE_UNQUOTED([DYNAMIC_SUFFIX], $OcpiDynamicLibrarySuffix, [OpenCPI dynamic library suffix])
AC_DEFINE_UNQUOTED([OS], OCPI$OcpiPlatformOs, [OpenCPI OS, e.g. linux, with OCPI prefix])
AC_DEFINE_UNQUOTED([OS_VERSION], $OcpiPlatformOsVersion, [OpenCPI OS Version, e.g. centos7])
AC_DEFINE_UNQUOTED([ARCH], $OcpiPlatformArch, [OpenCPI CPU Architecture/compiler, e.g. x86_64])
AC_DEFINE_UNQUOTED([PLATFORM], $OcpiPlatform, [OpenCPI platform])
AC_DEFINE_UNQUOTED([OPENCL_LIB], $OcpiOclLibs, [OpenCL libraries if installed/available])
AC_DEFINE_UNQUOTED([GETTIME_CLOCK_TYPE], $OcpiGetTimeClockId,
                   [CLOCK_MONOTONIC or CLOCK_MONOTONIC_RAW if system supports it])
dnl autoheader cannot handle defines with platform-specific names so they go into a separate file
AH_BOTTOM([#include "ocpi-config-platform.h"])
dnl ================================================================================
dnl 5. Determines some system attributes and puts them into a generated header file
dnl    Possibly by running test programs
dnl ================================================================================
AX_C___ATTRIBUTE__
AS_VAR_IF([ax_cv___attribute__],[yes],,AC_MSG_ERROR([Compiler does not support attributes?]))
dnl Check for nullptr now that C++ standard is set in CXXFLAGS
AX_CHECK_NULLPTR
dnl Check for working std::chrono (AV-1567)
OCPI_CHECK_CHRONO
dnl Check for best monotonic clock type(prefer CLOCK_MONOTONIC_RAW
dnl This is turned off since it does not solve the problem for cross-compilation anyway.
dnl OCPI_CLOCK_TYPES
dnl LT_INIT must be called after configuration tests due to AC_LANG_SOURCE
LT_INIT([dlopen])
AC_SUBST([LIBTOOL_DEPS])
dnl ================================================================================
dnl 7. Produce output
dnl ================================================================================
AC_CONFIG_FILES([Makefile ocpi-config-platform.h])
AC_OUTPUT
AS_IF([test -n "$OcpiCrossCompile"],
      [AS_BOX([Configured to cross-compile for platform: $OcpiPlatform])])
