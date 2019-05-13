# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

##########################################################################################
# This file provides defaults and documentation for platform-specific build variables.
#
# Any platform can override these variables except that the "*required*" flags indicate desirable
# functionality that should be invoked.  So if one of the required flags is not valid,
# replace it with an equivalent that performs the nearest available function.
# i.e.
# OcpiRequiredCFlags=$(patsubst <bad-flag>,<good-flag>,$(OcpiRequiredCFlags))
#
# The defaults are basically what CentOS6 wants, so everything other platform definition
# is essentially empty except to the extent that it deviates from the CentOS6 baseline.
# This file might be read more than once to RESET all the variables to their defaults.
#
# Some conventions:
#  - a boolean variable is 1 for true, otherwise false.
#  - linker related flags generally assume the use of the compiler command for linking
#    i.e. they would typically need a -Xlinker or -Wl, etc.
#    This is as opposed to directly using the underlying linker, e.g. using ld directly
#  - these variables are "immediate/simply-expanded" make variables,
#    not "deferred/recursively-expanded" make variables
#  - Variable names are camelcase to make it crystal clear they are not environment variables
#    I.e. if we DO want environment variables we use all upper case

# There is no guard on this file.  It is included repeatedly to set the variables to default
# values.

# Every platform MUST set at least these three variables, even if their values are the default.
#    OcpiPlatformOs=<e.g. linux>
#    OcpiPlatformOsVersion=<e.g. c6>
#    OcpiPlatformArch=<e.g. x86_64>

# Compilation modes:
# There are two aspects to compilation: strict or not, and dynamic or not.
# Strict is used for OpenCPI's own code.  Not strict is used for imported or prerequisite code.
# Strict mostly applies to warnings, and making some warnings into errors, but other options
# might apply in terms of code checking.
# Dynamic means suitable for dynamically loaded code, which we use for:
# 1. RCC workers
# 2. Plugins
# 3. Libraries that are not plugins only when the whole framework build mode is dynamic

# Library types:
# When libraries are created, they are fundamentally of these types:
# 1. Normal framework libraries, static or dynamic depending the overall framework build mode
# 2. Plugins that are always dynamic
# 3. Stubs that are used temporarily during the build process to check for undefined symbols
#    in libraries that are accessing external libraries that may or may not be present.
# 4. SWIG libraries that will link against all the normal libraries static or dynamic,
#    depending on the overall framework build mode.  AND when it links against normal libraries
#    when the overall framework mode is static, it links against special versions of those
#    static libraries which are built using the dynamic compilation mode because the final
#    resulting SWIG library is in fact a dynamic library.

# When the framework build mode is static, executables are statically linked in such a way
# that symbols in the executable are accessible from dynamically loaded plugins and RCC workers.

# The list of all variables defaulted and settable by platforms
# The value of this variable should not be changed lightly since it may affect all platforms.
# Each variable has a comment with the default values below.
# In case insensitive alphabetical order
OcpiComma:=,
OcpiAllPlatformVars:=\
  OcpiAR OcpiAsNeeded OcpiCanRemoveNeeded \
  OcpiCC OcpiCFlags OcpiCLD OcpiCrossCompile OcpiCXX OcpiCXXFlags OcpiCXXLD \
  OcpiDebugOffFlags OcpiDebugOnFlags OcpiDependencyFlags \
  OcpiDynamicCompilerFlags OcpiDynamicLibraryFlags OcpiDynamicLibrarySuffix \
  OcpiDynamicProgramFlags OcpiDynamicSwigFlags \
  OcpiExtraLibs OcpiGetTimeClockId OcpiKernelDir OcpiLD \
  OcpiLibraryPathEnv OcpiOclLibs OcpiOptionalCWarnings OcpiOptionalCXXWarnings \
  OcpiPlatform OcpiPlatformArch OcpiPlatformDir OcpiPlatformOs OcpiPlatformOsVersion \
  OcpiPlatformPrerequisites OcpiPluginFlags \
  OcpiRccCXXFlags  OcpiRccLDFlags \
  OcpiRequiredCFlags OcpiRequiredCPPFlags OcpiRequiredCXXFlags OcpiRpathOrigin \
  OcpiStaticLibraryFlags OcpiStaticLibrarySuffix OcpiStaticProgramFlags OcpiStaticSwigFlags \
  OcpiStrictCFlags OcpiStrictCXXFlags OcpiSTRIP OcpiSWIG \
  OcpiUnknownWarningsError OcpiPythonInclude \

# The list of variables containing optional warnings needed early (for autoreconf).
OcpiAllOptionalWarningsVars:=OcpiOptionalCWarnings OcpiOptionalCXXWarnings

# Can this platform support removing dynamic library references from a linked dynamic library
# (e.g. remote SO_NEEDED entries, using patchelf)
OcpiCanRemoveNeeded:=1
# The name of the environment variable for dynamic library loading.
# Rarely used, but needed for some cases.
OcpiLibraryPathEnv:=LD_LIBRARY_PATH
# The string used in executable/dynamic-library rpath settings indicating where the
# executable lives (the directory it is in).
OcpiRpathOrigin:=$${ORIGIN}
OcpiStaticLibrarySuffix:=.a
# linker flags when linking a static executable
OcpiStaticProgramFlags:=-Xlinker -export-dynamic -Wl,-z,relro -Wl,-z,now
# flags when linking a static library
OcpiStaticLibraryFlags:=
OcpiDynamicCompilerFlags:=-fPIC
OcpiDynamicLibrarySuffix:=.so
# linker flags when creating a dynamic library
OcpiDynamicLibraryFlags:=-shared -Xlinker --no-undefined -Wl,-z,relro -Wl,-z,now
# linker flags when creating an executable linked with dynamic libraries
OcpiDynamicProgramFlags:= -Wl,-z,relro -Wl,-z,now
# linker flags when creating a swig library linked against static libraries: FIXME: needed?
OcpiStaticSwigFlags:=-Xlinker -export-dynamic -Wl,-z,relro -Wl,-z,now
# linker flags when creating a swig library linked against dynamic libraries
OcpiDynamicSwigFlags:=
# linker flags when creating a driver/plugin library
# Note this is NOT an immediate variable
OcpiPluginFlags:=$(filter-out -Wl$(OcpiComma)-z$(OcpiComma)now,$(OcpiDynamicLibraryFlags))
# When linking against dynamic libraries, these options indicate that libraries on the
# linker command line that are not referenced should not be "needed" by the resulting
# executable or dynamic library
OcpiAsNeeded:=-Xlinker --no-as-needed
# The system libraries that are required on this platform for the framework.
# rt is typical for clockgettime, dl for dlopen.
# on some platforms these are all implicit
OcpiExtraLibs:=rt dl pthread
# The system's default library flag when using OpenCL
OcpiOclLibs:=-lOpenCL
# String indicating the (absolute) pathname prefix for tools for this platform.
# If blank, then it is not a cross-compiled platform
# Example for using zynq tools from xilinx
#    include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk
#    OcpiCrossCompile:=$(OcpiXilinxEdkDir)/gnu/arm/lin/bin/arm-xilinx-linux-gnueabi-
OcpiCrossCompile:=
########## The variables for "commands" and "languages" use all upper case to be slightly more
# similar to other make or autotools variables etc.
# These default -g flags are not intended to suppress optimization, just keep sym info around
# See DebugOn/Off flags below
OcpiCFlags:=-g -pipe
OcpiCXXFlags:=-g -pipe
# typical "make"
# The commands used to build for this platform
OcpiCC:=gcc
OcpiCXX:=g++
# These are the linker commands to use, per language
OcpiCLD:=gcc
OcpiCXXLD:=g++
OcpiAR:=ar
OcpiLD:=ld
OcpiSTRIP:=strip
# Where linux kernel headers should be found for out-of-tree building of OpenCPI kernel driver
# When not set, it is found in the default/standard place for a development system
# Either relative to where the platform is defined in its directory, or absolute
OcpiKernelDir:=
# These flags are followed directly by the name of the dependency file, roughly like -o
# MMD is for user headers, not ones included with <>.
# MP is to create phone targets for each such file so that it is not an error if it is deleted.
# MF is to indicate the actual file to write dependencies to
OcpiDependencyFlags:=-MMD -MP -MF$(Space)
# Other than -g, there is no need to force optimization off
OcpiOptimizedOffFlags:=
# FIXME: discuss whether we need to specifically offer independent assert control
# FIXME: discuss whether we should universally make the default to be debug (yes)
# FIXME: is there a need for C vs. C++ here?
# FIXME: is any level of FORTIFY_SOURCE allowed in debug mode?
# FIXME: does this work any differently in clang or macos?
# This is biased for best optimizations (bipolar)
# Note that FORTIFY_SOURCE is only active with optimization and a warning is issued if
# it is set without optimization (go figure).
OcpiOptimizeOnFlags:=-O2 -NDEBUG=1 -Wp,-D_FORTIFY_SOURCE=2
# For all code:
# We require these or their equivalent be supported.  If not its nearly an error
# I.e. there may be problems since code may well depend on it.
# They should not be overridden or reset by a platform, except perhaps to change the syntax
# when some equivalent name or syntax is used.
# The warnings could be removed (e.g. using filter-out) if they are truly not supported.
OcpiRequiredCPPFlags:= -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
OcpiRequiredCFlags:= -Wall -Wfloat-equal -Wextra -fno-strict-aliasing -Wformat -Wuninitialized \
                     -Winit-self -grecord-gcc-switches -fstack-protector \
                     -Wno-conversion -Wno-sign-conversion -std=c99 --param=ssp-buffer-size=4
OcpiRequiredCXXFlags:=$(filter-out -std%,$(OcpiRequiredCFlags)) -std=c++0x
# For all code:
# We prefer these warnings or their equivalent be supported.  It is not an error if they are not.
# If they are supported they are "error" warnings in strict cases, otherwise normal warnings
OcpiOptionalCWarnings:=maybe-uninitialized misleading-indentation null-dereference \
                       unused-result unused-but-set-variable
OcpiOptionalCXXWarnings:=$(OcpiOptionalCWarnings)
# For "native" OpenCPI code (as opposed to imported code):
# We prefer to ADD these or their equivalent.  It is not an error if they are not supported.
OcpiStrictCFlags:=-Wconversion -Werror=parentheses -Werror=unused-parameter -Werror=empty-body \
                  -Werror=write-strings -Werror=reorder -Werror=extra -Werror=comment \
                  -Werror=format -Werror=init-self -Wsign-conversion -Wshadow
OcpiStrictCXXFlags:=$(OcpiStrictCFlags)
# What we want to burden users with by default
OcpiRccCXXFlags:=-Wall -Wextra
OcpiRccLDFlags:=-g -shared
# Set this to the option that causes unknown warnings options to be errors
# The default (gcc) always causes errors with unknown warning options,
# but some compilers only warn unless you give another option (e.g. clang).
,OcpiUnknownWarningsError:=
# These next three are REQUIRED to be set in every platform definition file.
# These default variables are just "for example"
OcpiPlatformOs:=linux
OcpiPlatformArch:=x86_64
# Defining platform versions is discussed in the platform development guide.
# Normally a single letter for the linux distribution, followed immediately by the major version
# e.g. c6 for CentOS6, r5 for RHEL5, u16, for ubuntu 16 etc.
OcpiPlatformOsVersion:=
# This is a list of platform-specific prerequisite packages required for this platform.
# for each one there must be an install-<foo>.sh script in the platform's directory.
# They are normally just toolchains
OcpiPlatformPrerequisites:=
# This specifies the name/id of the preferred clock id for the POSIX clock_gettime function.
# It is the first argument.  The default is the "preferred" value.
# We generally fall back to CLOCK_MONOTONIC if CLOCK_MONOTONIC_RAW is not supported.
# In particular CLOCK_MONOTONIC_RAW is broken in centos6.  I.e. the symbol is defined but the
# functionality is broken.  Hence we need to specify a default, which is NOT in fact the default
# for centos6
OcpiGetTimeClockId:=CLOCK_MONOTONIC_RAW
